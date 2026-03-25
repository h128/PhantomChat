#pragma once
#include <string>
namespace phantomchat::contracts {
struct PerSocketData
{
  std::string user_uuid;
  std::string room_name;
  std::string public_key;

  void assign(const std::string &user_uuid_, const std::string &room_name_, const std::string &public_key_)
  {
    user_uuid = user_uuid_;
    room_name = room_name_;
    public_key = public_key_;
  }

  void clear()
  {
    user_uuid.clear();
    room_name.clear();
    public_key.clear();
  }
};
}// namespace phantomchat::contracts