#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/utils/HelperFunctions.h>
#include <phantomchat/utils/JsonSerialization.hpp>
#include <stdexcept>

namespace phantomchat::contracts {

using phantomchat::utils::is_safe;
using phantomchat::utils::is_valid_hex;
using phantomchat::utils::trim;

void JoinOrCreateRoomRequest::validate()
{
  trim(room_name);
  trim(display_name);

  if (user_uuid.size() > 64) { throw std::invalid_argument("user_uuid exceeds maximum length of 64"); }
  if (!is_safe(user_uuid)) {
    throw std::invalid_argument("user_uuid must contain only alphanumeric characters, hyphens, or underscores");
  }

  if (room_name.size() < 5) { throw std::invalid_argument("room_name must be at least 5 characters"); }
  if (room_name.size() > 64) { throw std::invalid_argument("room_name exceeds maximum length of 64"); }
  if (!is_safe(room_name)) {
    throw std::invalid_argument("room_name must contain only alphanumeric characters, hyphens, or underscores");
  }

  if (public_key.empty()) { throw std::invalid_argument("public_key is required"); }
  static constexpr std::size_t PUBLICKEYBYTES = 32;
  if (!is_valid_hex(public_key, PUBLICKEYBYTES)) {
    throw std::invalid_argument("public_key must be a valid 32-byte hex-encoded key");
  }

  if (display_name.size() > 64) { throw std::invalid_argument("display_name exceeds maximum length of 64"); }
}

void SendMessageRequest::validate()
{
  if (message.empty()) { throw std::invalid_argument("message cannot be empty"); }
  if (message.size() > 1024) { throw std::invalid_argument("message exceeds maximum length of 1024"); }
}

void SignalCallRequest::validate()
{
  // OFFER and ANSWER require a SessionDescription
  if (action == SignalCallAction::OFFER || action == SignalCallAction::ANSWER) {
    if (!std::holds_alternative<SessionDescription>(data)) {
      throw std::invalid_argument("OFFER/ANSWER actions require a SessionDescription");
    }
    const auto &sd = std::get<SessionDescription>(data);
    if (sd.sdp.empty()) { throw std::invalid_argument("SDP cannot be empty"); }
    if (sd.sdp.size() > 65536) { throw std::invalid_argument("SDP exceeds maximum length"); }
  }

  // CANDIDATE requires an IceCandidate
  if (action == SignalCallAction::CANDIDATE) {
    if (!std::holds_alternative<IceCandidate>(data)) {
      throw std::invalid_argument("CANDIDATE action requires an IceCandidate");
    }
  }
}

PhantomRequestPtr from_json(std::string_view json_string)
{
  json json_data = json::parse(json_string);

  if (!json_data.contains("command")) { throw std::invalid_argument("Missing 'command' field in JSON"); }

  auto cmd = static_cast<Command>(json_data["command"]);

  switch (cmd) {
  case Command::JoinOrCreateRoom: {
    auto request = std::make_unique<JoinOrCreateRoomRequest>();
    from_json(json_data, *request);
    return request;
  }
  case Command::SendMessage: {
    auto request = std::make_unique<SendMessageRequest>();
    from_json(json_data, *request);
    return request;
  }
  case Command::SignalCall: {
    auto request = std::make_unique<SignalCallRequest>();
    from_json(json_data, *request);
    return request;
  }
  case Command::LeaveRoom: {
    auto request = std::make_unique<LeaveRoomRequest>();
    from_json(json_data, *request);
    return request;
  }
  default:
    throw std::invalid_argument("Unknown command");
  }
}

}// namespace phantomchat::contracts