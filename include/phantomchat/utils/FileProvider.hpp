#ifndef PHANTOMCHAT_UTILS_FILEPROVIDER_HPP
#define PHANTOMCHAT_UTILS_FILEPROVIDER_HPP

#include <array>
#include <phantomchat/phantom_core_export.hpp>
#include <phantomchat/utils/IFileProvider.hpp>
#include <string>
#include <string_view>

namespace phantomchat::utils {
class PHANTOM_CORE_EXPORT FileProvider final : public IFileProvider
{
public:
  [[nodiscard]] std::vector<char> readAllBytes(const std::string &path) const override;
  [[nodiscard]] std::vector<char> compressFile(const std::string &path) const;
  [[nodiscard]] std::uintmax_t size(const std::string &path) const override;
  [[nodiscard]] std::string_view mimeType(const std::string &path) const noexcept override;
  [[nodiscard]] std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;
  [[nodiscard]] bool exists(const std::string &path) const noexcept override;

private:
  static constexpr std::array<std::pair<std::string_view, std::string_view>, 20> mime_types{
    { // Sorted by extension for binary search
      { ".bmp", "image/bmp" },
      { ".css", "text/css; charset=utf-8" },
      { ".gif", "image/gif" },
      { ".htm", "text/html; charset=utf-8" },
      { ".html", "text/html; charset=utf-8" },
      { ".ico", "image/x-icon" },
      { ".jpeg", "image/jpeg" },// e < p, so jpeg comes before jpg
      { ".jpg", "image/jpeg" },
      { ".js", "application/javascript; charset=utf-8" },
      { ".json", "application/json; charset=utf-8" },
      { ".ndjson", "application/x-ndjson; charset=utf-8" },
      { ".otf", "font/otf" },
      { ".png", "image/png" },
      { ".sfnt", "font/sfnt" },
      { ".svg", "image/svg+xml" },
      { ".ttf", "font/ttf" },
      { ".txt", "text/plain; charset=utf-8" },
      { ".webp", "image/webp" },
      { ".woff", "font/woff" },
      { ".woff2", "font/woff2" } }
  };
};
}// namespace phantomchat::utils

#endif
