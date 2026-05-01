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

  explicit PhantomResponseBase(ResponseStatus status_ = ResponseStatus::Success,
    std::string message_ = {},
    std::string request_uuid_ = {})
    : status(status_), message(std::move(message_)), request_uuid(std::move(request_uuid_))
  {}

  // Restore Move/Copy
  PhantomResponseBase(const PhantomResponseBase &) = default;
  PhantomResponseBase &operator=(const PhantomResponseBase &) = default;
  PhantomResponseBase(PhantomResponseBase &&) = default;
  PhantomResponseBase &operator=(PhantomResponseBase &&) = default;

  ResponseStatus status{};
  std::string message;
  std::string request_uuid;
};

// JoinOrCreateRoom response
class PHANTOM_CORE_EXPORT JoinOrCreateRoomResponse final : public PhantomResponseBase
{
public:
  explicit JoinOrCreateRoomResponse(std::string room_name_,
    std::string room_key_,
    std::string server_pub_key_,
    bool room_created_,
    std::span<const Member> members_,
    std::string request_uuid_ = {})
    : PhantomResponseBase(ResponseStatus::Success,
        room_created_ ? "Room created successfully" : "Joined room successfully",
        std::move(request_uuid_)),
      room_name(std::move(room_name_)), room_key(std::move(room_key_)), server_pub_key(std::move(server_pub_key_)),
      room_created(room_created_), members(members_)
  {}

  std::string room_name;
  std::string room_key;
  std::string server_pub_key;
  bool room_created = false;// true if room was newly created, false if existing
  std::span<const Member> members;
};

class PHANTOM_CORE_EXPORT SendMessageResponse final : public PhantomResponseBase
{
public:
  explicit SendMessageResponse(std::string message_, std::string request_uuid_)
    : PhantomResponseBase(ResponseStatus::Success, std::move(message_), std::move(request_uuid_))
  {}
};

class PHANTOM_CORE_EXPORT GeneralResponse final : public PhantomResponseBase
{
public:
  explicit GeneralResponse(std::string message_, std::string request_uuid_ = {})
    : PhantomResponseBase(ResponseStatus::Success, std::move(message_), std::move(request_uuid_))
  {}
};// namespace phantomchat::contracts

// Error response
class PHANTOM_CORE_EXPORT ErrorResponse final : public PhantomResponseBase
{
public:
  explicit ErrorResponse(std::string err_message, std::string request_uuid_ = {})
    : PhantomResponseBase(ResponseStatus::Error, std::move(err_message), std::move(request_uuid_))
  {}
};

}// namespace phantomchat::contracts
