#include <algorithm>
#include <cctype>
#include <fstream>
#include <phantomchat/utils/FileProvider.hpp>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <zlib.h>

namespace phantomchat::utils {

std::vector<char> FileProvider::readAllBytes(const std::string &path) const
{
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) { throw std::runtime_error("Could not open file: " + path); }

  const std::streamsize size = file.tellg();
  if (size < 0) { throw std::runtime_error("Could not determine file size: " + path); }

  std::vector<char> buffer(static_cast<size_t>(size));

  file.seekg(0, std::ios::beg);

  if (size > 0 && !file.read(buffer.data(), size)) { throw std::runtime_error("Could not read file: " + path); }

  return buffer;
}

std::uintmax_t FileProvider::size(const std::string &path) const { return std::filesystem::file_size(path); }

std::string_view FileProvider::mimeType(const std::string &path) const noexcept
{
  auto extension = std::filesystem::path(path).extension().string();
  std::ranges::transform(
    extension, extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

  auto it = std::ranges::lower_bound(mime_types,
    extension,
    {},// Default comparator (less than)
    &std::pair<std::string_view, std::string_view>::first// Projection
  );

  if (it != mime_types.end() && it->first == extension) { return it->second; }
  return "application/octet-stream";// Default fallback
}

std::filesystem::file_time_type FileProvider::lastWriteTime(const std::string &path) const
{ return std::filesystem::last_write_time(path); }

bool FileProvider::exists(const std::string &path) const noexcept { return std::filesystem::exists(path); }

std::vector<char> FileProvider::compressFile(const std::string &path) const
{
  auto isCompressibleExtension = [](std::string_view ext) noexcept -> bool {
    // Keep this sorted alphabetically
    static constexpr std::array<std::string_view, 8> compressible = {
      ".css", ".htm", ".html", ".js", ".json", ".svg", ".txt", ".xml"
    };
    return std::ranges::binary_search(compressible, ext);
  };

  const auto ext = std::filesystem::path(path).extension().string();
  if (!isCompressibleExtension(ext)) { return {}; }

  auto bytes = readAllBytes(path);

  z_stream stream{};
  static constexpr int memLevel = 9;// Maximum memory usage, for best compression
  if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, memLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
    throw std::runtime_error("Failed to initialize zlib deflate for: " + path);
  }

  stream.next_in = reinterpret_cast<Bytef *>(bytes.data());
  stream.avail_in = static_cast<uInt>(bytes.size());

  std::vector<char> compressed;
  compressed.resize(deflateBound(&stream, stream.avail_in));

  stream.next_out = reinterpret_cast<Bytef *>(compressed.data());
  stream.avail_out = static_cast<uInt>(compressed.size());

  const int ret = deflate(&stream, Z_FINISH);
  deflateEnd(&stream);

  if (ret != Z_STREAM_END) { throw std::runtime_error("zlib deflate failed for: " + path); }
  compressed.resize(stream.total_out);

  return compressed;
}
}// namespace phantomchat::utils