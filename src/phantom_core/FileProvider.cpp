#include <fstream>

#include <phantomchat/IFileProvider.hpp>
#include <sstream>
#include <stdexcept>

std::string FileProvider::readAll(const std::string &path) const
{
  std::ifstream file(path);
  if (!file.is_open()) { throw std::runtime_error("Could not open file: " + path); }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::filesystem::file_time_type FileProvider::lastWriteTime(const std::string &path) const
{
  return std::filesystem::last_write_time(path);
}