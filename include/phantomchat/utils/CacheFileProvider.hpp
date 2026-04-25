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
  std::filesystem::file_time_type last_write_time;
  std::vector<char> bytes;
  std::size_t size;
  std::string_view mime_type;
  std::vector<char> compressed_bytes;
  bool has_compressed = false;
};

class PHANTOM_CORE_EXPORT CacheFileProvider final : public IFileProvider
{
public:
  explicit CacheFileProvider(std::string root_path, bool gzip_compression = false);

  [[nodiscard]] std::vector<char> readAllBytes(const std::string &path) const override;

  [[nodiscard]] const std::vector<char> &readBytesRef(const std::string &path) const;

  [[nodiscard]] const std::vector<char> &readCompressedBytesRef(const std::string &path) const;

  [[nodiscard]] bool hasCompressed(const std::string &path) const noexcept;

  [[nodiscard]] std::uintmax_t size(const std::string &path) const override;

  [[nodiscard]] std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;

  [[nodiscard]] std::string_view mimeType(const std::string &path) const noexcept override;

  [[nodiscard]] bool exists(const std::string &path) const noexcept override;

private:
  using CacheIterator = std::unordered_map<std::string, CachedFileEntry>::const_iterator;
  const CachedFileEntry &getEntry(const std::string &path) const;
  CacheIterator findFile(const std::string &path) const;
  std::string root_path_;
  bool gzip_enabled_;
  std::unordered_map<std::string, CachedFileEntry> cache_;
};
}// namespace phantomchat::utils

#endif
