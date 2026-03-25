#include "handlers/RequestHandler.h"
#include <App.h>
#include <fmt/core.h>
#include <phantomchat/IFileProvider.hpp>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/services/RoomManager.h>

using namespace phantomchat::contracts;
using namespace phantomchat::services;

int main()
{
  fmt::print("Hello, {}...\n", "PhantomServer");

  // Create shared room manager
  auto &room_manager = RoomManager::getInstance();
  constexpr static int ListenPort = 8080;

  uWS::App app;

  app
    .get("/",
      [](auto *res, auto *req) {
        fmt::print("Received request for {}\n", req->getUrl());
        res->writeHeader("Content-Type", "text/html; charset=utf-8");

        FileProvider fileProvider;
        auto content = fileProvider.readAll("assets/index.html");

        res->end(content);
      })
    .ws<PerSocketData>("/room",
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