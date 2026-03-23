#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace phantomchat::contracts {

enum class Command { JoinOrCreateRoom = 1, SendMessage = 2 };

// Base request class
class PhantomRequestBase
{
public:
  virtual ~PhantomRequestBase() = default;

  std::string request_uuid;
  std::string user_uuid;
  Command command;


  virtual void validate() const = 0;
};

// JoinOrCreateRoom request
class JoinOrCreateRoomRequest final : public PhantomRequestBase
{
public:
  JoinOrCreateRoomRequest() { command = Command::JoinOrCreateRoom; }

  std::string room_name;
  std::string public_key;

  void validate() const override;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(JoinOrCreateRoomRequest, request_uuid, user_uuid, command, room_name, public_key)
};

// SendMessage request
class SendMessageRequest final : public PhantomRequestBase
{
public:
  SendMessageRequest() { command = Command::SendMessage; }

  std::string room_name;
  std::string message;

  void validate() const override;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(SendMessageRequest, request_uuid, user_uuid, command, room_name, message)
};

using PhantomRequestPtr = std::unique_ptr<PhantomRequestBase>;

PhantomRequestPtr from_json(const std::string &jsonString);

}// namespace phantomchat::contracts