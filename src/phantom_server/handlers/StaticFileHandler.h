#pragma once

#include <App.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <memory>
#include <phantomchat/utils/CacheFileProvider.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace phantomchat::handlers {
using namespace std::chrono;
using namespace std::filesystem;

inline std::time_t toEpoch(file_time_type ftime)
{
  auto sctp = time_point_cast<system_clock::duration>(ftime - file_time_type::clock::now() + system_clock::now());
  return system_clock::to_time_t(sctp);
}

inline std::string formatHttpDate(file_time_type ftime)
{
  std::tm tm{};
  auto epoch = toEpoch(ftime);
  gmtime_r(&epoch, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
  return std::string(buf);
}

inline std::string makeETag(std::uintmax_t size, file_time_type ftime)
{
  return '"' + std::to_string(size) + '-' + std::to_string(toEpoch(ftime)) + '"';
}

inline std::time_t parseHttpDate(std::string_view sv)
{
  std::tm tm{};
  if (strptime(std::string(sv).c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm) == nullptr) {
    return static_cast<std::time_t>(-1);
  }
  return timegm(&tm);
}

template<typename ResponseType, typename RequestType>
void handleStaticFile(ResponseType *res, RequestType *req, const phantomchat::utils::CacheFileProvider &fileProvider)
{
  static constexpr std::uintmax_t ChunkedTransferThresholdBytes = 64U * 1024U;
  static constexpr std::size_t ChunkSizeBytes = 32U * 1024U;

  const std::string_view url = req->getUrl();
  std::string assetPath;
  if (url.empty() || url == "/") {
    assetPath = "index.html";
  } else {
    assetPath = std::string(url);
  }

  if (!fileProvider.exists(assetPath)) {
    res->writeStatus("404 Not Found");
    res->end("Not Found");
    return;
  }

  const auto fileSizeBytes = fileProvider.size(assetPath);
  const auto lastWriteTime = fileProvider.lastWriteTime(assetPath);
  const auto etag = makeETag(fileSizeBytes, lastWriteTime);
  const auto lastModifiedStr = formatHttpDate(lastWriteTime);

  // Conditional request: If-None-Match takes priority over If-Modified-Since
  const std::string_view ifNoneMatch = req->getHeader("if-none-match");
  const std::string_view ifModifiedSince = req->getHeader("if-modified-since");

  const bool etagMatch = !ifNoneMatch.empty() && ifNoneMatch == etag;
  const bool notModified =
    ifNoneMatch.empty() && !ifModifiedSince.empty() && parseHttpDate(ifModifiedSince) >= toEpoch(lastWriteTime);

  if (etagMatch || notModified) {
    res->writeStatus("304 Not Modified");
    if (etagMatch) { res->writeHeader("ETag", etag); }
    res->end();
    return;
  }

  res->writeHeader("Content-Type", fileProvider.mimeType(assetPath));
  res->writeHeader("Cache-Control", "public, max-age=7200");
  res->writeHeader("ETag", etag);
  res->writeHeader("Last-Modified", lastModifiedStr);

  if (fileSizeBytes < ChunkedTransferThresholdBytes) {
    const auto &bytes = fileProvider.readBytesRef(assetPath);
    res->end(std::string_view(bytes.data(), bytes.size()));
    return;
  }

  res->writeHeader("Transfer-Encoding", "chunked");

  auto bytes = std::make_shared<std::vector<char>>(fileProvider.readBytesRef(assetPath));
  auto offset = std::make_shared<std::size_t>(0U);
  auto finished = std::make_shared<bool>(false);

  res->onAborted([bytes, finished] {
    *finished = true;
    bytes->clear();
    bytes->shrink_to_fit();// Release memory immediately
  });

  res->onWritable([res, bytes, offset, finished](std::uintmax_t) mutable {
    if (*finished) { return false; }
    while (*offset < bytes->size()) {
      const auto remaining = bytes->size() - *offset;
      const auto chunkSize = std::min<std::size_t>(ChunkSizeBytes, remaining);
      const auto ok = res->write(std::string_view(bytes->data() + *offset, chunkSize));
      *offset += chunkSize;
      if (!ok) { return true; }
    }
    *finished = true;
    res->end();
    return false;
  });

  while (*offset < bytes->size()) {
    const auto remaining = bytes->size() - *offset;
    const auto chunkSize = std::min<std::size_t>(ChunkSizeBytes, remaining);
    const auto ok = res->write(std::string_view(bytes->data() + *offset, chunkSize));
    *offset += chunkSize;
    if (!ok) { break; }
  }
  if (*offset >= bytes->size() && !*finished) {
    *finished = true;
    res->end();
  }
}

}// namespace phantomchat::handlers
