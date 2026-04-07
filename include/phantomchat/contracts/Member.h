#pragma once

#include <compare>
#include <cstdint>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <utility>

namespace phantomchat::contracts {

struct PHANTOM_CORE_EXPORT Member
{
  std::string user_uuid;
  int16_t avatar_id = 0;
  std::string display_name;

  auto operator<=>(const Member &other) const { return user_uuid <=> other.user_uuid; }
};

}// namespace phantomchat::contracts
