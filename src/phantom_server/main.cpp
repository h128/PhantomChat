#include "handlers/DocumentHandler.h"
#include "handlers/SocketRequestHandler.h"
#include "handlers/StaticFileHandler.h"
#include <App.h>
#include <fmt/core.h>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/services/RoomManager.h>
#include <phantomchat/utils/CacheFileProvider.hpp>
#include <sodium.h>
#include <string_view>

int main()
{
  if (sodium_init() == -1) { throw std::runtime_error("Failed to initialize libsodium"); }

  fmt::print("Hello, {}...\n", "PhantomServer");

  phantomchat::config::AppSettings settings;
  settings.load_from_file("appsettings.json");

  auto &room_manager = phantomchat::services::RoomManager::getInstance();

  phantomchat::utils::CacheFileProvider fileProvider("assets");


  uWS::App app;

  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask> uploadTaskQueue;
  phantomchat::processors::uploadDocumentBackgroundProcess(&app, uploadTaskQueue);

  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask> downloadTaskQueue;
  phantomchat::processors::downloadDocumentBackgroundProcess(&app, downloadTaskQueue);

  auto handleStaticFileWithCache = [&fileProvider](auto *res, auto *req) {
    phantomchat::handlers::handleStaticFile(res, req, fileProvider);
  };
  auto handleUploadDocument = [&room_manager, &uploadTaskQueue](auto *res, auto *req) {
    phantomchat::handlers::handleUploadDocument(res, req, room_manager, uploadTaskQueue);
  };

  auto handleDownloadDocument = [&downloadTaskQueue](auto *res, auto *req) {
    phantomchat::handlers::handleDownloadDocument(res, req, downloadTaskQueue);
  };

  app.get("/download-document/:room/:filename", handleDownloadDocument)
    .post("/upload-document/:filename", handleUploadDocument)
    .get("/*", handleStaticFileWithCache)
    .ws<phantomchat::contracts::PerSocketData>("/room",
      { .open = [](auto *) {},
        .message = [&room_manager](auto *ws,
                     std::string_view msg,
                     uWS::OpCode) { phantomchat::handlers::handleMessage(ws, room_manager, msg); },
        .close = [&room_manager, &app](auto *ws,
                   int,
                   std::string_view) { phantomchat::handlers::handleLeaveRoom(ws, room_manager, &app); } })
    .listen(settings.listen_port,
      [&settings](auto *listenSocket) {
        if (listenSocket) {
          fmt::print("Server listening on {}\n", settings.listen_port);
        } else {
          fmt::print(stderr, "Failed to listen on {}\n", settings.listen_port);
        }
      })
    .run();

  phantomchat::processors::uploadProcessorRunning = false;// Signal the upload and download processor to stop

  return 0;
}
