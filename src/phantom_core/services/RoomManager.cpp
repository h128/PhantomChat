#include <algorithm>
#include <phantomchat/services/CryptoRoom.h>
#include <phantomchat/services/RoomManager.h>
#include <ranges>

namespace phantomchat::services {

RoomManager::JoinOrCreateResult RoomManager::joinOrCreateRoom(const RoomArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);

  auto [it, created] = rooms.try_emplace(args.room_name);

  auto &room = it->second;
  auto &members = room.members;

  bool found = std::ranges::contains(members, args.user_uuid, &contracts::Member::user_uuid);
  if (!found) {
    members.push_back({ //
      .user_uuid = args.user_uuid,
      .avatar_id = args.avatar_id,
      .display_name = args.display_name,
      .fcm_token = args.fcm_token });
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

void RoomManager::setUserStatus(const SetUserStatusArgs &args)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(args.room_name);
  if (it == rooms.end()) { return; }

  auto &members = it->second.members;
  auto member_it = std::ranges::find(members, args.user_uuid, &contracts::Member::user_uuid);
  if (member_it == members.end()) { return; }

  member_it->status = args.status;
  member_it->status_message = std::move(args.status_message);

  return;
}

std::vector<std::string> RoomManager::getIdleMembers(const std::string &room_name,
  std::chrono::seconds min_push_interval) const
{
  std::shared_lock<std::shared_mutex> lock(rooms_mutex);

  auto it = rooms.find(room_name);
  if (it == rooms.end()) { return {}; }

  const auto now = std::chrono::system_clock::now();
  const auto cutoff = now - min_push_interval;

  const auto &members = it->second.members;

  auto is_idle = [](const auto &member) { return member.status == contracts::UserStatus::Idle; };

  auto has_fcm_token = [](const auto &member) { return !member.fcm_token.empty(); };

  auto is_outdated = [cutoff](const auto &member) { return member.last_push_notification_timestamp <= cutoff; };

  // Pipeline with chained filters
  auto idle_tokens = members |//
                     std::views::filter(is_idle) |//
                     std::views::filter(has_fcm_token) |//
                     std::views::filter(is_outdated) |//
                     std::views::transform(&contracts::Member::fcm_token);

  std::vector<std::string> result(members.size());
  std::ranges::copy(idle_tokens, std::back_inserter(result));

  return result;
}

void RoomManager::markPushed(const std::string &room_name, std::string_view fcm_token)
{
  std::unique_lock<std::shared_mutex> lock(rooms_mutex);
  auto it = rooms.find(room_name);
  if (it == rooms.end()) { return; }

  auto &members = it->second.members;
  auto member_it = std::ranges::find(members, fcm_token, &contracts::Member::fcm_token);
  if (member_it == members.end()) { return; }

  member_it->last_push_notification_timestamp = std::chrono::system_clock::now();
}

}// namespace phantomchat::services
