#include <catch2/catch_test_macros.hpp>
#include <phantomchat/services/RoomManager.h>

TEST_CASE("joinOrCreateRoom creates room and seeds creator", "[room-manager]")
{
  using phantomchat::services::RoomManager;

  RoomManager manager;
  const std::string room_name = "room-manager-alpha";

  const RoomManager::RoomArgs args{ .room_name = room_name, .user_uuid = "user-1" };

  const auto result = manager.joinOrCreateRoom(args);

  REQUIRE(result.room_created);
  REQUIRE(result.room_key.size() == 64);
  REQUIRE(manager.roomExists(room_name));

  const auto room_opt = manager.getRoom(room_name);
  REQUIRE(room_opt.has_value());

  const auto &room = room_opt->get();
  REQUIRE(room.room_name == room_name);
  REQUIRE(room.created_by == "user-1");
  REQUIRE(room.room_key == result.room_key);
  REQUIRE(room.members.size() == 1);
  REQUIRE(room.members.count("user-1") == 1);
}

TEST_CASE("joinOrCreateRoom adds new members and avoids duplicates", "[room-manager]")
{
  using phantomchat::services::RoomManager;

  RoomManager manager;
  const std::string room_name = "room-manager-beta";

  const auto first = manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-1" });
  const auto second = manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-2" });
  const auto duplicate = manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-2" });

  REQUIRE(first.room_created);
  REQUIRE_FALSE(second.room_created);
  REQUIRE_FALSE(duplicate.room_created);
  REQUIRE(first.room_key == second.room_key);
  REQUIRE(second.room_key == duplicate.room_key);

  const auto room_opt = manager.getRoom(room_name);
  REQUIRE(room_opt.has_value());

  const auto &members = room_opt->get().members;
  REQUIRE(members.size() == 2);
  REQUIRE(members.count("user-1") == 1);
  REQUIRE(members.count("user-2") == 1);
}

TEST_CASE("leaveRoom removes member and deletes empty room", "[room-manager]")
{
  using phantomchat::services::RoomManager;

  RoomManager manager;
  const std::string room_name = "room-manager-gamma";

  manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-1" });
  manager.joinOrCreateRoom({ .room_name = room_name, .user_uuid = "user-2" });

  manager.leaveRoom({ .room_name = room_name, .user_uuid = "user-2" });

  REQUIRE(manager.roomExists(room_name));
  const auto room_after_first_leave = manager.getRoom(room_name);
  REQUIRE(room_after_first_leave.has_value());
  REQUIRE(room_after_first_leave->get().members.size() == 1);
  REQUIRE(room_after_first_leave->get().members.count("user-1") == 1);

  manager.leaveRoom({ .room_name = room_name, .user_uuid = "user-1" });

  REQUIRE_FALSE(manager.roomExists(room_name));
  REQUIRE_FALSE(manager.getRoom(room_name).has_value());
}
