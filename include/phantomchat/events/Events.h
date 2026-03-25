#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <string_view>


using json = nlohmann::json;
namespace phantomchat::events {

class PHANTOM_CORE_EXPORT EventBase
{
public:
  explicit EventBase(std::string_view event_name_) : event_name(event_name_) {}
  virtual ~EventBase() = default;

  std::string_view event_name;
};

class PHANTOM_CORE_EXPORT RoomCreatedEvent final : public EventBase
{
public:
  explicit RoomCreatedEvent(std::string room_name_) : EventBase("RoomCreated"), room_name(std::move(room_name_)) {}

  std::string room_name;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomCreatedEvent, event_name, room_name)
};

class PHANTOM_CORE_EXPORT UserEnteredRoomEvent final : public EventBase
{
public:
  explicit UserEnteredRoomEvent(std::string room_name_, std::string user_uuid_)
    : EventBase("UserEnteredRoom"), room_name(std::move(room_name_)), user_uuid(std::move(user_uuid_))
  {}

  std::string room_name;
  std::string user_uuid;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(UserEnteredRoomEvent, event_name, room_name, user_uuid)
};

class PHANTOM_CORE_EXPORT NewMessageReceivedEvent final : public EventBase
{
public:
  explicit NewMessageReceivedEvent(std::string sender_uuid_, std::string message_)
    : EventBase("NewMessageReceived"), sender_uuid(std::move(sender_uuid_)), message(std::move(message_))
  {}

  std::string sender_uuid;
  std::string message;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(NewMessageReceivedEvent, event_name, sender_uuid, message)
};

class PHANTOM_CORE_EXPORT LeaveRoomEvent final : public EventBase
{
public:
  explicit LeaveRoomEvent(std::string user_uuid_) : EventBase("LeaveRoom"), user_uuid(std::move(user_uuid_)) {}

  std::string user_uuid;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(LeaveRoomEvent, event_name, user_uuid)
};


}// namespace phantomchat::events