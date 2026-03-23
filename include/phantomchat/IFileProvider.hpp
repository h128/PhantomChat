#ifndef IFileProvider_HPP
#define IFileProvider_HPP

#include <filesystem>
#include <phantomchat/phantom_core_export.hpp>

class PHANTOM_CORE_EXPORT IFileProvider
{
public:
  virtual ~IFileProvider() = default;

  virtual std::string readAll(const std::string &path) const = 0;
  virtual std::filesystem::file_time_type lastWriteTime(const std::string &path) const = 0;
};


class PHANTOM_CORE_EXPORT FileProvider final : public IFileProvider
{
public:
  std::string readAll(const std::string &path) const override;

  std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;
};

#endif