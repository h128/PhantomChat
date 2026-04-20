#pragma once

#include <chrono>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <string_view>

namespace phantomchat::services::firebase {

struct PHANTOM_CORE_EXPORT JwtResult
{
  std::string token;
  std::chrono::system_clock::time_point issued_at;
};

struct PHANTOM_CORE_EXPORT AccessToken
{
  std::string token;
  std::chrono::system_clock::time_point expires_at;
};
PHANTOM_CORE_EXPORT AccessToken fetch_access_token(const phantomchat::config::FirebaseSettings &settings);

enum class FcmSendResult { Success, Unauthorized, Failed };

PHANTOM_CORE_EXPORT FcmSendResult send_fcm_message(std::string_view access_token,
  std::string_view project_id,
  std::string_view fcm_token,
  std::string_view title,
  std::string_view body,
  std::string_view icon = {});

}// namespace phantomchat::services::firebase
