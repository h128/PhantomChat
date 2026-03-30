#ifndef PHANTOMCHAT_UTILS_HELPERFUNCTIONS_HPP
#define PHANTOMCHAT_UTILS_HELPERFUNCTIONS_HPP
#include <cstddef>
#include <string>
#include <string_view>
namespace phantomchat::utils {

std::size_t str_to_long(std::string_view sv);
std::string url_decode(std::string_view sv);
void trim(std::string &s);
bool is_safe(const std::string &value);

}// namespace phantomchat::utils
#endif