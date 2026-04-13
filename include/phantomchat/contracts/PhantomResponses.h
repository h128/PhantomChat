#pragma once

#include <phantomchat/contracts/Member.h>
#include <phantomchat/phantom_core_export.hpp>
#include <span>
#include <string>
#include <vector>

namespace phantomchat::contracts {

enum class ResponseStatus { Success = 0, Error = 1 };

// Base response class
class PHANTOM_CORE_EXPORT PhantomResponseBase
{
public:
  virtual ~PhantomResponseBase() = default;

  // Restore Move/Copy
  PhantomResponseBase() = default;
  PhantomResponseBase(const PhantomResponseBase &) = default;
  PhantomResponseBase &operator=(const PhantomResponseBase &) = default;
  PhantomResponseBase(PhantomResponseBase &&) = default;
  PhantomResponseBase &operator=(PhantomResponseBase &&) = default;

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
  std::string server_pub_key;
  bool room_created = false;// true if room was newly created, false if existing
  std::span<const Member> members;
};

class PHANTOM_CORE_EXPORT SendMessageResponse final : public PhantomResponseBase
{
public:
  SendMessageResponse() { status = ResponseStatus::Success; }
};

class PHANTOM_CORE_EXPORT GeneralResponse final : public PhantomResponseBase
{
public:
  explicit GeneralResponse(std::string message_ = {}, std::string request_uuid_ = {})
  {
    status = ResponseStatus::Success;
    message = std::move(message_);
    request_uuid = std::move(request_uuid_);
  }
};// namespace phantomchat::contracts

// Error response
class PHANTOM_CORE_EXPORT ErrorResponse final : public PhantomResponseBase
{
public:
  explicit ErrorResponse(std::string err_message = {})
  {
    status = ResponseStatus::Error;
    message = std::move(err_message);
  }
};

}// namespace phantomchat::contracts
