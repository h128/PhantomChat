#include "../headers/SocketRequestHandler.h"
#include <chrono>
#include <fmt/core.h>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/contracts/PhantomResponses.h>
#include <phantomchat/events/Events.h>
#include <phantomchat/services/CryptoRoom.h>
#include <phantomchat/utils/JsonSerialization.hpp>

namespace phantomchat::handlers {

using namespace phantomchat::services;
using namespace phantomchat::contracts;
using namespace phantomchat::events;
using namespace phantomchat::processors;

namespace {
  template<typename WS_TYPE, typename EventType>
  void dispatch_event(WS_TYPE *ws,
    moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
    const EventType &event,
    const std::string &topic)
  {
    // Publish event to websocket
    auto json_event = json(event).dump();
    ws->publish(topic, json_event, uWS::OpCode::TEXT);
    // Store event in chat history

    event_logger.enqueue({ .room_name = topic, .json_event = std::move(json_event) });
  }
}// anonymous namespace

template<typename APP_TYPE, typename WS_TYPE>
void handleSendMessage(APP_TYPE &,
  WS_TYPE *ws,
  const RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &push_notification_queue,
  const SendMessageRequest *request)
{
  const auto *session = get_session(ws);
  if (session->isEmpty()) { throw std::invalid_argument("User not in a room"); }
  ws->send(json(SendMessageResponse{ request->message, request->request_uuid }).dump(), uWS::OpCode::TEXT);

  const std::string &topic = session->room_name;
  const std::string &sender_uuid = session->user_uuid;
  dispatch_event(ws, event_logger, NewMessageReceivedEvent(sender_uuid, request->message), topic);

  const auto &fb = phantomchat::config::AppSettings::getInstance().firebase_settings;
  if (!fb.enabled) return;

  auto idle_members = room_manager.getIdleMembers(topic, std::chrono::seconds{ fb.min_push_interval_seconds });
  if (idle_members.empty()) return;
  push_notification_queue.enqueue({ //
    .room_name = topic,
    .recipients = std::move(idle_members),
    .title = fmt::format("New message in {}", topic),
    .body = request->message });
}

template<typename APP_TYPE, typename WS_TYPE>
void handleSetUserStatus(APP_TYPE &, WS_TYPE *ws, RoomManager &room_manager, const SetUserStatusRequest *request)
{
  const auto *session = get_session(ws);
  if (session->isEmpty()) throw std::invalid_argument("User not in a room");

  room_manager.setUserStatus({
    .room_name = session->room_name,
    .user_uuid = session->user_uuid,
    .status = request->status,
    .status_message = request->status_message,
  });

  ws->send(json(GeneralResponse{ "Status updated", request->request_uuid }).dump(), uWS::OpCode::TEXT);
}

template<typename APP_TYPE, typename WS_TYPE>
void handleLeaveRoom(APP_TYPE &app,
  WS_TYPE *ws,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  bool is_client_initiated_leave)
{
  auto *session = get_session(ws);
  if (session->isEmpty()) return;

  const std::string &topic = session->room_name;
  const std::string &user_uuid = session->user_uuid;

  auto result = room_manager.leaveRoom({ .room_name = topic, .user_uuid = user_uuid });


  if (!is_client_initiated_leave) {
    // underlying WebSocket connection closed, inform other clients
    app.publish(topic, json(LeaveRoomEvent{ user_uuid }).dump(), uWS::OpCode::TEXT);
  } else /* client requested to leave the room*/ {
    dispatch_event(ws, event_logger, LeaveRoomEvent{ user_uuid }, topic);
    ws->unsubscribe(topic);
    ws->send(json(GeneralResponse{ "Left room " + topic }).dump(), uWS::OpCode::TEXT);
  }

  if (result == RoomManager::LeaveRoomResult::RoomEmptyAndDeleted) {
    event_logger.enqueue({ .room_name = topic, .delete_room_on_empty = true });
  }
  session->clear();
}

template<typename APP_TYPE, typename WS_TYPE>
void handleJoinOrCreateRoom(APP_TYPE &,
  WS_TYPE *ws,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  const JoinOrCreateRoomRequest *request)
{
  auto *session = get_session(ws);

  if (!session->isEmpty()) {
    throw std::invalid_argument(
      "The user is already in another room; they should leave the current room before creating or joining a new one");
  }

  auto result = room_manager.joinOrCreateRoom({ .room_name = request->room_name,
    .user_uuid = request->user_uuid,
    .avatar_id = request->avatar_id,
    .display_name = request->display_name,
    .fcm_token = request->fcm_token });

  JoinOrCreateRoomResponse response{ //
    request->room_name,
    crypto_room::encryptRoomKey({
      .room_key = result.room_key,
      .user_public_key_hex = request->public_key,
      .server_secret_key = result.server_key_pair.secret_key//
    }),
    result.server_key_pair.public_key,
    result.room_created,
    result.members,
    request->request_uuid
  };

  // Update socket data
  session->assign(request->user_uuid, request->room_name, request->public_key);

  // Subscribe to room updates
  ws->subscribe(request->room_name);

  ws->send(json(response).dump(), uWS::OpCode::TEXT);

  // Dispatch events
  const std::string &topic = request->room_name;
  dispatch_event(ws,
    event_logger,
    UserEnteredRoomEvent(request->room_name, request->user_uuid, request->avatar_id, request->display_name),
    topic);
}

template<typename APP_TYPE, typename WS_TYPE>
void handleSignalCall(APP_TYPE &,
  WS_TYPE *ws,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  const SignalCallRequest *request)
{
  const auto *session = get_session(ws);
  if (session->isEmpty()) { throw std::invalid_argument("User not in a room"); }

  const std::string &topic = session->room_name;
  const std::string &sender_uuid = session->user_uuid;

  ws->send(
    json(GeneralResponse{ "Signal call dispatched successfully", request->request_uuid }).dump(), uWS::OpCode::TEXT);

  dispatch_event(ws, event_logger, SignalCallRelayEvent(request->action, sender_uuid, request->data), topic);
}

template<typename WS_TYPE> void sendError(WS_TYPE *ws, const std::string &message, const std::string &request_uuid)
{ ws->send(json(ErrorResponse{ message, request_uuid }).dump(), uWS::OpCode::TEXT); }

template<typename APP_TYPE, typename WS_TYPE>
void handleMessage(APP_TYPE &app,
  WS_TYPE *ws,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &push_notification_queue,
  std::string_view msg)
{
  PhantomRequestPtr request;
  try {
    request = from_json(msg);
    request->validate();

    switch (request->command) {
    case Command::JoinOrCreateRoom:
      handleJoinOrCreateRoom(app,//
        ws,
        room_manager,
        event_logger,
        static_cast<JoinOrCreateRoomRequest *>(request.get()));
      break;
    case Command::SendMessage:
      handleSendMessage(app,//
        ws,
        room_manager,
        event_logger,
        push_notification_queue,
        static_cast<SendMessageRequest *>(request.get()));
      break;
    case Command::SignalCall:
      handleSignalCall(app,//
        ws,
        event_logger,
        static_cast<SignalCallRequest *>(request.get()));
      break;
    case Command::LeaveRoom: {
      const bool is_client_initiated_leave = true;
      handleLeaveRoom(app,//
        ws,
        room_manager,
        event_logger,
        is_client_initiated_leave);
      break;
    }
    case Command::SetUserStatus:
      handleSetUserStatus(app,//
        ws,
        room_manager,
        static_cast<SetUserStatusRequest *>(request.get()));
      break;
    }
  } catch (const std::invalid_argument &e) {
    sendError(ws, e.what(), request ? request->request_uuid : "unknown");
  } catch (const std::exception &) {
    sendError(ws, "An unexpected error occurred", request ? request->request_uuid : "unknown");
  }
}

}// namespace phantomchat::handlers

