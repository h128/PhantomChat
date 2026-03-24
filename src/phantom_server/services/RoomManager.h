#pragma once

#include "../contracts/PhantomRequests.h"
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace phantomchat::services {
using namespace phantomchat::contracts;

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
    bool success = false;
    bool room_created = false;// true if new room was created, false if existing room
    std::string room_key;
    std::string error_message;
  };

  RoomManager() = default;
  ~RoomManager() = default;

  // Non-copyable, non-movable
  RoomManager(const RoomManager &) = delete;
  RoomManager &operator=(const RoomManager &) = delete;
  RoomManager(RoomManager &&) = delete;
  RoomManager &operator=(RoomManager &&) = delete;


  JoinOrCreateResult joinOrCreateRoom(const JoinOrCreateRoomRequest &request);

  std::optional<Room> getRoom(const std::string &room_name) const;

  bool roomExists(const std::string &room_name) const;

  std::vector<Room> getAllRooms() const;

private:
  mutable std::mutex rooms_mutex;
  std::map<std::string, Room> rooms;
  static std::string generateRoomKey();
};

}// namespace phantomchat::services
