#pragma once

#include "../contracts/PerSocketData.h"
#include "../contracts/PhantomRequests.h"
#include "../contracts/PhantomResponses.h"
#include "../events/Events.h"
#include "../services/RoomManager.h"
#include <App.h>
#include <memory>

namespace phantomchat::handlers {

using namespace phantomchat::services;
using namespace phantomchat::contracts;
using namespace phantomchat::events;

template<typename WS_TYPE>
void handleJoinOrCreateRoom(WS_TYPE *ws,
  RoomManager &room_manager,
  const JoinOrCreateRoomRequest *request,
  PerSocketData *socket_data)
{

  auto result = room_manager.joinOrCreateRoom({ .room_name = request->room_name, .user_uuid = request->user_uuid });

  JoinOrCreateRoomResponse response;
  response.request_uuid = request->request_uuid;
  response.room_name = request->room_name;


  response.status = ResponseStatus::Success;
  response.room_key = result.room_key;
  response.room_created = result.room_created;
  response.message = result.room_created ? "Room created successfully" : "Joined room successfully";
  auto roomOpt = room_manager.getRoom(request->room_name);
  if (roomOpt) {
    const Room &room = roomOpt->get();
    response.members = room.members;
  }

  // Update socket data
  socket_data->user_uuid = request->user_uuid;
  socket_data->room_name = request->room_name;
  socket_data->public_key = request->public_key;

  // Subscribe to room updates
  ws->subscribe(request->room_name);

  ws->send(response.to_json().dump(), uWS::OpCode::TEXT);

  // Dispatch events
  const std::string topic = request->room_name;
  if (result.room_created) { dispatch_event(ws, RoomCreatedEvent(request->room_name), topic); }
  dispatch_event(ws, UserEnteredRoomEvent(request->room_name, request->user_uuid), topic);
}


template<typename WS_TYPE>
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void handleMessage(WS_TYPE *ws, RoomManager &room_manager, std::string_view msg)
{
  try {
    auto request = from_json(std::string(msg));
    auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());

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
    ws->send(error.to_json().dump(), uWS::OpCode::TEXT);
  }
}


}// namespace phantomchat::handlers
