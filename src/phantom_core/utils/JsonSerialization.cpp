#include <chrono>
#include <fmt/chrono.h>
#include <phantomchat/utils/JsonSerialization.hpp>

namespace {
inline std::string utc_now_iso()
{
  return fmt::format(
    "{:%Y-%m-%dT%H:%M:%SZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}
}// anonymous namespace

namespace phantomchat::contracts {

void to_json(nlohmann::json &j, const Member &member)
{
  j = nlohmann::json{ { "user_uuid", member.user_uuid },
    { "avatar_id", member.avatar_id },
    { "display_name", member.display_name },
    { "status", static_cast<int>(member.status) },
    { "status_message", member.status_message } };
}

void from_json(const nlohmann::json &j, Member &member)
{
  member.user_uuid = j.at("user_uuid").get<std::string>();
  member.avatar_id = j.value("avatar_id", static_cast<int16_t>(0));
  member.display_name = j.value("display_name", std::string{});
  member.status = static_cast<UserStatus>(j.value("status", 0));
  member.status_message = j.value("status_message", std::string{});
}


void from_json(const nlohmann::json &j, JoinOrCreateRoomRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.user_uuid = j.at("user_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
  request.room_name = j.at("room_name").get<std::string>();
  request.public_key = j.at("public_key").get<std::string>();
  request.avatar_id = j.value("avatar_id", static_cast<int16_t>(0));
  request.display_name = j.value("display_name", std::string{});
  request.fcm_token = j.value("fcm_token", std::string{});
}


void from_json(const nlohmann::json &j, SetUserStatusRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
  request.status = static_cast<UserStatus>(j.at("status").get<int>());
  request.status_message = j.value("status_message", std::string{});
}


void from_json(const nlohmann::json &j, SendMessageRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
  request.message = j.at("message").get<std::string>();
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
    { "server_pub_key", response.server_pub_key },
    { "room_created", response.room_created },
    { "members", response.members },
    { "timestamp", utc_now_iso() } };
}

void to_json(nlohmann::json &j, const SendMessageResponse &response)
{
  j = nlohmann::json{ { "request_uuid", response.request_uuid },
    { "status", static_cast<int>(response.status) },
    { "message", response.message },
    { "timestamp", utc_now_iso() } };
}

void to_json(nlohmann::json &j, const ErrorResponse &response)
{
  j = nlohmann::json{ { "request_uuid", response.request_uuid },
    { "status", static_cast<int>(response.status) },
    { "message", response.message },
    { "timestamp", utc_now_iso() } };
}
void to_json(nlohmann::json &j, const GeneralResponse &response)
{
  j = nlohmann::json{ { "request_uuid", response.request_uuid },
    { "status", static_cast<int>(response.status) },
    { "message", response.message },
    { "timestamp", utc_now_iso() } };
}

void to_json(nlohmann::json &j, const SessionDescription &sd)
{ j = nlohmann::json{ { "type", sd.type }, { "sdp", sd.sdp } }; }

void from_json(const nlohmann::json &j, SessionDescription &sd)
{
  sd.type = j.at("type").get<std::string>();
  sd.sdp = j.at("sdp").get<std::string>();
}

void to_json(nlohmann::json &j, const IceCandidate &ic)
{
  j = nlohmann::json{ { "candidate", ic.candidate }, { "sdpMid", ic.sdpMid }, { "sdpMLineIndex", ic.sdpMLineIndex } };
  if (ic.usernameFragment.has_value()) { j["usernameFragment"] = ic.usernameFragment.value(); }
}

void from_json(const nlohmann::json &j, IceCandidate &ic)
{
  ic.candidate = j.at("candidate").get<std::string>();
  ic.sdpMid = j.at("sdpMid").get<std::string>();
  ic.sdpMLineIndex = j.at("sdpMLineIndex").get<int>();
  if (j.contains("usernameFragment") && !j["usernameFragment"].is_null()) {
    ic.usernameFragment = j["usernameFragment"].get<std::string>();
  }
}


void from_json(const nlohmann::json &j, SignalCallRequest &request)
{
  request.request_uuid = j.at("request_uuid").get<std::string>();
  request.command = static_cast<Command>(j.at("command").get<int>());
  request.action = static_cast<SignalCallAction>(j.at("action").get<int>());

  if (j.contains("data") && !j["data"].is_null()) {
    switch (request.action) {
    case SignalCallAction::OFFER:
    case SignalCallAction::ANSWER:
      request.data = j["data"].get<SessionDescription>();
      break;
    case SignalCallAction::CANDIDATE:
      request.data = j["data"].get<IceCandidate>();
      break;
    default:
      request.data = std::monostate{};
      break;
    }
  }
}

}// namespace phantomchat::contracts

namespace phantomchat::events {

void to_json(nlohmann::json &j, const RoomCreatedEvent &event)
{
  j = nlohmann::json{
    { "event_name", event.event_name }, { "room_name", event.room_name }, { "timestamp", utc_now_iso() }
  };
}

void to_json(nlohmann::json &j, const UserEnteredRoomEvent &event)
{
  j = nlohmann::json{ { "event_name", event.event_name },
    { "room_name", event.room_name },
    { "user_uuid", event.user_uuid },
    { "avatar_id", event.avatar_id },
    { "display_name", event.display_name },
    { "timestamp", utc_now_iso() } };
}

void to_json(nlohmann::json &j, const NewMessageReceivedEvent &event)
{
  j = nlohmann::json{ { "event_name", event.event_name },
    { "sender_uuid", event.sender_uuid },
    { "message", event.message },
    { "timestamp", utc_now_iso() } };
}

void to_json(nlohmann::json &j, const LeaveRoomEvent &event)
{
  j = nlohmann::json{
    { "event_name", event.event_name }, { "user_uuid", event.user_uuid }, { "timestamp", utc_now_iso() }
  };
}

void to_json(nlohmann::json &j, const FileUploadedEvent &event)
{
  j = nlohmann::json{ { "event_name", event.event_name },
    { "file_name", event.file_name },
    { "user_uuid", event.user_uuid },
    { "poster", event.poster },
    { "timestamp", utc_now_iso() } };
}

void to_json(nlohmann::json &j, const SignalCallRelayEvent &event)
{
  j = nlohmann::json{ { "event_name", event.event_name },
    { "action", static_cast<int>(event.action) },
    { "sender_uuid", event.sender_uuid },
    { "data",
      std::visit(
        [](auto &&arg) -> nlohmann::json {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, contracts::SessionDescription> || std::is_same_v<T, contracts::IceCandidate>)
            return arg;
          return nullptr;
        },
        event.signaling_data) },
    { "timestamp", utc_now_iso() } };
}

}// namespace phantomchat::events


namespace phantomchat::config {

void from_json(const nlohmann::json &j, FirebaseSettings &settings)
{
  settings.enabled = j.value("enabled", false);
  settings.min_push_interval_seconds = j.value("min_push_interval_seconds", 60);
  settings.type = j.at("type").get<std::string>();
  settings.scope = j.at("scope").get<std::string>();
  settings.project_id = j.at("project_id").get<std::string>();
  settings.private_key_id = j.at("private_key_id").get<std::string>();
  settings.private_key = j.at("private_key").get<std::string>();
  settings.client_email = j.at("client_email").get<std::string>();
  settings.client_id = j.at("client_id").get<std::string>();
  settings.auth_uri = j.at("auth_uri").get<std::string>();
  settings.token_uri = j.at("token_uri").get<std::string>();
  settings.auth_provider_x509_cert_url = j.at("auth_provider_x509_cert_url").get<std::string>();
  settings.client_x509_cert_url = j.at("client_x509_cert_url").get<std::string>();
  settings.universe_domain = j.at("universe_domain").get<std::string>();
}

void from_json(const nlohmann::json &j, AppSettings &settings)
{
  settings.listen_port = j.at("listen_port").get<int>();
  settings.ice_servers = j.at("ice_servers").get<std::vector<std::string>>();
  settings.ssl_certificate = j.at("ssl_certificate").get<std::string>();
  settings.ssl_certificate_key = j.at("ssl_certificate_key").get<std::string>();
  settings.gzip_compression = j.at("gzip_compression").get<bool>();
  settings.worker_threads = j.at("worker_threads").get<int>();
  settings.web_root_path = j.at("web_root_path").get<std::string>();
  settings.upload_path = j.at("upload_path").get<std::string>();
  settings.cors_allowed_origins = j.at("cors_allowed_origins").get<std::vector<std::string>>();
  settings.firebase_settings = j.at("firebase_settings").get<FirebaseSettings>();
}
}// namespace phantomchat::config
