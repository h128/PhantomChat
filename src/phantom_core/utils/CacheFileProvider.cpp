#include <phantomchat/utils/CacheFileProvider.hpp>
#include <phantomchat/utils/FileProvider.hpp>

#include <stdexcept>
#include <utility>

namespace {
std::string normalizePathKey(std::string_view path)
{
  const auto pos = path.find_first_not_of('/');
  if (pos == std::string_view::npos) { return {}; }
  const auto new_path = path.substr(pos);

  // Reject path traversal and null bytes
  if (new_path.find("..") != std::string_view::npos || new_path.find('\0') != std::string_view::npos) { return {}; }

  return std::string(new_path);
}
}// namespace

namespace phantomchat::utils {

CacheFileProvider::CacheFileProvider(std::string root_path, bool gzip_compression)
  : root_path_(std::move(root_path)), gzip_enabled_(gzip_compression)
{


  const auto root = std::filesystem::path(root_path_);
  if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) { return; }

  phantomchat::utils::FileProvider file_provider;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) { continue; }

    const auto absolute_path = entry.path();
    const auto relative_path = std::filesystem::relative(absolute_path, root).generic_string();
    const auto absolute_path_string = absolute_path.string();

    CachedFileEntry cached_entry;
    cached_entry.last_write_time = file_provider.lastWriteTime(absolute_path_string);
    cached_entry.bytes = file_provider.readAllBytes(absolute_path_string);
    cached_entry.size = cached_entry.bytes.size();
    cached_entry.mime_type = file_provider.mimeType(absolute_path_string);

    if (gzip_enabled_) {
      cached_entry.compressed_bytes = file_provider.compressFile(absolute_path_string);
      cached_entry.has_compressed = !cached_entry.compressed_bytes.empty();
      if (cached_entry.has_compressed) {
        cached_entry.bytes.clear();
        cached_entry.bytes.shrink_to_fit();
      }
    }

    cache_.emplace(relative_path, std::move(cached_entry));
  }
}

const std::vector<char> &CacheFileProvider::readBytesRef(const std::string &path) const { return getEntry(path).bytes; }

std::vector<char> CacheFileProvider::readAllBytes(const std::string &path) const { return readBytesRef(path); }

std::uintmax_t CacheFileProvider::size(const std::string &path) const { return getEntry(path).size; }

std::filesystem::file_time_type CacheFileProvider::lastWriteTime(const std::string &path) const
{ return getEntry(path).last_write_time; }

const CachedFileEntry &CacheFileProvider::getEntry(const std::string &path) const
{
  const auto it = findFile(path);
  if (it == cache_.end()) { throw std::runtime_error("File not found in cache: " + path); }
  return it->second;
}

std::string_view CacheFileProvider::mimeType(const std::string &path) const noexcept
{
  const auto it = findFile(path);
  return it != cache_.end() ? it->second.mime_type : std::string_view{};
}

bool CacheFileProvider::exists(const std::string &path) const noexcept { return findFile(path) != cache_.end(); }

bool CacheFileProvider::hasCompressed(const std::string &path) const noexcept
{
  const auto it = findFile(path);
  return it != cache_.end() && it->second.has_compressed;
}

const std::vector<char> &CacheFileProvider::readCompressedBytesRef(const std::string &path) const
{ return getEntry(path).compressed_bytes; }

CacheFileProvider::CacheIterator CacheFileProvider::findFile(const std::string &path) const
{ return cache_.find(normalizePathKey(path)); }
}// namespace phantomchat::utils