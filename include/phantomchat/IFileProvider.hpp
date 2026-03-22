#ifndef IFileProvider_HPP
#define IFileProvider_HPP

#include <filesystem>
#include <phantomchat/sample_library_export.hpp>

class SAMPLE_LIBRARY_EXPORT IFileProvider
{
public:
  virtual ~IFileProvider() = default;

  virtual std::string readAll(const std::string &path) const = 0;
  virtual std::filesystem::file_time_type lastWriteTime(const std::string &path) const = 0;
};


class SAMPLE_LIBRARY_EXPORT FileProvider final : public IFileProvider
{
public:
  std::string readAll(const std::string &path) const override;

  std::filesystem::file_time_type lastWriteTime(const std::string &path) const override;
};

#endif