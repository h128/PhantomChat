#include <phantomchat/utils/CacheFileProvider.hpp>
#include <phantomchat/utils/FileProvider.hpp>

#include <stdexcept>
#include <utility>

namespace {
std::string normalizePathKey(const std::string &path)
{
  auto normalized = std::filesystem::path(path).lexically_normal().generic_string();
  while (!normalized.empty() && normalized.front() == '/') { normalized.erase(normalized.begin()); }
  return normalized;
}
}// namespace

namespace phantomchat::utils {

CacheFileProvider::CacheFileProvider(std::string rootPath) : rootPath_(std::move(rootPath))
{
  phantomchat::utils::FileProvider fileProvider;

  const auto root = std::filesystem::path(rootPath_);
  if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
    throw std::invalid_argument("Invalid root directory for cache file provider: " + rootPath_);
  }

  for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) { continue; }

    const auto absolutePath = entry.path();
    const auto relativePath = std::filesystem::relative(absolutePath, root).generic_string();
    const auto absolutePathString = absolutePath.string();

    CachedFileEntry cachedEntry;
    cachedEntry.lastWriteTime = fileProvider.lastWriteTime(absolutePathString);
    cachedEntry.bytes = fileProvider.readAllBytes(absolutePathString);
    cachedEntry.size = cachedEntry.bytes.size();
    cachedEntry.mimeType = fileProvider.mimeType(absolutePathString);
    cache_.emplace(relativePath, std::move(cachedEntry));
  }
}

std::vector<char> CacheFileProvider::readAllBytes(const std::string &path) const { return getEntry(path).bytes; }

std::uintmax_t CacheFileProvider::size(const std::string &path) const { return getEntry(path).size; }

std::filesystem::file_time_type CacheFileProvider::lastWriteTime(const std::string &path) const
{
  return getEntry(path).lastWriteTime;
}

const CachedFileEntry &CacheFileProvider::getEntry(const std::string &path) const
{
  const auto it = findFile(path);
  if (it == cache_.end()) { throw std::runtime_error("File not found in cache: " + path); }
  return it->second;
}

std::string_view CacheFileProvider::mimeType(const std::string &path) const { return getEntry(path).mimeType; }

bool CacheFileProvider::exists(const std::string &path) const noexcept { return findFile(path) != cache_.end(); }

CacheFileProvider::CacheIterator CacheFileProvider::findFile(const std::string &path) const
{
  return cache_.find(normalizePathKey(path));
}
}// namespace phantomchat::utils