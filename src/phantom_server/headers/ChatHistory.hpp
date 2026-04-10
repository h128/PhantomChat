#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <phantomchat/config/AppSettings.h>
#include <sstream>

namespace phantomchat::utils {
namespace {
  std::mutex &get_room_mutex(const std::string &room_name)
  {
    static std::map<std::string, std::mutex> mutexes;
    static std::mutex global_mutex;
    std::lock_guard<std::mutex> lock(global_mutex);
    return mutexes[room_name];// ← returns ref to mutex inside map
  }

}// namespace

void append_event_to_history(const std::string &room_name, const std::string &json)
{
  namespace fs = std::filesystem;
  namespace cfg = phantomchat::config;
  const fs::path storage_dir = fs::path(cfg::AppSettings::getInstance().upload_path) / room_name;
  const fs::path file_path = storage_dir / (room_name + ".ndjson");
  std::lock_guard<std::mutex> lock(get_room_mutex(room_name));

  fs::create_directories(storage_dir);

  std::ofstream out(file_path, std::ios::out | std::ios::app);
  if (!out) { return; }
  out << json << '\n';
  out.close();
}

}// namespace phantomchat::utils
