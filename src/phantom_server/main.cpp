#include "handlers/RequestHandler.h"
#include "handlers/StaticFileHandler.h"
#include <App.h>
#include <fmt/core.h>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/services/RoomManager.h>
#include <phantomchat/utils/CacheFileProvider.hpp>
#include <string_view>

int main()
{
  fmt::print("Hello, {}...\n", "PhantomServer");

  // Create shared room manager
  auto &room_manager = phantomchat::services::RoomManager::getInstance();
  constexpr static int ListenPort = 8080;

  phantomchat::utils::CacheFileProvider fileProvider("assets");

  uWS::App app;

  auto handleStaticFileWithCache = [&fileProvider](auto *res, auto *req) {
    phantomchat::handlers::handleStaticFile(res, req, fileProvider);
  };

  app.get("/*", handleStaticFileWithCache)
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
    .listen(ListenPort,
      [](auto *listenSocket) {
        if (listenSocket) {
          fmt::print("Server listening on http://localhost:{}\n", ListenPort);
        } else {
          fmt::print(stderr, "Failed to listen on port {}\n", ListenPort);
        }
      })
    .run();

  return 0;
}