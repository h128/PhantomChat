#ifndef PHANTOMCHAT_UTILS_FILEPROVIDER_HPP
#define PHANTOMCHAT_UTILS_FILEPROVIDER_HPP

#include <phantomchat/phantom_core_export.hpp>
#include <phantomchat/utils/IFileProvider.hpp>
#include <string>
#include <string_view>

namespace phantomchat::utils {
class PHANTOM_CORE_EXPORT FileProvider final : public IFileProvider
{
public:
  std::vector<char> readAllBytes(const std::string &path) const override;

  std::uintmax_t size(const std::string &path) const override;

  std::string_view mimeType(const std::string &path) const override;

  std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;

  bool exists(const std::string &path) const override;
};
}// namespace phantomchat::utils

#endif