using WsType = uWS::WebSocket<false, true, phantomchat::contracts::PerSocketData>;

template void phantomchat::handlers::handleSendMessage<uWS::App, WsType>(uWS::App &,
  WsType *,
  const phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &,
  const phantomchat::contracts::SendMessageRequest *);

template void phantomchat::handlers::handleSetUserStatus<uWS::App, WsType>(uWS::App &,
  WsType *,
  phantomchat::services::RoomManager &,
  const phantomchat::contracts::SetUserStatusRequest *);

template void phantomchat::handlers::handleLeaveRoom<uWS::App, WsType>(uWS::App &,
  WsType *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  bool);

template void phantomchat::handlers::handleJoinOrCreateRoom<uWS::App, WsType>(uWS::App &,
  WsType *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  const phantomchat::contracts::JoinOrCreateRoomRequest *);

template void phantomchat::handlers::sendError<WsType>(WsType *, const std::string &, const std::string &);

template void phantomchat::handlers::handleSignalCall<uWS::App, WsType>(uWS::App &,
  WsType *,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  const phantomchat::contracts::SignalCallRequest *);

template void phantomchat::handlers::handleMessage<uWS::App, WsType>(uWS::App &,
  WsType *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &,
  std::string_view);

using SslWsType = uWS::WebSocket<true, true, phantomchat::contracts::PerSocketData>;

template void phantomchat::handlers::handleSendMessage<uWS::SSLApp, SslWsType>(uWS::SSLApp &,
  SslWsType *,
  const phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &,
  const phantomchat::contracts::SendMessageRequest *);

template void phantomchat::handlers::handleSetUserStatus<uWS::SSLApp, SslWsType>(uWS::SSLApp &,
  SslWsType *,
  phantomchat::services::RoomManager &,
  const phantomchat::contracts::SetUserStatusRequest *);

template void phantomchat::handlers::handleLeaveRoom<uWS::SSLApp, SslWsType>(uWS::SSLApp &,
  SslWsType *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  bool);


template void phantomchat::handlers::handleJoinOrCreateRoom<uWS::SSLApp, SslWsType>(uWS::SSLApp &,
  SslWsType *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  const phantomchat::contracts::JoinOrCreateRoomRequest *);

template void phantomchat::handlers::sendError<SslWsType>(SslWsType *, const std::string &, const std::string &);

template void phantomchat::handlers::handleSignalCall<uWS::SSLApp, SslWsType>(uWS::SSLApp &,
  SslWsType *,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  const phantomchat::contracts::SignalCallRequest *);

template void phantomchat::handlers::handleMessage<uWS::SSLApp, SslWsType>(uWS::SSLApp &,
  SslWsType *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &,
  std::string_view);
