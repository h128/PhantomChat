#pragma once
#include <string>
namespace phantomchat::contracts {
struct PerSocketData
{
  std::string user_uuid;
  std::string room_name;
  std::string public_key;
};
}