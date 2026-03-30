#include <phantomchat/JsonMapper.hpp>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/utils/HelperFunctions.h>
#include <stdexcept>

namespace phantomchat::contracts {

using phantomchat::utils::is_safe;
using phantomchat::utils::trim;

void JoinOrCreateRoomRequest::validate()
{
  trim(user_uuid);
  trim(room_name);
  trim(public_key);

  if (user_uuid.size() > 64) { throw std::invalid_argument("user_uuid exceeds maximum length of 64"); }
  if (!is_safe(user_uuid)) {
    throw std::invalid_argument("user_uuid must contain only alphanumeric characters, hyphens, or underscores");
  }

  if (room_name.size() < 5) { throw std::invalid_argument("room_name must be at least 5 characters"); }
  if (room_name.size() > 64) { throw std::invalid_argument("room_name exceeds maximum length of 64"); }
  if (!is_safe(room_name)) {
    throw std::invalid_argument("room_name must contain only alphanumeric characters, hyphens, or underscores");
  }

  if (public_key.size() > 512) { throw std::invalid_argument("public_key exceeds maximum length of 512"); }
  if (!is_safe(public_key)) {
    throw std::invalid_argument("public_key must contain only alphanumeric characters, hyphens, or underscores");
  }
}

void SendMessageRequest::validate()
{
  trim(message);

  if (message.empty()) { throw std::invalid_argument("message cannot be empty"); }
  if (message.size() > 1024) { throw std::invalid_argument("message exceeds maximum length of 1024"); }
}

PhantomRequestPtr from_json(std::string_view jsonString)
{
  json jsonData = json::parse(jsonString);

  if (!jsonData.contains("command")) { throw std::invalid_argument("Missing 'command' field in JSON"); }

  auto cmd = static_cast<Command>(jsonData["command"]);

  switch (cmd) {
  case Command::JoinOrCreateRoom: {
    auto request = std::make_unique<JoinOrCreateRoomRequest>(jsonData.get<JoinOrCreateRoomRequest>());
    return request;
  }
  case Command::SendMessage: {
    auto request = std::make_unique<SendMessageRequest>(jsonData.get<SendMessageRequest>());
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