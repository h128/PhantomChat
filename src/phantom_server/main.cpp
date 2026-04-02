#include "headers/DocumentHandler.h"
#include "headers/SocketRequestHandler.h"
#include "headers/StaticFileHandler.h"
#include "headers/CorsHelper.h"
#include <App.h>
#include <fmt/core.h>
#include <fmt/std.h>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/services/RoomManager.h>
#include <phantomchat/utils/CacheFileProvider.hpp>
#include <sodium.h>
#include <string_view>
#include <thread>

using namespace phantomchat::processors;

template<typename APP_TYPE>
void setup_rest(APP_TYPE &app,
  phantomchat::services::RoomManager &room_manager,
  phantomchat::utils::CacheFileProvider &file_provider,
  moodycamel::BlockingConcurrentQueue<UploadTask> &upload_task_queue,
  moodycamel::BlockingConcurrentQueue<DownloadTask> &download_task_queue)
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
  if (sodium_init() == -1) { throw std::runtime_error("Failed to initialize libsodium"); }

  fmt::print("Hello, {}...\n", "PhantomServer");


  phantomchat::config::AppSettings settings;
  settings.load_from_file("appsettings.json");
  UploadTask::upload_root_path = settings.upload_path;
  phantomchat::cors::allowed_origins = settings.cors_allowed_origins;

  auto &room_manager = phantomchat::services::RoomManager::getInstance();

  phantomchat::utils::CacheFileProvider file_provider(settings.web_root_path);
  moodycamel::BlockingConcurrentQueue<UploadTask> upload_task_queue;
  moodycamel::BlockingConcurrentQueue<DownloadTask> download_task_queue;

  const int worker_thread_count = std::max(1, settings.worker_threads);
  std::vector<std::jthread> worker_threads;
  for (int i = 0; i < worker_thread_count; ++i) {
    worker_threads.emplace_back([&] {
      uWS::App app;

      auto uploadThread = uploadDocumentBackgroundProcess(&app, upload_task_queue);
      auto downloadThread = downloadDocumentBackgroundProcess(&app, download_task_queue);

      setup_rest(app, room_manager, file_provider, upload_task_queue, download_task_queue);
      setup_websocket(app, room_manager);
      setup_listen(app, settings);

      app.run();
    });
  }

  phantomchat::processors::uploadProcessorRunning = false;// Signal the upload and download processor to stop
}
