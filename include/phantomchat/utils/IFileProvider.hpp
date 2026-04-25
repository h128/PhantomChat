#ifndef PHANTOMCHAT_UTILS_IFILEPROVIDER_HPP
#define PHANTOMCHAT_UTILS_IFILEPROVIDER_HPP

#include <cstdint>
#include <filesystem>
#include <phantomchat/phantom_core_export.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace phantomchat::utils {
class PHANTOM_CORE_EXPORT IFileProvider
{
public:
  virtual ~IFileProvider() = default;

  [[nodiscard]] virtual std::vector<char> readAllBytes(const std::string &path) const = 0;
  [[nodiscard]] virtual std::uintmax_t size(const std::string &path) const = 0;
  [[nodiscard]] virtual std::string_view mimeType(const std::string &path) const noexcept = 0;
  [[nodiscard]] virtual std::filesystem::file_time_type lastWriteTime(const std::string &path) const = 0;
  [[nodiscard]] virtual bool exists(const std::string &path) const noexcept = 0;
};
}// namespace phantomchat::utils

#endif
