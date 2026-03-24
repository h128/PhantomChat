#pragma once
#include <WebSocketProtocol.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>


using json = nlohmann::json;
namespace phantomchat::events {

class EventBase
{
public:
  explicit EventBase(std::string event_name_) : event_name(std::move(event_name_)) {}
  virtual ~EventBase() = default;

  std::string event_name;
};

class RoomCreatedEvent final : public EventBase
{
public:
  explicit RoomCreatedEvent(std::string room_name_) : EventBase("RoomCreated"), room_name(std::move(room_name_)) {}

  std::string room_name;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomCreatedEvent, event_name, room_name)
};

class UserEnteredRoomEvent final : public EventBase
{
public:
  explicit UserEnteredRoomEvent(std::string room_name_, std::string user_uuid_)
    : EventBase("UserEnteredRoom"), room_name(std::move(room_name_)), user_uuid(std::move(user_uuid_))
  {}

  std::string room_name;
  std::string user_uuid;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(UserEnteredRoomEvent, event_name, room_name, user_uuid)
};

template<typename WS_TYPE, typename EventType>
void dispatch_event(WS_TYPE *ws, const EventType &event, const std::string &topic)
{
  json j = event;
  const std::string payload = j.dump();
  ws->publish(topic, payload, uWS::OpCode::TEXT);
}

}// namespace phantomchat::events