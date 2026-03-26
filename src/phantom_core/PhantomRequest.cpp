#include <phantomchat/JsonMapper.hpp>
#include <phantomchat/contracts/PhantomRequests.h>
#include <stdexcept>

namespace phantomchat::contracts {

void JoinOrCreateRoomRequest::validate() const
{
  if (user_uuid.empty()) { throw std::invalid_argument("user_uuid cannot be empty"); }
  if (public_key.empty()) { throw std::invalid_argument("public_key cannot be empty"); }
  if (room_name.empty()) { throw std::invalid_argument("room_name cannot be empty"); }
}

void SendMessageRequest::validate() const
{
  if (message.empty()) { throw std::invalid_argument("message cannot be empty"); }
}

PhantomRequestPtr from_json(std::string_view jsonString)
{
  json jsonData = json::parse(jsonString);

  if (!jsonData.contains("command")) { throw std::invalid_argument("Missing 'command' field in JSON"); }

  auto cmd = static_cast<Command>(jsonData["command"]);

  switch (cmd) {
  case Command::JoinOrCreateRoom: {
    auto request = std::make_unique<JoinOrCreateRoomRequest>(jsonData.get<JoinOrCreateRoomRequest>());
    request->validate();
    return request;
  }
  case Command::SendMessage: {
    auto request = std::make_unique<SendMessageRequest>(jsonData.get<SendMessageRequest>());
    request->validate();
    return request;
  }
  case Command::LeaveRoom: {
    auto request = std::make_unique<LeaveRoomRequest>(jsonData.get<LeaveRoomRequest>());
    return request;
  }
  default:
    throw std::invalid_argument("Unknown command");
  }
}

}// namespace phantomchat::contracts