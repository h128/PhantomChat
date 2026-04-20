#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <utility>

namespace phantomchat::contracts {

enum class UserStatus { Active = 0, Idle = 1 };

struct PHANTOM_CORE_EXPORT Member
{
  std::string user_uuid;
  int16_t avatar_id = 0;
  std::string display_name;
  UserStatus status = UserStatus::Active;
  std::string fcm_token;
  std::string status_message{};
  std::chrono::system_clock::time_point last_push_notification_timestamp{};

  auto operator<=>(const Member &other) const { return user_uuid <=> other.user_uuid; }
};

}// namespace phantomchat::contracts
