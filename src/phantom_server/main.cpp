#include "headers/CorsHelper.h"
#include "headers/DocumentHandler.h"
#include "headers/SocketRequestHandler.h"
#include "headers/StaticFileHandler.h"
#include <App.h>
#include <fmt/core.h>
#include <fmt/std.h>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/services/CryptoRoom.h>
#include <phantomchat/services/RoomManager.h>
#include <phantomchat/utils/CacheFileProvider.hpp>
#include <string_view>
#include <thread>
#include <type_traits>

using namespace phantomchat::processors;

template<bool SSL>
void setup_rest(uWS::TemplatedApp<SSL> &app,
  phantomchat::services::RoomManager &room_manager,
  phantomchat::utils::CacheFileProvider &file_provider,
  moodycamel::BlockingConcurrentQueue<UploadTask<SSL>> &upload_task_queue,
  moodycamel::BlockingConcurrentQueue<DownloadTask<SSL>> &download_task_queue)
{
  auto handle_static_file_with_cache = [&file_provider](auto *res, auto *req) {
    phantomchat::handlers::handleStaticFile(res, req, file_provider);
  };
  auto handle_upload_document = [&room_manager, &upload_task_queue](auto *res, auto *req) {
    phantomchat::handlers::handleUploadDocument(res, req, room_manager, upload_task_queue);
  };
  auto handle_download_document = [&download_task_queue](auto *res, auto *req) {
    phantomchat::handlers::handleDownloadDocument(res, req, download_task_queue);
  };
  auto handle_options = [](auto *res, auto *req) {
    auto origin = phantomchat::cors::resolveOrigin(req->getHeader("origin"));
    res->writeStatus("204 No Content");
    phantomchat::cors::writePreflightHeaders(res, origin);
    res->end();
  };

  app.options("/*", handle_options)
    .get("/download-document/:room/:filename", handle_download_document)
    .post("/upload-document/:filename", handle_upload_document)
    .get("/*", handle_static_file_with_cache);
}

template<typename APP_TYPE> void setup_websocket(APP_TYPE &app, phantomchat::services::RoomManager &room_manager)
{
  app.template ws<phantomchat::contracts::PerSocketData>("/room",
    { .open = [](auto *) {},
      .message = [&room_manager](auto *ws,
                   std::string_view msg,
                   uWS::OpCode) { phantomchat::handlers::handleMessage(ws, room_manager, msg); },
      .close = [&room_manager, &app](auto *ws,
                 int,
                 std::string_view) { phantomchat::handlers::handleLeaveRoom(ws, room_manager, &app); } });
}

template<typename APP_TYPE> void setup_listen(APP_TYPE &app, const phantomchat::config::AppSettings &settings)
{
  app.listen(settings.listen_port, [&settings](auto *listen_socket) {
    if (listen_socket) {
      fmt::print("Thread {} listening on port {}...\n", std::this_thread::get_id(), settings.listen_port);
    } else {
      fmt::print("Thread {} failed to listen on port {}...\n", std::this_thread::get_id(), settings.listen_port);
    }
  });
}

int main()
{
  phantomchat::services::crypto_room::init();

  fmt::print("Hello, {}...\n", "PhantomServer");


  auto &settings = phantomchat::config::AppSettings::getInstance();
  settings.load_from_file("appsettings.json");

  auto &room_manager = phantomchat::services::RoomManager::getInstance();

  phantomchat::utils::CacheFileProvider file_provider(settings.web_root_path, settings.gzip_compression);

  const bool use_ssl = !settings.ssl_certificate.empty() && !settings.ssl_certificate_key.empty();

  auto run_workers = [&]<bool SSL>(std::bool_constant<SSL>) {
    moodycamel::BlockingConcurrentQueue<UploadTask<SSL>> upload_task_queue;
    moodycamel::BlockingConcurrentQueue<DownloadTask<SSL>> download_task_queue;

    const int worker_thread_count = std::max(1, settings.worker_threads);
    std::vector<std::jthread> worker_threads;
    for (int i = 0; i < worker_thread_count; ++i) {
      worker_threads.emplace_back([&] {
        uWS::SocketContextOptions options{};
        if constexpr (SSL) {
          options.key_file_name = settings.ssl_certificate_key.c_str();
          options.cert_file_name = settings.ssl_certificate.c_str();
        }
        uWS::TemplatedApp<SSL> app(options);

        auto uploadThread = uploadDocumentBackgroundProcess(&app, upload_task_queue);
        auto downloadThread = downloadDocumentBackgroundProcess(&app, download_task_queue);

        setup_rest(app, room_manager, file_provider, upload_task_queue, download_task_queue);
        setup_websocket(app, room_manager);
        setup_listen(app, settings);

        app.run();
      });
    }

    phantomchat::processors::backgroundTasksRunning = false;
  };

  if (use_ssl) {
    run_workers(std::bool_constant<true>{});
  } else {
    run_workers(std::bool_constant<false>{});
  }
}
