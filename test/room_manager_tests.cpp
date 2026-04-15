#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <phantomchat/services/RoomManager.h>

TEST_CASE("joinOrCreateRoom creates room and seeds creator", "[room-manager]")
{
  using phantomchat::services::RoomManager;

  auto &manager = phantomchat::services::RoomManager::getInstance();
  const std::string room_name = "room-manager-alpha";

  const RoomManager::RoomArgs args{ .room_name = room_name, .user_uuid = "user-1" };

  const auto result = manager.joinOrCreateRoom(args);

  REQUIRE(result.room_created);
  REQUIRE(result.room_key.size() == 64);
  REQUIRE_FALSE(result.server_key_pair.public_key.empty());
  REQUIRE_FALSE(result.server_key_pair.secret_key.empty());
  REQUIRE(manager.roomExists(room_name));

  // Can't access room directly, so check via public API
  REQUIRE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-1" }));
}

TEST_CASE("joinOrCreateRoom adds new members and avoids duplicates", "[room-manager]")
{
  using phantomchat::services::RoomManager;

  auto &manager = RoomManager::getInstance();
  const std::string room_name = "room-manager-beta";

  const auto first = manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-1" });
  const auto second = manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-2" });
  const auto duplicate = manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-2" });

  REQUIRE(first.room_created);
  REQUIRE_FALSE(second.room_created);
  REQUIRE_FALSE(duplicate.room_created);
  REQUIRE(first.room_key == second.room_key);
  REQUIRE(second.room_key == duplicate.room_key);
  REQUIRE(first.server_key_pair.public_key == second.server_key_pair.public_key);

  // Can't access room directly, so check via public API
  REQUIRE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-1" }));
  REQUIRE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-2" }));
}

TEST_CASE("leaveRoom removes member and deletes empty room", "[room-manager]")
{
  using phantomchat::services::RoomManager;

  auto &manager = RoomManager::getInstance();
  const std::string room_name = "room-manager-gamma";

  manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-1" });
  manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-2" });

  manager.leaveRoom({ .room_name = room_name, .user_uuid = "user-2" });

  REQUIRE(manager.roomExists(room_name));
  // After first leave, user-1 should still be a member, user-2 should not
  REQUIRE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-1" }));
  REQUIRE_FALSE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-2" }));

  manager.leaveRoom({ .room_name = room_name, .user_uuid = "user-1" });

  REQUIRE_FALSE(manager.roomExists(room_name));
  // Room should not exist, so neither user should be a member
  REQUIRE_FALSE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-1" }));
  REQUIRE_FALSE(manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = "user-2" }));
}
