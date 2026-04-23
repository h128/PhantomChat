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
    const auto json_event = json(event).dump();
    ws->publish(topic, json_event, uWS::OpCode::TEXT);
    // Store event in chat history

    event_logger.enqueue({ .room_name = topic, .json_event = json_event });
  }
}// anonymous namespace

template<typename APP_TYPE, typename WS_TYPE>
void handleSendMessage(APP_TYPE &,
  WS_TYPE *ws,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  moodycamel::BlockingConcurrentQueue<PushNotificationTask> &push_notification_queue,
  const SendMessageRequest *request)
{
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
  if (socket_data->isEmpty()) { throw std::invalid_argument("User not in a room"); }
  SendMessageResponse response;
  response.request_uuid = request->request_uuid;
  response.message = request->message;

  ws->send(json(response).dump(), uWS::OpCode::TEXT);

  const std::string &topic = socket_data->room_name;
  const std::string &sender_uuid = socket_data->user_uuid;
  dispatch_event(ws, event_logger, NewMessageReceivedEvent(sender_uuid, request->message), topic);

  const auto &fb = phantomchat::config::AppSettings::getInstance().firebase_settings;
  if (fb.enabled) {
    auto idle_members = room_manager.getIdleMembers(topic, std::chrono::seconds{ fb.min_push_interval_seconds });
    if (!idle_members.empty()) {
      push_notification_queue.enqueue({
        .room_name = topic,
        .recipients = std::move(idle_members),
        .title = fmt::format("New message in {}", topic),
        .body = fmt::format("{} sent a message", sender_uuid),
        .icon = "https://fantom.chat/comment.png"//
      });
    }
  }
}

template<typename APP_TYPE, typename WS_TYPE>
void handleSetUserStatus(APP_TYPE &, WS_TYPE *ws, RoomManager &room_manager, const SetUserStatusRequest *request)
{
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
  if (socket_data->isEmpty()) throw std::invalid_argument("User not in a room");

  room_manager.setUserStatus({
    .room_name = socket_data->room_name,
    .user_uuid = socket_data->user_uuid,
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
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
  if (socket_data->isEmpty()) return;

  const std::string &topic = socket_data->room_name;
  const std::string &user_uuid = socket_data->user_uuid;

  auto result = room_manager.leaveRoom({ .room_name = topic, .user_uuid = user_uuid });

  LeaveRoomEvent leave_room_event(user_uuid);

  if (!is_client_initiated_leave) {
    // underlying WebSocket connection closed, inform other clients
    app.publish(topic, json(leave_room_event).dump(), uWS::OpCode::TEXT);
  } else /* client requested to leave the room*/ {
    dispatch_event(ws, event_logger, leave_room_event, topic);
    ws->unsubscribe(topic);
    ws->send(json(GeneralResponse{ "Left room " + topic }).dump(), uWS::OpCode::TEXT);
  }

  if (result == RoomManager::LeaveRoomResult::RoomEmptyAndDeleted) {
    event_logger.enqueue({ .room_name = topic, .delete_room_on_empty = true });
  }
  socket_data->clear();
}

template<typename APP_TYPE, typename WS_TYPE>
void handleJoinOrCreateRoom(APP_TYPE &,
  WS_TYPE *ws,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  const JoinOrCreateRoomRequest *request)
{
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());

  if (!socket_data->isEmpty()) {
    throw std::invalid_argument(
      "The user is already in another room; they should leave the current room before creating or joining a new one");
  }

  auto result = room_manager.joinOrCreateRoom({ .room_name = request->room_name,
    .user_uuid = request->user_uuid,
    .avatar_id = request->avatar_id,
    .display_name = request->display_name,
    .fcm_token = request->fcm_token });

  JoinOrCreateRoomResponse response;
  response.request_uuid = request->request_uuid;
  response.room_name = request->room_name;

  response.room_key = crypto_room::encryptRoomKey({ //
    .room_key = result.room_key,
    .user_public_key_hex = request->public_key,
    .server_secret_key = result.server_key_pair.secret_key });

  response.server_pub_key = result.server_key_pair.public_key;
  response.room_created = result.room_created;
  response.members = result.members;
  response.message = result.room_created ? "Room created successfully" : "Joined room successfully";

  // Update socket data
  socket_data->assign(request->user_uuid, request->room_name, request->public_key);

  // Subscribe to room updates
  ws->subscribe(request->room_name);

  ws->send(json(response).dump(), uWS::OpCode::TEXT);

  // Dispatch events
  const std::string &topic = request->room_name;
  if (result.room_created) { dispatch_event(ws, event_logger, RoomCreatedEvent(request->room_name), topic); }
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
  auto *socket_data = static_cast<PerSocketData *>(ws->getUserData());
  if (socket_data->isEmpty()) { throw std::invalid_argument("User not in a room"); }

  const std::string &topic = socket_data->room_name;
  const std::string &sender_uuid = socket_data->user_uuid;

  GeneralResponse resp("Signal call dispatched successfully", request->request_uuid);
  ws->send(json(resp).dump(), uWS::OpCode::TEXT);

  dispatch_event(ws, event_logger, SignalCallRelayEvent(request->action, sender_uuid, request->data), topic);
}

template<typename WS_TYPE> void sendError(WS_TYPE *ws, const std::string &message, const std::string &request_uuid)
{
  ErrorResponse error(message);
  error.request_uuid = request_uuid;
  ws->send(json(error).dump(), uWS::OpCode::TEXT);
}

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
  phantomchat::services::RoomManager &,
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
  phantomchat::services::RoomManager &,
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
