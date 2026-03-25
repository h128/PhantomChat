#include "contracts/PerSocketData.h"
#include "contracts/PhantomRequests.h"
#include "handlers/RequestHandler.h"
#include "services/RoomManager.h"
#include <App.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <phantomchat/IFileProvider.hpp>

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
            auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
            if (!socket_data->room_name.empty() && !socket_data->user_uuid.empty()) {
              nlohmann::json j = phantomchat::events::LeaveRoomEvent(socket_data->user_uuid);
              app.publish(socket_data->room_name, j.dump(), uWS::OpCode::TEXT);
            }
            phantomchat::handlers::handleLeaveRoom(ws, room_manager);
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