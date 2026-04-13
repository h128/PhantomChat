#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <string_view>
#include <variant>

namespace phantomchat::contracts {

enum class Command { JoinOrCreateRoom = 1, SendMessage = 2, LeaveRoom = 3, SignalCall = 4 };
enum class SignalCallAction {
  OFFER = 1,// WebRTC SDP Offer
  ANSWER = 2,// WebRTC SDP Answer (The technical 'Accept')
  REJECT = 3,// User declined the call
  CANDIDATE = 4,// ICE Network path
  HANGUP = 5// End the call
};

// Base request class
class PHANTOM_CORE_EXPORT PhantomRequestBase
{
public:
  virtual ~PhantomRequestBase() = default;

  // Restore the Move/Copy operations
  PhantomRequestBase() = default;
  PhantomRequestBase(const PhantomRequestBase &) = default;
  PhantomRequestBase &operator=(const PhantomRequestBase &) = default;
  PhantomRequestBase(PhantomRequestBase &&) = default;
  PhantomRequestBase &operator=(PhantomRequestBase &&) = default;

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
  int16_t avatar_id = 0;
  std::string display_name;

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


struct SessionDescription
{
  std::string type;// "offer" or "answer"
  std::string sdp;// The actual SDP string
};

// Structure for ICE Candidates
struct IceCandidate
{
  std::string candidate;
  std::string sdpMid;
  int sdpMLineIndex;
  std::optional<std::string> usernameFragment;
};

using SignalingData = std::variant<std::monostate, SessionDescription, IceCandidate>;
struct SignalCallRequest final : public PhantomRequestBase
{
  SignalCallRequest() { command = Command::SignalCall; }

  SignalCallAction action;// The Enum we created earlier
  SignalingData data;// The structured data field

  void validate() override;
};


using PhantomRequestPtr = std::unique_ptr<PhantomRequestBase>;

PHANTOM_CORE_EXPORT PhantomRequestPtr from_json(std::string_view jsonString);

}// namespace phantomchat::contracts