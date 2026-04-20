#pragma once

#include "DocumentProcessor.h"
#include "PushNotificationProcessor.h"
#include <App.h>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/services/RoomManager.h>
#include <string>
#include <string_view>

namespace phantomchat::handlers {

template<typename APP_TYPE, typename WS_TYPE>
void handleSendMessage(APP_TYPE &app,
  WS_TYPE *ws,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &event_logger,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::PushNotificationTask> &push_notification_queue,
  const phantomchat::contracts::SendMessageRequest *request);

template<typename APP_TYPE, typename WS_TYPE>
void handleSetUserStatus(APP_TYPE &app,
  WS_TYPE *ws,
  phantomchat::services::RoomManager &room_manager,
  const phantomchat::contracts::SetUserStatusRequest *request);

template<typename APP_TYPE, typename WS_TYPE>
void handleLeaveRoom(APP_TYPE &app,
  WS_TYPE *ws,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &event_logger,
  bool is_client_initiated_leave);

template<typename APP_TYPE, typename WS_TYPE>
void handleJoinOrCreateRoom(APP_TYPE &app,
  WS_TYPE *ws,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &event_logger,
  const phantomchat::contracts::JoinOrCreateRoomRequest *request);

template<typename APP_TYPE, typename WS_TYPE>
void handleSignalCall(APP_TYPE &app,
  WS_TYPE *ws,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &event_logger,
  const phantomchat::contracts::SignalCallRequest *request);

template<typename WS_TYPE> void sendError(WS_TYPE *ws, const std::string &message, const std::string &request_uuid);

template<typename APP_TYPE, typename WS_TYPE>
void handleMessage(APP_TYPE &app,
  WS_TYPE *ws,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &event_logger,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::PushNotificationTask> &push_notification_queue,
  std::string_view msg);

}// namespace phantomchat::handlers
