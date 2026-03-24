#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace phantomchat::services {
struct Room
{
  std::string room_name;
  std::string room_key;
  std::vector<std::string> members;// list of user_uuids
  std::string created_by;// user_uuid who created the room
};

class RoomManager
{
public:
  struct JoinOrCreateResult
  {
    bool room_created = false;// true if new room was created, false if existing room
    std::string room_key;
    std::string error_message;
  };

  struct RoomArgs
  {
    std::string room_name;
    std::string user_uuid;
  };


  RoomManager() = default;
  ~RoomManager() = default;

  static RoomManager &getInstance()
  {
    static RoomManager instance;
    return instance;
  }

  // Non-copyable, non-movable
  RoomManager(const RoomManager &) = delete;
  RoomManager &operator=(const RoomManager &) = delete;
  RoomManager(RoomManager &&) = delete;
  RoomManager &operator=(RoomManager &&) = delete;


  JoinOrCreateResult joinOrCreateRoom(const RoomArgs &args);

  std::optional<std::reference_wrapper<const Room>> getRoom(const std::string &room_name) const;

  bool roomExists(const std::string &room_name) const;

  void leaveRoom(const RoomArgs &args);

  std::vector<Room> getAllRooms() const;

private:
  mutable std::mutex rooms_mutex;
  std::unordered_map<std::string, Room> rooms;
  static std::string generateRoomKey();
};

}// namespace phantomchat::services
