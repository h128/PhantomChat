#include <array>
#include <iomanip>
#include <phantomchat/services/RoomManager.h>
#include <sodium.h>
#include <sstream>

namespace phantomchat::services {

std::string RoomManager::generateRoomKey()
{
  constexpr int ROOM_KEY_SIZE = 32;

  std::array<unsigned char, ROOM_KEY_SIZE> key{};
  randombytes_buf(key.data(), key.size());

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (const auto byte : key) { oss << std::setw(2) << static_cast<int>(byte); }

  return oss.str();
}

RoomManager::JoinOrCreateResult RoomManager::joinOrCreateRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);

  auto it = rooms.find(args.room_name);

  if (it != rooms.end()) {
    // Room exists, add user to it
    auto &room = it->second;
    auto &members = room.members;

    // Check if user is already in the room
    if (std::find(members.begin(), members.end(), args.user_uuid) == members.end()) {
      members.push_back(args.user_uuid);
    }

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
  return std::find(members.begin(), members.end(), args.user_uuid) != members.end();
}

std::vector<Room> RoomManager::getAllRooms() const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);
  std::vector<Room> result;
  result.reserve(rooms.size());
  for (const auto &[_, room] : rooms) { result.push_back(room); }
  return result;
}

void RoomManager::leaveRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(args.room_name);

  if (it != rooms.end()) {

    auto &members = it->second.members;
    members.erase(std::remove(members.begin(), members.end(), args.user_uuid), members.end());

    // If room is empty after user leaves, remove the room
    if (members.empty()) { rooms.erase(it); }
  }
}

}// namespace phantomchat::services
