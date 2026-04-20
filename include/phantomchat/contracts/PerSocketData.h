#pragma once
#include <string>
namespace phantomchat::contracts {
struct PerSocketData
{
  std::string user_uuid;
  std::string room_name;
  std::string public_key;

  void assign(std::string user_uuid_, std::string room_name_, std::string public_key_)
  {
    user_uuid = std::move(user_uuid_);
    room_name = std::move(room_name_);
    public_key = std::move(public_key_);
  }

  void clear()
  {
    user_uuid.clear();
    room_name.clear();
    public_key.clear();
  }

  bool isEmpty() const { return user_uuid.empty() || room_name.empty(); }
};
}// namespace phantomchat::contracts