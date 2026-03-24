#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace phantomchat::contracts {

enum class ResponseStatus { Success = 0, Error = 1 };

// Base response class
class PhantomResponseBase
{
public:
  virtual ~PhantomResponseBase() = default;

  std::string request_uuid;
  ResponseStatus status;
  std::string message;
};

// JoinOrCreateRoom response
class JoinOrCreateRoomResponse final : public PhantomResponseBase
{
public:
  JoinOrCreateRoomResponse() { status = ResponseStatus::Success; }

  std::string room_name;
  std::string room_key;
  bool room_created = false;// true if room was newly created, false if existing

  json to_json() const
  {
    return json{ { "request_uuid", request_uuid },
      { "status", static_cast<int>(status) },
      { "message", message },
      { "room_name", room_name },
      { "room_key", room_key },
      { "room_created", room_created } };
  }
};

// Error response
class ErrorResponse final : public PhantomResponseBase
{
public:
  ErrorResponse(const std::string &err_message = "")
  {
    status = ResponseStatus::Error;
    message = err_message;
  }

  json to_json() const
  {
    return json{ { "request_uuid", request_uuid }, { "status", static_cast<int>(status) }, { "message", message } };
  }
};

}// namespace phantomchat::contracts
