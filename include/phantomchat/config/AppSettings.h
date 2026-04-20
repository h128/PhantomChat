#pragma once
#include <string>
#include <vector>
namespace phantomchat::config {

struct FirebaseSettings
{
  bool enabled = false;
  int min_push_interval_seconds = 60;
  std::string type;
  std::string scope;
  std::string project_id;
  std::string private_key_id;
  std::string private_key;
  std::string client_email;
  std::string client_id;
  std::string auth_uri;
  std::string token_uri;
  std::string auth_provider_x509_cert_url;
  std::string client_x509_cert_url;
  std::string universe_domain;
};

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
  FirebaseSettings firebase_settings;

  void load_from_file(const std::string &filename);

  static AppSettings &getInstance()
  {
    static AppSettings instance;
    return instance;
  }
};
}// namespace phantomchat::config