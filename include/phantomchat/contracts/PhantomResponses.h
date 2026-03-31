#pragma once

#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace phantomchat::contracts {

enum class ResponseStatus { Success = 0, Error = 1 };

// Base response class
class PHANTOM_CORE_EXPORT PhantomResponseBase
{
public:
  virtual ~PhantomResponseBase() = default;

  std::string request_uuid;
  ResponseStatus status;
  std::string message;
};

// JoinOrCreateRoom response
class PHANTOM_CORE_EXPORT JoinOrCreateRoomResponse final : public PhantomResponseBase
{
public:
  JoinOrCreateRoomResponse() { status = ResponseStatus::Success; }

  std::string room_name;
  std::string room_key;
  bool room_created = false;// true if room was newly created, false if existing
  std::unordered_set<std::string> members;// list of user_uuids currently in the room
};

class PHANTOM_CORE_EXPORT SendMessageResponse final : public PhantomResponseBase
{
public:
  SendMessageResponse() { status = ResponseStatus::Success; }
};

// Error response
class PHANTOM_CORE_EXPORT ErrorResponse final : public PhantomResponseBase
{
public:
  ErrorResponse(const std::string &err_message = "")
  {
    status = ResponseStatus::Error;
    message = err_message;
  }
};

}// namespace phantomchat::contracts
