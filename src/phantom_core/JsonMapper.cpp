#include <phantomchat/JsonMapper.hpp>

namespace phantomchat::contracts {

void to_json(nlohmann::json &j, const JoinOrCreateRoomRequest &request)
{
  j = nlohmann::json{ { "request_uuid", request.request_uuid },
    { "user_uuid", request.user_uuid },
    { "command", static_cast<int>(request.command) },
    { "room_name", request.room_name },
    { "public_key", request.public_key } };
}

void from_json(const nlohmann::json &j, JoinOrCreateRoomRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.user_uuid = j.at("user_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
  request.room_name = j.at("room_name").get<std::string>();
  request.public_key = j.at("public_key").get<std::string>();
}

void to_json(nlohmann::json &j, const SendMessageRequest &request)
{
  j = nlohmann::json{ { "request_uuid", request.request_uuid },
    { "command", static_cast<int>(request.command) },
    { "message", request.message } };
}

void from_json(const nlohmann::json &j, SendMessageRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
  request.message = j.at("message").get<std::string>();
}

void to_json(nlohmann::json &j, const LeaveRoomRequest &request)
{
  j = nlohmann::json{ { "request_uuid", request.request_uuid }, { "command", static_cast<int>(request.command) } };
}

void from_json(const nlohmann::json &j, LeaveRoomRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
}

void to_json(nlohmann::json &j, const JoinOrCreateRoomResponse &response)
{
  j = nlohmann::json{ { "request_uuid", response.request_uuid },
    { "status", static_cast<int>(response.status) },
    { "message", response.message },
    { "room_name", response.room_name },
    { "room_key", response.room_key },
    { "room_created", response.room_created },
    { "members", response.members } };
}

void to_json(nlohmann::json &j, const SendMessageResponse &response)
{
  j = nlohmann::json{ { "request_uuid", response.request_uuid },
    { "status", static_cast<int>(response.status) },
    { "message", response.message } };
}

void to_json(nlohmann::json &j, const ErrorResponse &response)
{
  j = nlohmann::json{ { "request_uuid", response.request_uuid },
    { "status", static_cast<int>(response.status) },
    { "message", response.message } };
}

}// namespace phantomchat::contracts

namespace phantomchat::events {

void to_json(nlohmann::json &j, const RoomCreatedEvent &event)
{
  j = nlohmann::json{ { "event_name", event.event_name }, { "room_name", event.room_name } };
}

void to_json(nlohmann::json &j, const UserEnteredRoomEvent &event)
{
  j = nlohmann::json{
    { "event_name", event.event_name }, { "room_name", event.room_name }, { "user_uuid", event.user_uuid }
  };
}

void to_json(nlohmann::json &j, const NewMessageReceivedEvent &event)
{
  j = nlohmann::json{
    { "event_name", event.event_name }, { "sender_uuid", event.sender_uuid }, { "message", event.message }
  };
}

void to_json(nlohmann::json &j, const LeaveRoomEvent &event)
{
  j = nlohmann::json{ { "event_name", event.event_name }, { "user_uuid", event.user_uuid } };
}

}// namespace phantomchat::events


namespace phantomchat::config {
void to_json(nlohmann::json &j, const AppSettings &settings)
{
  j = nlohmann::json{ { "listen_port", settings.listen_port },
    { "ice_servers", settings.ice_servers },
    { "ssl_certificate", settings.ssl_certificate },
    { "ssl_certificate_key", settings.ssl_certificate_key },
    { "gzip_compression", settings.gzip_compression } };
}

void from_json(const nlohmann::json &j, AppSettings &settings)
{
  settings.listen_port = j.at("listen_port").get<int>();
  settings.ice_servers = j.at("ice_servers").get<std::vector<std::string>>();
  settings.ssl_certificate = j.at("ssl_certificate").get<std::string>();
  settings.ssl_certificate_key = j.at("ssl_certificate_key").get<std::string>();
  settings.gzip_compression = j.at("gzip_compression").get<bool>();
}
}// namespace phantomchat::config
