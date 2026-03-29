#include "handlers/DocumentUploadHandler.h"
#include "handlers/RequestHandler.h"
#include "handlers/StaticFileHandler.h"
#include "processors/DocumentFileUploadProcessor.h"
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

  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask> uploadTaskQueue;
  phantomchat::processors::uploadDocumentBackgroundProcess(uploadTaskQueue);

  uWS::App app;

  auto handleStaticFileWithCache = [&fileProvider](auto *res, auto *req) {
    phantomchat::handlers::handleStaticFile(res, req, fileProvider);
  };
  auto handleDocumentUpload = [&room_manager, &uploadTaskQueue](auto *res, auto *req) {
    phantomchat::handlers::handleDocumentUpload(res, req, room_manager, uploadTaskQueue);
  };

  app.get("/*", handleStaticFileWithCache)
    .post("/document-upload/:filename", handleDocumentUpload)
    .ws<phantomchat::contracts::PerSocketData>("/room",
      { .open = [](auto *) { fmt::print("WebSocket connected\n"); },
        .message =
          [&room_manager](auto *ws, std::string_view msg, uWS::OpCode) {
            fmt::print("new message received!\n");
            phantomchat::handlers::handleMessage(ws, room_manager, msg);
          },
        .close =
          [&room_manager, &app](auto *ws, int, std::string_view) {
            fmt::print("WebSocket closed\n");
            phantomchat::handlers::handleLeaveRoom(ws, room_manager, &app);
          } })
    .listen(settings.listen_port,
      [&settings](auto *listenSocket) {
        if (listenSocket) {
          fmt::print("Server listening on http://localhost:{}\n", settings.listen_port);
        } else {
          fmt::print(stderr, "Failed to listen on port {}\n", settings.listen_port);
        }
      })
    .run();

  phantomchat::processors::uploadProcessorRunning = false;// Signal the upload processor to stop

  return 0;
}
