#pragma once

#include <nlohmann/json.hpp>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/contracts/PhantomResponses.h>
#include <phantomchat/events/Events.h>

using json = nlohmann::json;

namespace phantomchat::contracts {
void to_json(nlohmann::json &j, const JoinOrCreateRoomRequest &request);
void from_json(const nlohmann::json &j, JoinOrCreateRoomRequest &request);

void to_json(nlohmann::json &j, const SendMessageRequest &request);
void from_json(const nlohmann::json &j, SendMessageRequest &request);

void to_json(nlohmann::json &j, const LeaveRoomRequest &request);
void from_json(const nlohmann::json &j, LeaveRoomRequest &request);

void to_json(nlohmann::json &j, const JoinOrCreateRoomResponse &response);
void to_json(nlohmann::json &j, const SendMessageResponse &response);
void to_json(nlohmann::json &j, const ErrorResponse &response);
}// namespace phantomchat::contracts

namespace phantomchat::events {
void to_json(nlohmann::json &j, const RoomCreatedEvent &event);
void to_json(nlohmann::json &j, const UserEnteredRoomEvent &event);
void to_json(nlohmann::json &j, const NewMessageReceivedEvent &event);
void to_json(nlohmann::json &j, const LeaveRoomEvent &event);
}// namespace phantomchat::events


namespace phantomchat::config {
void from_json(const nlohmann::json &j, AppSettings &settings);
void to_json(nlohmann::json &j, const AppSettings &settings);
}// namespace phantomchat::config
