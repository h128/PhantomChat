#pragma once

#include "../contracts/PerSocketData.h"
#include "../contracts/PhantomRequests.h"
#include "../contracts/PhantomResponses.h"
#include "../services/RoomManager.h"
#include <App.h>
#include <memory>

namespace phantomchat::handlers {
using namespace phantomchat::services;
using namespace phantomchat::contracts;

template<typename WS_TYPE>
void handleJoinOrCreateRoom(WS_TYPE *ws,
  std::shared_ptr<RoomManager> room_manager,
  JoinOrCreateRoomRequest *request,
  PerSocketData *socket_data)
{

  auto result = room_manager->joinOrCreateRoom(*request);

  JoinOrCreateRoomResponse response;
  response.request_uuid = request->request_uuid;
  response.room_name = request->room_name;

  if (result.success) {
    response.status = ResponseStatus::Success;
    response.room_key = result.room_key;
    response.room_created = result.room_created;
    response.message = result.room_created ? "Room created successfully" : "Joined room successfully";

    // Update socket data
    socket_data->user_uuid = request->user_uuid;
    socket_data->room_name = request->room_name;
    socket_data->public_key = request->public_key;

  } else {
    response.status = ResponseStatus::Error;
    response.message = result.error_message;
    response.room_created = false;
  }

  ws->send(response.to_json().dump());
}


template<typename WS_TYPE>
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void handleMessage(WS_TYPE *ws,
  std::shared_ptr<RoomManager> room_manager,
  std::string_view msg,
  PerSocketData *socket_data)
{
  using namespace phantomchat::contracts;

  try {
    auto request = from_json(std::string(msg));

    switch (request->command) {
    case Command::JoinOrCreateRoom:
      handleJoinOrCreateRoom(ws, room_manager, dynamic_cast<JoinOrCreateRoomRequest *>(request.get()), socket_data);
      break;
    case Command::SendMessage:
      // TODO: Implement send message handler
      break;
    }
  } catch (const std::exception &e) {
    ErrorResponse error(e.what());
    error.request_uuid = "unknown";
    ws->send(error.to_json().dump());
  }
}


}// namespace phantomchat::handlers
