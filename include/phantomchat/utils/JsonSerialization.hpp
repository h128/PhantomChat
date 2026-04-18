#pragma once

#include <nlohmann/json.hpp>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/contracts/Member.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/contracts/PhantomResponses.h>
#include <phantomchat/events/Events.h>

using json = nlohmann::json;

namespace phantomchat::contracts {
void to_json(nlohmann::json &j, const Member &member);
void from_json(const nlohmann::json &j, Member &member);

void to_json(nlohmann::json &j, const JoinOrCreateRoomRequest &request);
void from_json(const nlohmann::json &j, JoinOrCreateRoomRequest &request);

void to_json(nlohmann::json &j, const SendMessageRequest &request);
void from_json(const nlohmann::json &j, SendMessageRequest &request);

void to_json(nlohmann::json &j, const LeaveRoomRequest &request);
void from_json(const nlohmann::json &j, LeaveRoomRequest &request);

void to_json(nlohmann::json &j, const SessionDescription &sd);
void from_json(const nlohmann::json &j, SessionDescription &sd);

void to_json(nlohmann::json &j, const IceCandidate &ic);
void from_json(const nlohmann::json &j, IceCandidate &ic);

void to_json(nlohmann::json &j, const SignalCallRequest &request);
void from_json(const nlohmann::json &j, SignalCallRequest &request);

void to_json(nlohmann::json &j, const JoinOrCreateRoomResponse &response);
void to_json(nlohmann::json &j, const SendMessageResponse &response);
void to_json(nlohmann::json &j, const ErrorResponse &response);
void to_json(nlohmann::json &j, const GeneralResponse &response);
}// namespace phantomchat::contracts

namespace phantomchat::events {
void to_json(nlohmann::json &j, const RoomCreatedEvent &event);
void to_json(nlohmann::json &j, const UserEnteredRoomEvent &event);
void to_json(nlohmann::json &j, const NewMessageReceivedEvent &event);
void to_json(nlohmann::json &j, const LeaveRoomEvent &event);
void to_json(nlohmann::json &j, const FileUploadedEvent &event);
void to_json(nlohmann::json &j, const SignalCallRelayEvent &event);
}// namespace phantomchat::events


namespace phantomchat::config {
void to_json(nlohmann::json &j, const AppSettings &settings);
void from_json(const nlohmann::json &j, FirebaseSettings &settings);
void from_json(const nlohmann::json &j, AppSettings &settings);
}// namespace phantomchat::config
