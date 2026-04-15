#ifndef PHANTOMCHAT_UTILS_FILEPROVIDER_HPP
#define PHANTOMCHAT_UTILS_FILEPROVIDER_HPP

#include <phantomchat/phantom_core_export.hpp>
#include <phantomchat/utils/IFileProvider.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace phantomchat::utils {
class PHANTOM_CORE_EXPORT FileProvider final : public IFileProvider
{
public:
  std::vector<char> readAllBytes(const std::string &path) const override;

  std::vector<char> compressFile(const std::string &path) const;

  std::uintmax_t size(const std::string &path) const override;

  std::string_view mimeType(const std::string &path) const override;

  std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;

  bool exists(const std::string &path) const override;

private:
  static const inline std::unordered_map<std::string_view, std::string_view> mime_types = {
    { ".html", "text/html; charset=utf-8" },
    { ".htm", "text/html; charset=utf-8" },
    { ".txt", "text/plain; charset=utf-8" },
    { ".css", "text/css; charset=utf-8" },
    { ".js", "application/javascript; charset=utf-8" },
    { ".json", "application/json; charset=utf-8" },
    { ".ndjson", "application/x-ndjson; charset=utf-8" },
    { ".svg", "image/svg+xml" },
    { ".png", "image/png" },
    { ".jpg", "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".gif", "image/gif" },
    { ".webp", "image/webp" },
    { ".ico", "image/x-icon" },
    { ".bmp", "image/bmp" },
    { ".otf", "font/otf" },
    { ".sfnt", "font/sfnt" },
    { ".ttf", "font/ttf" },
    { ".woff", "font/woff" },
    { ".woff2", "font/woff2" },
  };
};
}// namespace phantomchat::utils

#endif
