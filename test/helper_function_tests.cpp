#include <catch2/catch_test_macros.hpp>
#include <phantomchat/utils/HelperFunctions.h>

using namespace phantomchat::utils;

// ---------------------------------------------------------------------------
// trim
// ---------------------------------------------------------------------------
TEST_CASE("trim removes leading and trailing whitespace", "[helper][trim]")
{
  std::string s = "  hello  ";
  trim(s);
  REQUIRE(s == "hello");
}

TEST_CASE("trim handles tabs and newlines", "[helper][trim]")
{
  std::string s = "\t\n hello \n\t";
  trim(s);
  REQUIRE(s == "hello");
}

TEST_CASE("trim leaves inner spaces untouched", "[helper][trim]")
{
  std::string s = "  hello world  ";
  trim(s);
  REQUIRE(s == "hello world");
}

TEST_CASE("trim on empty string is a no-op", "[helper][trim]")
{
  std::string s;
  trim(s);
  REQUIRE(s.empty());
}

TEST_CASE("trim on already-clean string is a no-op", "[helper][trim]")
{
  std::string s = "clean";
  trim(s);
  REQUIRE(s == "clean");
}

// ---------------------------------------------------------------------------
// is_safe
// ---------------------------------------------------------------------------
TEST_CASE("is_safe accepts alphanumeric characters", "[helper][is_safe]")
{
  REQUIRE(is_safe("user1"));
  REQUIRE(is_safe("RoomAlpha123"));
  REQUIRE(is_safe("abc"));
}

TEST_CASE("is_safe accepts hyphens and underscores", "[helper][is_safe]")
{
  REQUIRE(is_safe("user-1"));
  REQUIRE(is_safe("my_room"));
  REQUIRE(is_safe("hello-world_42"));
}

TEST_CASE("is_safe rejects spaces", "[helper][is_safe]")
{
  REQUIRE_FALSE(is_safe("user 1"));
  REQUIRE_FALSE(is_safe(" leading"));
  REQUIRE_FALSE(is_safe("trailing "));
}

TEST_CASE("is_safe rejects special characters", "[helper][is_safe]")
{
  REQUIRE_FALSE(is_safe("user@domain"));
  REQUIRE_FALSE(is_safe("na/me"));
  REQUIRE_FALSE(is_safe("dot.name"));
  REQUIRE_FALSE(is_safe("bang!"));
}

TEST_CASE("is_safe returns false for empty string", "[helper][is_safe]") { REQUIRE_FALSE(is_safe("")); }

// ---------------------------------------------------------------------------
// url_decode
// ---------------------------------------------------------------------------
TEST_CASE("url_decode decodes percent-encoded ASCII", "[helper][url_decode]")
{
  REQUIRE(url_decode("hello%20world") == "hello world");
  REQUIRE(url_decode("file%2Ename") == "file.name");
}

TEST_CASE("url_decode decodes plus sign as space", "[helper][url_decode]")
{
  REQUIRE(url_decode("hello+world") == "hello world");
}

TEST_CASE("url_decode decodes uppercase hex", "[helper][url_decode]") { REQUIRE(url_decode("%41%42%43") == "ABC"); }

TEST_CASE("url_decode decodes lowercase hex", "[helper][url_decode]") { REQUIRE(url_decode("%61%62%63") == "abc"); }

TEST_CASE("url_decode decodes UTF-8 multi-byte sequence", "[helper][url_decode]")
{
  // سلام in UTF-8 percent-encoded
  REQUIRE(url_decode("%D8%B3%D9%84%D8%A7%D9%85") == "\xD8\xB3\xD9\x84\xD8\xA7\xD9\x85");
}

TEST_CASE("url_decode returns empty on null-byte injection", "[helper][url_decode]")
{
  REQUIRE(url_decode("hello%00world").empty());
}

TEST_CASE("url_decode leaves unencoded strings unchanged", "[helper][url_decode]")
{
  REQUIRE(url_decode("hello") == "hello");
  REQUIRE(url_decode("") == "");
}

TEST_CASE("url_decode passes through incomplete percent sequences", "[helper][url_decode]")
{
  REQUIRE(url_decode("50%") == "50%");
  REQUIRE(url_decode("50%2") == "50%2");
}

// ---------------------------------------------------------------------------
// str_to_long
// ---------------------------------------------------------------------------
TEST_CASE("str_to_long parses valid numbers", "[helper][str_to_long]")
{
  REQUIRE(str_to_long("0") == 0);
  REQUIRE(str_to_long("42") == 42);
  REQUIRE(str_to_long("1048576") == 1048576);
}

TEST_CASE("str_to_long returns 0 for invalid input", "[helper][str_to_long]")
{
  REQUIRE(str_to_long("") == 0);
  REQUIRE(str_to_long("abc") == 0);
  REQUIRE(str_to_long("-1") == 0);
}
