#include <algorithm>
#include <cctype>
#include <fstream>
#include <string_view>

#include <phantomchat/utils/FileProvider.hpp>
#include <stdexcept>

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

std::string_view FileProvider::mimeType(const std::string &path) const
{
  auto extension = std::filesystem::path(path).extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });

  if (extension == ".html" || extension == ".htm") { return "text/html; charset=utf-8"; }
  if (extension == ".txt") { return "text/plain; charset=utf-8"; }
  if (extension == ".css") { return "text/css; charset=utf-8"; }
  if (extension == ".js") { return "application/javascript; charset=utf-8"; }
  if (extension == ".json") { return "application/json; charset=utf-8"; }
  if (extension == ".svg") { return "image/svg+xml"; }
  if (extension == ".png") { return "image/png"; }
  if (extension == ".jpg" || extension == ".jpeg") { return "image/jpeg"; }
  if (extension == ".gif") { return "image/gif"; }
  if (extension == ".webp") { return "image/webp"; }
  if (extension == ".ico") { return "image/x-icon"; }
  if (extension == ".bmp") { return "image/bmp"; }
  if (extension == ".otf") { return "font/otf"; }
  if (extension == ".sfnt") { return "font/sfnt"; }
  if (extension == ".ttf") { return "font/ttf"; }
  if (extension == ".woff") { return "font/woff"; }
  if (extension == ".woff2") { return "font/woff2"; }

  return "application/octet-stream";
}

std::filesystem::file_time_type FileProvider::lastWriteTime(const std::string &path) const
{
  return std::filesystem::last_write_time(path);
}

bool FileProvider::exists(const std::string &path) const { return std::filesystem::exists(path); }
}// namespace phantomchat::utils