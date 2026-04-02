#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace phantomchat::cors {

inline std::vector<std::string> allowed_origins = { "*" };

inline std::string resolveOrigin(std::string_view request_origin)
{
  for (const auto &origin : allowed_origins) {
    if (origin == "*") { return "*"; }
    if (origin == request_origin) { return std::string(request_origin); }
  }
  return {};
}

template<typename ResponseType> void writeHeaders(ResponseType *res, const std::string &origin)
{
  if (!origin.empty()) { res->writeHeader("Access-Control-Allow-Origin", origin); }
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
}

}// namespace phantomchat::cors
