#pragma once

#include <memory>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <string_view>

namespace phantomchat::contracts {

enum class Command { JoinOrCreateRoom = 1, SendMessage = 2, LeaveRoom = 3 };

// Base request class
class PHANTOM_CORE_EXPORT PhantomRequestBase
{
public:
  virtual ~PhantomRequestBase() = default;

  std::string request_uuid;
  Command command;


  virtual void validate() = 0;
};

// JoinOrCreateRoom request
class PHANTOM_CORE_EXPORT JoinOrCreateRoomRequest final : public PhantomRequestBase
{
public:
  JoinOrCreateRoomRequest() { command = Command::JoinOrCreateRoom; }

  std::string user_uuid;
  std::string room_name;
  std::string public_key;

  void validate() override;
};

// SendMessage request
class PHANTOM_CORE_EXPORT SendMessageRequest final : public PhantomRequestBase
{
public:
  SendMessageRequest() { command = Command::SendMessage; }

  std::string message;

  void validate() override;
};

class PHANTOM_CORE_EXPORT LeaveRoomRequest final : public PhantomRequestBase
{
public:
  LeaveRoomRequest() { command = Command::LeaveRoom; }

  void validate() override
  { /* No additional validation needed for leaving a room */
  }
};

using PhantomRequestPtr = std::unique_ptr<PhantomRequestBase>;

PHANTOM_CORE_EXPORT PhantomRequestPtr from_json(std::string_view jsonString);

}// namespace phantomchat::contracts