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

template<typename WS_TYPE> void handleSendMessage(WS_TYPE *ws, const SendMessageRequest *request)
{
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
  if (socket_data->room_name.empty() || socket_data->user_uuid.empty()) {
    throw std::invalid_argument("User not in a room");
  }
  SendMessageResponse response;
  response.request_uuid = request->request_uuid;
  response.message = request->message;

  ws->send(response.to_json().dump(), uWS::OpCode::TEXT);

  const std::string topic = socket_data->room_name;
  const std::string sender_uuid = socket_data->user_uuid;
  dispatch_event(ws, NewMessageReceivedEvent(sender_uuid, request->message), topic);
}

template<typename WS_TYPE> void handleLeaveRoom(WS_TYPE *ws, RoomManager &room_manager)
{
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
  if (!socket_data->room_name.empty() && !socket_data->user_uuid.empty()) {
    const std::string topic = socket_data->room_name;
    const std::string user_uuid = socket_data->user_uuid;

    room_manager.leaveRoom({ .room_name = topic, .user_uuid = user_uuid });

    dispatch_event(ws, LeaveRoomEvent(user_uuid), topic);

    socket_data->clear();
  }
}


template<typename WS_TYPE>
void handleJoinOrCreateRoom(WS_TYPE *ws, RoomManager &room_manager, const JoinOrCreateRoomRequest *request)
{
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());

  auto result = room_manager.joinOrCreateRoom({ .room_name = request->room_name, .user_uuid = request->user_uuid });

  JoinOrCreateRoomResponse response;
  response.request_uuid = request->request_uuid;
  response.room_name = request->room_name;

  response.room_key = result.room_key;
  response.room_created = result.room_created;
  response.message = result.room_created ? "Room created successfully" : "Joined room successfully";

  auto roomOpt = room_manager.getRoom(request->room_name);
  if (roomOpt) {
    const Room &room = roomOpt->get();
    response.members = room.members;
  }

  // Update socket data
  socket_data->assign(request->user_uuid, request->room_name, request->public_key);

  // Subscribe to room updates
  ws->subscribe(request->room_name);

  ws->send(response.to_json().dump(), uWS::OpCode::TEXT);

  // Dispatch events
  const std::string topic = request->room_name;
  if (result.room_created) { dispatch_event(ws, RoomCreatedEvent(request->room_name), topic); }
  dispatch_event(ws, UserEnteredRoomEvent(request->room_name, request->user_uuid), topic);
}


template<typename WS_TYPE> void sendError(WS_TYPE *ws, const std::string &message, const std::string &request_uuid)
{
  ErrorResponse error(message);
  error.request_uuid = request_uuid;
  ws->send(error.to_json().dump(), uWS::OpCode::TEXT);
}


template<typename WS_TYPE> void handleMessage(WS_TYPE *ws, RoomManager &room_manager, std::string_view msg)
{
  PhantomRequestPtr request;
  try {
    request = from_json(msg);

    switch (request->command) {
    case Command::JoinOrCreateRoom:
      handleJoinOrCreateRoom(ws, room_manager, dynamic_cast<JoinOrCreateRoomRequest *>(request.get()));
      break;
    case Command::SendMessage:
      handleSendMessage(ws, dynamic_cast<SendMessageRequest *>(request.get()));
      break;
    case Command::LeaveRoom:
      handleLeaveRoom(ws, room_manager);
      break;
    }
  } catch (const std::invalid_argument &e) {
    sendError(ws, e.what(), request ? request->request_uuid : "unknown");
  } catch (const std::exception &) {
    sendError(ws, "An unexpected error occurred", request ? request->request_uuid : "unknown");
  }
}


}// namespace phantomchat::handlers
