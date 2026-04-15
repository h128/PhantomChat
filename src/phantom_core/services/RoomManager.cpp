#include <algorithm>
#include <phantomchat/services/CryptoRoom.h>
#include <phantomchat/services/RoomManager.h>

namespace phantomchat::services {

RoomManager::JoinOrCreateResult RoomManager::joinOrCreateRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);

  auto [it, created] = rooms.try_emplace(args.room_name);

  auto &room = it->second;
  auto &members = room.members;

  if (!std::ranges::any_of(members, [&](const auto &m) { return m.user_uuid == args.user_uuid; })) {
    members.push_back({ .user_uuid = args.user_uuid, .avatar_id = args.avatar_id, .display_name = args.display_name });
  }
  if (created) {
    // Initialize new room
    room.room_name = args.room_name;
    room.room_key = crypto_room::generateRoomKey();
    room.server_key_pair = crypto_room::genNewKeyPair();
    room.created_by = args.user_uuid;
  }

  return {
    .room_created = created, .room_key = room.room_key, .server_key_pair = room.server_key_pair, .members = members
  };
}

bool RoomManager::roomExists(const std::string &room_name) const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);
  return rooms.contains(room_name);
}

bool RoomManager::isUserMemberOfRoom(const RoomArgs &args) const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(args.room_name);
  if (it == rooms.end()) { return false; }
  const auto &members = it->second.members;
  return std::ranges::any_of(members, [&](const auto &m) { return m.user_uuid == args.user_uuid; });
}

RoomManager::LeaveRoomResult RoomManager::leaveRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(args.room_name);

  if (it == rooms.end()) { return LeaveRoomResult::RoomNotFound; }

  auto &members = it->second.members;
  auto member_it = std::ranges::find(members, args.user_uuid, &contracts::Member::user_uuid);
  if (member_it == members.end()) { return LeaveRoomResult::UserNotInRoom; }

  members.erase(member_it);

  // If room is empty after user leaves, remove the room
  if (members.empty()) {
    rooms.erase(it);
    return LeaveRoomResult::RoomEmptyAndDeleted;
  }
  return LeaveRoomResult::Success;
}

}// namespace phantomchat::services
