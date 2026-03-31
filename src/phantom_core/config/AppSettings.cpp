#include "phantomchat/config/AppSettings.h"
#include <phantomchat/utils/JsonSerialization.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace phantomchat::config {

void AppSettings::load_from_file(const std::string &filename)
{
  std::ifstream file(filename);
  if (!file.is_open()) { throw std::runtime_error("Failed to open config file: " + filename); }
  const auto j = nlohmann::json::parse(file);
  from_json(j, *this);
}

}// namespace phantomchat::config