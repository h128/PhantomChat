#include <algorithm>
#include <phantomchat/services/CryptoRoom.h>
#include <phantomchat/services/RoomManager.h>

namespace phantomchat::services {

RoomManager::JoinOrCreateResult RoomManager::joinOrCreateRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);

  auto it = rooms.find(args.room_name);

  if (it != rooms.end()) {
    // Room exists, add user to it (skip if already present)
    auto &room = it->second;
    auto &members = room.members;
    auto member_it = std::ranges::find(members, args.user_uuid, &contracts::Member::user_uuid);
    if (member_it == members.end()) {
      members.push_back(
        { .user_uuid = args.user_uuid, .avatar_id = args.avatar_id, .display_name = args.display_name });
    }

    return {
      .room_created = false, .room_key = room.room_key, .server_key_pair = room.server_key_pair, .members = members
    };
  } else {

    Room new_room{ .room_name = args.room_name,
      .room_key = crypto_room::generateRoomKey(),
      .server_key_pair = crypto_room::genNewKeyPair(),
      .members = { { .user_uuid = args.user_uuid, .avatar_id = args.avatar_id, .display_name = args.display_name } },
      .created_by = args.user_uuid };

    auto &[_, emplaced_room] = *rooms.emplace(args.room_name, std::move(new_room)).first;

    return { .room_created = true,
      .room_key = emplaced_room.room_key,
      .server_key_pair = emplaced_room.server_key_pair,
      .members = emplaced_room.members };
  }
}

std::optional<std::reference_wrapper<const Room>> RoomManager::getRoom(const std::string &room_name) const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(room_name);
  if (it != rooms.end()) { return std::cref(it->second); }
  return {};
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

std::vector<Room> RoomManager::getAllRooms() const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);
  std::vector<Room> result;
  result.reserve(rooms.size());
  for (const auto &[_, room] : rooms) { result.push_back(room); }
  return result;
}

RoomManager::LeaveRoomResult RoomManager::leaveRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(args.room_name);

  if (it != rooms.end()) {

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
  return LeaveRoomResult::RoomNotFound;
}

}// namespace phantomchat::services
