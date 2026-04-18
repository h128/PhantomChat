#ifndef PHANTOMCHAT_UTILS_HELPERFUNCTIONS_HPP
#define PHANTOMCHAT_UTILS_HELPERFUNCTIONS_HPP
#include <cstddef>
#include <string>
#include <string_view>
namespace phantomchat::utils {

std::size_t str_to_long(std::string_view sv);
std::string base64url_encode(std::string_view s);
std::string url_decode(std::string_view sv);
void trim(std::string &s);
bool is_safe(std::string_view sv);
bool is_valid_hex(std::string_view sv, std::size_t expected_bytes);

}// namespace phantomchat::utils
#endif