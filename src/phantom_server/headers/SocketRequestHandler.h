#pragma once

#include <App.h>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/services/RoomManager.h>
#include <string>
#include <string_view>

namespace phantomchat::handlers {

template<typename WS_TYPE>
void handleSendMessage(WS_TYPE *ws, const phantomchat::contracts::SendMessageRequest *request);

template<typename WS_TYPE, typename APP_TYPE>
void handleLeaveRoom(WS_TYPE *ws, phantomchat::services::RoomManager &room_manager, APP_TYPE *app);

template<typename WS_TYPE>
void handleJoinOrCreateRoom(WS_TYPE *ws,
  phantomchat::services::RoomManager &room_manager,
  const phantomchat::contracts::JoinOrCreateRoomRequest *request);

template<typename WS_TYPE> void handleSignalCall(WS_TYPE *ws, const phantomchat::contracts::SignalCallRequest *request);

template<typename WS_TYPE> void sendError(WS_TYPE *ws, const std::string &message, const std::string &request_uuid);

template<typename WS_TYPE>
void handleMessage(WS_TYPE *ws, phantomchat::services::RoomManager &room_manager, std::string_view msg);

}// namespace phantomchat::handlers
