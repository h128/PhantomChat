#ifndef PHANTOMCHAT_UTILS_CACHEFILEPROVIDER_HPP
#define PHANTOMCHAT_UTILS_CACHEFILEPROVIDER_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <phantomchat/phantom_core_export.hpp>
#include <phantomchat/utils/IFileProvider.hpp>

namespace phantomchat::utils {
struct PHANTOM_CORE_EXPORT CachedFileEntry
{
  std::filesystem::file_time_type lastWriteTime;
  std::vector<char> bytes;
  std::size_t size;
  std::string mimeType;
};

class PHANTOM_CORE_EXPORT CacheFileProvider final : public IFileProvider
{
public:
  explicit CacheFileProvider(std::string rootPath);

  std::vector<char> readAllBytes(const std::string &path) const override;

  std::uintmax_t size(const std::string &path) const override;

  std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;

  std::string_view mimeType(const std::string &path) const override;

  bool exists(const std::string &path) const noexcept override;

private:
  using CacheIterator = std::unordered_map<std::string, CachedFileEntry>::const_iterator;
  const CachedFileEntry &getEntry(const std::string &path) const;
  CacheIterator findFile(const std::string &path) const;
  std::string rootPath_;
  std::unordered_map<std::string, CachedFileEntry> cache_;
};
}// namespace phantomchat::utils

#endif
