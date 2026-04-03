#include "../headers/StaticFileHandler.h"
#include "../headers/CorsHelper.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace std::chrono;
using namespace std::filesystem;

std::time_t toEpoch(file_time_type ftime)
{
  auto sctp = time_point_cast<system_clock::duration>(ftime - file_time_type::clock::now() + system_clock::now());
  return system_clock::to_time_t(sctp);
}

std::string formatHttpDate(file_time_type ftime)
{
  std::tm tm{};
  auto epoch = toEpoch(ftime);
  gmtime_r(&epoch, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
  return std::string(buf);
}

std::string makeETag(std::uintmax_t size, file_time_type ftime)
{
  return '"' + std::to_string(size) + '-' + std::to_string(toEpoch(ftime)) + '"';
}

std::time_t parseHttpDate(std::string_view sv)
{
  std::tm tm{};
  if (strptime(std::string(sv).c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm) == nullptr) {
    return static_cast<std::time_t>(-1);
  }
  return timegm(&tm);
}
}// anonymous namespace

namespace phantomchat::handlers {

template<typename ResponseType, typename RequestType>
void handleStaticFile(ResponseType *res, RequestType *req, const phantomchat::utils::CacheFileProvider &file_provider)
{
  static constexpr std::uintmax_t chunked_transfer_threshold_bytes = 64U * 1024U;
  static constexpr std::size_t chunk_size_bytes = 32U * 1024U;

  const std::string_view url = req->getUrl();
  std::string asset_path;
  if (url.empty() || url == "/") {
    asset_path = "index.html";
  } else {
    asset_path = std::string(url);
  }

  // SPA fallback: if the path has no file extension, serve index.html
  if (!file_provider.exists(asset_path) && asset_path.find('.') == std::string::npos) { asset_path = "index.html"; }

  if (!file_provider.exists(asset_path)) {
    res->writeStatus("404 Not Found");
    phantomchat::cors::writeHeaders(res, req);
    res->end("Not Found");
    return;
  }

  const auto file_size_bytes = file_provider.size(asset_path);
  const auto last_write_time = file_provider.lastWriteTime(asset_path);
  const auto etag = makeETag(file_size_bytes, last_write_time);
  const auto last_modified_str = formatHttpDate(last_write_time);

  // Conditional request: If-None-Match takes priority over If-Modified-Since
  const std::string_view if_none_match = req->getHeader("if-none-match");
  const std::string_view if_modified_since = req->getHeader("if-modified-since");

  const bool etag_match = !if_none_match.empty() && if_none_match == etag;
  const bool not_modified =
    if_none_match.empty() && !if_modified_since.empty() && parseHttpDate(if_modified_since) >= toEpoch(last_write_time);

  if (etag_match || not_modified) {
    res->writeStatus("304 Not Modified");
    phantomchat::cors::writeHeaders(res, req);
    if (etag_match) { res->writeHeader("ETag", etag); }
    res->end();
    return;
  }

  phantomchat::cors::writeHeaders(res, req);
  res->writeHeader("Content-Type", file_provider.mimeType(asset_path));
  res->writeHeader("Cache-Control", "public, max-age=7200");
  res->writeHeader("ETag", etag);
  res->writeHeader("Last-Modified", last_modified_str);

  const auto &bytes = file_provider.readBytesRef(asset_path);
  if (file_size_bytes < chunked_transfer_threshold_bytes) {
    res->end(std::string_view(bytes.data(), bytes.size()));
    return;
  }

  res->writeHeader("Transfer-Encoding", "chunked");

  auto offset = std::make_shared<std::size_t>(0U);
  auto finished = std::make_shared<bool>(false);

  res->onAborted([finished] { *finished = true; });

  res->onWritable([res, &bytes, offset, finished](std::uintmax_t) mutable {
    if (*finished) { return false; }
    while (*offset < bytes.size()) {
      const auto remaining = bytes.size() - *offset;
      const auto chunk_size = std::min<std::size_t>(chunk_size_bytes, remaining);
      const auto ok = res->write(std::string_view(bytes.data() + *offset, chunk_size));
      *offset += chunk_size;
      if (!ok) { return true; }
    }
    *finished = true;
    res->end();
    return false;
  });

  while (*offset < bytes.size()) {
    const auto remaining = bytes.size() - *offset;
    const auto chunk_size = std::min<std::size_t>(chunk_size_bytes, remaining);
    const auto ok = res->write(std::string_view(bytes.data() + *offset, chunk_size));
    *offset += chunk_size;
    if (!ok) { break; }
  }
  if (*offset >= bytes.size() && !*finished) {
    *finished = true;
    res->end();
  }
}

}// namespace phantomchat::handlers

template void phantomchat::handlers::handleStaticFile<uWS::HttpResponse<false>, uWS::HttpRequest>(
  uWS::HttpResponse<false> *,
  uWS::HttpRequest *,
  const phantomchat::utils::CacheFileProvider &);

template void phantomchat::handlers::handleStaticFile<uWS::HttpResponse<true>, uWS::HttpRequest>(
  uWS::HttpResponse<true> *,
  uWS::HttpRequest *,
  const phantomchat::utils::CacheFileProvider &);
