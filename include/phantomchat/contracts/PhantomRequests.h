#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <phantomchat/phantom_core_export.hpp>
#include <string_view>

using json = nlohmann::json;

namespace phantomchat::contracts {

enum class Command { JoinOrCreateRoom = 1, SendMessage = 2, LeaveRoom = 3 };

// Base request class
class PHANTOM_CORE_EXPORT PhantomRequestBase
{
public:
  virtual ~PhantomRequestBase() = default;

  std::string request_uuid;
  Command command;


  virtual void validate() const = 0;
};

// JoinOrCreateRoom request
class PHANTOM_CORE_EXPORT JoinOrCreateRoomRequest final : public PhantomRequestBase
{
public:
  JoinOrCreateRoomRequest() { command = Command::JoinOrCreateRoom; }

  std::string user_uuid;
  std::string room_name;
  std::string public_key;

  void validate() const override;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(JoinOrCreateRoomRequest, request_uuid, user_uuid, command, room_name, public_key)
};

// SendMessage request
class PHANTOM_CORE_EXPORT SendMessageRequest final : public PhantomRequestBase
{
public:
  SendMessageRequest() { command = Command::SendMessage; }

  std::string message;

  void validate() const override;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(SendMessageRequest, request_uuid, command, message)
};

class PHANTOM_CORE_EXPORT LeaveRoomRequest final : public PhantomRequestBase
{
public:
  LeaveRoomRequest() { command = Command::LeaveRoom; }

  void validate() const override
  { /* No additional validation needed for leaving a room */
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(LeaveRoomRequest, request_uuid, command)
};

using PhantomRequestPtr = std::unique_ptr<PhantomRequestBase>;

PHANTOM_CORE_EXPORT PhantomRequestPtr from_json(std::string_view jsonString);

}// namespace phantomchat::contracts