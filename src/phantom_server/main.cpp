#include "contracts/PerSocketData.h"
#include "contracts/PhantomRequests.h"
#include "handlers/RequestHandler.h"
#include "services/RoomManager.h"
#include <App.h>
#include <fmt/core.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <phantomchat/IFileProvider.hpp>

using namespace phantomchat::contracts;
int main()
{
  fmt::print("Hello, {}...\n", "PhantomServer");

  // Create shared room manager
  auto &room_manager = phantomchat::services::RoomManager::getInstance();

  uWS::App()
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
            fmt::print("Received message: {}\n", msg);
            phantomchat::handlers::handleMessage(ws, room_manager, msg);
          },
        .close =
          [&room_manager](auto *ws, int, std::string_view) {
            auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
            if (!socket_data->room_name.empty() && !socket_data->user_uuid.empty()) {
              room_manager.leaveRoom({ .room_name = socket_data->room_name, .user_uuid = socket_data->user_uuid });
            }
            fmt::print("WebSocket disconnected\n");
          } })
    .listen(8080,
      [](auto *listenSocket) {
        if (listenSocket) {
          std::cout << "Server listening on http://localhost:8080" << std::endl;
        } else {
          std::cerr << "Failed to listen on port 8080" << std::endl;
        }
      })
    .run();

  return 0;
}