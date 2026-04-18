#pragma once

#include <chrono>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <vector>

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

}// namespace phantomchat::services::firebase
