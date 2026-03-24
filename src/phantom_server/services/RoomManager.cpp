#include "RoomManager.h"
#include <array>
#include <iomanip>
#include <sodium.h>
#include <sstream>

namespace phantomchat::services {

std::string RoomManager::generateRoomKey()
{
  constexpr int ROOM_KEY_SIZE = 32;
  if (sodium_init() == -1) { throw std::runtime_error("Failed to initialize libsodium"); }

  std::array<unsigned char, ROOM_KEY_SIZE> key{};
  randombytes_buf(key.data(), key.size());

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (const auto byte : key) { oss << std::setw(2) << static_cast<int>(byte); }

  return oss.str();
}

RoomManager::JoinOrCreateResult RoomManager::joinOrCreateRoom(const JoinOrCreateRoomRequest &request)
{
  std::lock_guard<std::mutex> lock(rooms_mutex);

  auto it = rooms.find(request.room_name);

  if (it != rooms.end()) {
    // Room exists, add user to it
    auto &room = it->second;
    auto &members = room.members;

    // Check if user is already in the room
    if (std::find(members.begin(), members.end(), request.user_uuid) == members.end()) {
      members.push_back(request.user_uuid);
    }

    return { .success = true, .room_created = false, .room_key = room.room_key, .error_message = "" };
  } else {
    // Room doesn't exist, create it
    try {
      Room new_room{ .room_name = request.room_name,
        .room_key = generateRoomKey(),
        .members = { request.user_uuid },
        .created_by = request.user_uuid };

      rooms.insert({ request.room_name, new_room });

      return { .success = true, .room_created = true, .room_key = new_room.room_key, .error_message = "" };
    } catch (const std::exception &e) {
      return { .success = false, .room_created = false, .room_key = "", .error_message = e.what() };
    }
  }
}

std::optional<Room> RoomManager::getRoom(const std::string &room_name) const
{
  std::lock_guard<std::mutex> lock(rooms_mutex);
  auto it = rooms.find(room_name);
  if (it != rooms.end()) { return it->second; }
  return {};
}


bool RoomManager::roomExists(const std::string &room_name) const
{
  std::lock_guard<std::mutex> lock(rooms_mutex);
  return rooms.find(room_name) != rooms.end();
}

std::vector<Room> RoomManager::getAllRooms() const
{
  std::lock_guard<std::mutex> lock(rooms_mutex);
  std::vector<Room> result;
  for (const auto &[name, room] : rooms) { result.push_back(room); }
  return result;
}

}// namespace phantomchat::services
