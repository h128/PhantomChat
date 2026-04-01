#pragma once
#include <memory>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <string_view>
#include <utility>

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
};

class PHANTOM_CORE_EXPORT UserEnteredRoomEvent final : public EventBase
{
public:
  explicit UserEnteredRoomEvent(std::string room_name_, std::string user_uuid_)
    : EventBase("UserEnteredRoom"), room_name(std::move(room_name_)), user_uuid(std::move(user_uuid_))
  {}

  std::string room_name;
  std::string user_uuid;
};

class PHANTOM_CORE_EXPORT NewMessageReceivedEvent final : public EventBase
{
public:
  explicit NewMessageReceivedEvent(std::string sender_uuid_, std::string message_)
    : EventBase("NewMessageReceived"), sender_uuid(std::move(sender_uuid_)), message(std::move(message_))
  {}

  std::string sender_uuid;
  std::string message;
};

class PHANTOM_CORE_EXPORT LeaveRoomEvent final : public EventBase
{
public:
  explicit LeaveRoomEvent(std::string user_uuid_) : EventBase("LeaveRoom"), user_uuid(std::move(user_uuid_)) {}

  std::string user_uuid;
};

class PHANTOM_CORE_EXPORT FileUploadedEvent final : public EventBase
{
public:
  explicit FileUploadedEvent(std::string file_name_, std::string room_name_, std::string user_uuid_, bool poster_)
    : EventBase("FileUploaded"), file_name(std::move(file_name_)), room_name(std::move(room_name_)),
      user_uuid(std::move(user_uuid_)), poster(poster_)
  {}

  std::string file_name;
  std::string room_name;
  std::string user_uuid;
  bool poster = false;// for the thumbnail
};

class PHANTOM_CORE_EXPORT SignalCallRelayEvent final : public EventBase
{
public:
  explicit SignalCallRelayEvent(contracts::SignalCallAction action_,
    std::string sender_uuid_,
    contracts::SignalingData signaling_data_)
    : EventBase("SignalCallRelay"), action(action_), sender_uuid(std::move(sender_uuid_)),
      signaling_data(std::move(signaling_data_))
  {}

  contracts::SignalCallAction action;
  std::string sender_uuid;
  contracts::SignalingData signaling_data;
};


}// namespace phantomchat::events