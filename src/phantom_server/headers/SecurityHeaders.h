#pragma once
#include <App.h>
#include <algorithm>
#include <phantomchat/config/AppSettings.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace phantomchat::security {

inline std::string resolveOrigin(std::string_view request_origin)
{
  const auto &allowed_origins = phantomchat::config::AppSettings::getInstance().cors_allowed_origins;
  for (const auto &origin : allowed_origins) {
    if (origin == "*") { return "*"; }
    if (origin == request_origin) { return std::string(request_origin); }
  }
  return {};
}

template<typename ResponseType> void writeHstsIfHttps(ResponseType *res)
{
  if constexpr (std::is_same_v<ResponseType, uWS::HttpResponse<true>>) {
    res->writeHeader("Strict-Transport-Security", "max-age=63072000; includeSubDomains; preload");
  }
}

template<typename ResponseType> void writeHeaders(ResponseType *res, const std::string &origin)
{
  if (!origin.empty()) { res->writeHeader("Access-Control-Allow-Origin", origin); }
  writeHstsIfHttps(res);
}

template<typename ResponseType, typename RequestType> void writeHeaders(ResponseType *res, RequestType *req)
{
  writeHeaders(res, resolveOrigin(req->getHeader("origin")));
}

template<typename ResponseType> void writePreflightHeaders(ResponseType *res, const std::string &origin)
{
  if (origin.empty()) { return; }
  res->writeHeader("Access-Control-Allow-Origin", origin);
  res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  res->writeHeader("Access-Control-Allow-Headers", "Content-Type, Content-Length, x-room-name, x-user-uuid");
  res->writeHeader("Access-Control-Max-Age", "86400");
  writeHstsIfHttps(res);
}

}// namespace phantomchat::security
