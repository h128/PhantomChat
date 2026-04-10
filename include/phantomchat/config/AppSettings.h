#pragma once
#include <string>
#include <vector>
namespace phantomchat::config {
struct AppSettings
{
private:
  AppSettings() = default;

public:
  int listen_port = 8080;
  std::vector<std::string> ice_servers;
  std::string ssl_certificate;
  std::string ssl_certificate_key;
  bool gzip_compression = false;
  int worker_threads = 2;
  std::string web_root_path;
  std::string upload_path;
  std::vector<std::string> cors_allowed_origins = { "*" };

  void load_from_file(const std::string &filename);

  static AppSettings &getInstance()
  {
    static AppSettings instance;
    return instance;
  }
};
}// namespace phantomchat::config