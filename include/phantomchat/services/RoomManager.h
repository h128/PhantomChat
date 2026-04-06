#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <phantomchat/phantom_core_export.hpp>
#include <phantomchat/services/CryptoRoom.h>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace phantomchat::services {
struct PHANTOM_CORE_EXPORT Room
{
  std::string room_name;
  std::string room_key;
  crypto_room::KeyPair server_key_pair;// hex-encoded
  std::unordered_set<std::string> members;// list of user_uuids
  std::string created_by;// user_uuid who created the room
};

class PHANTOM_CORE_EXPORT RoomManager
{
public:
  struct JoinOrCreateResult
  {
    bool room_created = false;// true if new room was created, false if existing room
    std::string room_key;
    crypto_room::KeyPair server_key_pair;// hex-encoded server key pair for this room
    std::unordered_set<std::string> members;// list of user_uuids
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

  bool isUserMemberOfRoom(const RoomArgs &args) const;

  enum class LeaveRoomResult { Success, RoomNotFound, UserNotInRoom, RoomEmptyAndDeleted };
  LeaveRoomResult leaveRoom(const RoomArgs &args);

  std::vector<Room> getAllRooms() const;

private:
  mutable std::shared_mutex rooms_mutex;
  std::unordered_map<std::string, Room> rooms;
};

}// namespace phantomchat::services
