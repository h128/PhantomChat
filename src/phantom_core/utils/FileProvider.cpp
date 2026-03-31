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

  if (const auto it = mime_types.find(extension); it != mime_types.end()) { return it->second; }

  return "application/octet-stream";
}

std::filesystem::file_time_type FileProvider::lastWriteTime(const std::string &path) const
{
  return std::filesystem::last_write_time(path);
}

bool FileProvider::exists(const std::string &path) const { return std::filesystem::exists(path); }
}// namespace phantomchat::utils