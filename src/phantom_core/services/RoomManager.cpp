#include <array>
#include <phantomchat/services/RoomManager.h>
#include <sodium.h>

namespace phantomchat::services {

std::string RoomManager::generateRoomKey()
{
  constexpr int room_key_size = 32;

  std::array<unsigned char, room_key_size> key{};
  randombytes_buf(key.data(), key.size());

  std::array<char, room_key_size * 2 + 1> hex{};
  sodium_bin2hex(hex.data(), hex.size(), key.data(), key.size());

  return std::string(hex.data(), hex.size() - 1);
}

RoomManager::JoinOrCreateResult RoomManager::joinOrCreateRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);

  auto it = rooms.find(args.room_name);

  if (it != rooms.end()) {
    // Room exists, add user to it
    auto &room = it->second;
    auto &members = room.members;
    members.insert(args.user_uuid);

    return { .room_created = false, .room_key = room.room_key, .members = members };
  } else {
    // Room doesn't exist, create it

    Room new_room{ .room_name = args.room_name,
      .room_key = generateRoomKey(),
      .members = { args.user_uuid },
      .created_by = args.user_uuid };

    JoinOrCreateResult response{ .room_created = true, .room_key = new_room.room_key, .members = new_room.members };
    rooms.emplace(args.room_name, std::move(new_room));

    return response;
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
  return rooms.find(room_name) != rooms.end();
}

bool RoomManager::isUserMemberOfRoom(const RoomArgs &args) const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(args.room_name);
  if (it == rooms.end()) { return false; }
  const auto &members = it->second.members;
  return members.contains(args.user_uuid);
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
    if (!members.contains(args.user_uuid)) { return LeaveRoomResult::UserNotInRoom; }
    members.erase(args.user_uuid);

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
