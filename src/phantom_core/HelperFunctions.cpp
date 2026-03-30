#include <algorithm>
#include <cctype>
#include <phantomchat/utils/HelperFunctions.h>

namespace phantomchat::utils {

std::size_t str_to_long(std::string_view sv)
{
  if (sv.empty() || sv.front() == '-') { return 0; }
  try {
    return std::stoull(std::string(sv));
  } catch (const std::exception &) {
    return 0;
  }
}

std::string url_decode(std::string_view sv)
{
  std::string result;
  result.reserve(sv.size());
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '%' && i + 2 < sv.size() && std::isxdigit(static_cast<unsigned char>(sv[i + 1]))
        && std::isxdigit(static_cast<unsigned char>(sv[i + 2]))) {
      const auto hi = sv[i + 1];
      const auto lo = sv[i + 2];
      auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return c - 'A' + 10;
      };
      const char decoded = static_cast<char>(hexVal(hi) << 4 | hexVal(lo));
      if (decoded == '\0') { return {}; }// Reject null-byte injection
      result += decoded;
      i += 2;
    } else if (sv[i] == '+') {
      result += ' ';
    } else {
      result += sv[i];
    }
  }
  return result;
}

void trim(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
}

bool is_safe(const std::string &value)
{
  if (value.empty()) { return false; }
  return std::all_of(
    value.begin(), value.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
}

}// namespace phantomchat::utils