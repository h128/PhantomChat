#include <algorithm>
#include <cctype>
#include <charconv>
#include <openssl/evp.h>
#include <phantomchat/utils/HelperFunctions.h>

namespace phantomchat::utils {

std::size_t str_to_long(std::string_view sv) noexcept
{
  std::size_t value = 0;
  const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

  if (ec != std::errc{} || ptr == sv.data()) { return 0; }

  return value;
}

std::string url_decode(std::string_view sv) noexcept
{
  constexpr auto hex_val = [](char c) noexcept -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
  };

  std::string result;
  result.reserve(sv.size());
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '%'//
        && i + 2 < sv.size()//
        && std::isxdigit(static_cast<unsigned char>(sv[i + 1]))//
        && std::isxdigit(static_cast<unsigned char>(sv[i + 2]))//
    ) {
      const auto hi = sv[i + 1];
      const auto lo = sv[i + 2];

      const char decoded = static_cast<char>(hex_val(hi) << 4 | hex_val(lo));
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

std::string base64url_encode(std::string_view s) noexcept
{
  if (s.empty()) { return {}; }
  // EVP_EncodeBlock produces standard Base64; we rewrite the two URL-unsafe
  // characters in place and trim trailing '=' padding.
  std::string out(4 * ((s.size() + 2) / 3), '\0');
  auto *out_ptr = reinterpret_cast<unsigned char *>(out.data());
  auto *in_ptr = reinterpret_cast<const unsigned char *>(s.data());

  const int written = EVP_EncodeBlock(out_ptr, in_ptr, static_cast<int>(s.size()));
  out.resize(static_cast<std::size_t>(written));

  for (char &c : out) {
    if (c == '+')
      c = '-';
    else if (c == '/')
      c = '_';
  }

  while (!out.empty() && out.back() == '=') { out.pop_back(); }

  return out;
}


void trim(std::string &s) noexcept
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
}

bool is_safe(std::string_view value) noexcept
{
  if (value.empty()) { return false; }
  return std::all_of(
    value.begin(), value.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
}

bool is_valid_hex(std::string_view value, std::size_t expected_bytes) noexcept
{
  if (value.size() != expected_bytes * 2) { return false; }
  return std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c); });
}

}// namespace phantomchat::utils