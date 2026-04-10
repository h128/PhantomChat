#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace phantomchat::http {

static constexpr std::size_t default_chunk_size_bytes = 64U * 1024U;

// Send a byte buffer using chunked transfer encoding with backpressure support.
// The caller must have already written status and other headers before calling this.
// `bytes` is kept alive via shared_ptr until the transfer completes or is aborted.
template<typename ResponseType>
void sendChunked(ResponseType *res,
  std::shared_ptr<const std::vector<char>> bytes,
  std::size_t chunk_size = default_chunk_size_bytes)
{
  res->writeHeader("Transfer-Encoding", "chunked");

  auto offset = std::make_shared<std::size_t>(0U);
  auto finished = std::make_shared<bool>(false);

  res->onAborted([bytes, finished]() { *finished = true; });

  res->onWritable([res, bytes, offset, finished, chunk_size](std::uintmax_t) mutable {
    if (*finished) { return false; }
    while (*offset < bytes->size()) {
      const auto remaining = bytes->size() - *offset;
      const auto len = std::min<std::size_t>(chunk_size, remaining);
      const auto ok = res->write(std::string_view(bytes->data() + *offset, len));
      *offset += len;
      if (!ok) { return true; }
    }
    *finished = true;
    res->end();
    return false;
  });

  // Initial write attempt
  while (*offset < bytes->size()) {
    const auto remaining = bytes->size() - *offset;
    const auto len = std::min<std::size_t>(chunk_size, remaining);
    const auto ok = res->write(std::string_view(bytes->data() + *offset, len));
    *offset += len;
    if (!ok) { return; }
  }
  if (!*finished) {
    *finished = true;
    res->end();
  }
}

}// namespace phantomchat::http
