#include <algorithm>
#include <curl/curl.h>
#include <fmt/compile.h>
#include <nlohmann/json.hpp>
#include <phantomchat/services/CryptoRoom.h>
#include <phantomchat/services/Firebase.h>
#include <phantomchat/utils/HelperFunctions.h>
#include <stdexcept>
#include <string_view>

namespace phantomchat::services::firebase {
using namespace phantomchat::utils;
using namespace phantomchat::services::crypto_room;
using namespace std::string_view_literals;

static constexpr std::chrono::seconds JWT_LIFETIME{ 3600 };

namespace {
  std::string url_encode(CURL *curl, std::string_view value)
  {
    char *escaped = curl_easy_escape(curl, value.data(), static_cast<int>(value.size()));
    if (!escaped) throw std::runtime_error{ "curl_easy_escape failed" };
    std::string out{ escaped };
    curl_free(escaped);
    return out;
  }

  std::string http_post_form(std::string_view url, std::string_view body)
  {
    constexpr static auto FORM_CONTENT_TYPE = "Content-Type: application/x-www-form-urlencoded"sv;

    constexpr auto curl_deleter = [](CURL *c) noexcept {
      if (c) curl_easy_cleanup(c);
    };
    std::unique_ptr<CURL, decltype(curl_deleter)> curl{ curl_easy_init() };
    if (!curl) throw std::runtime_error{ "curl_easy_init failed" };

    // libcurl requires null-terminated C strings for these options.
    const std::string url_str{ url };
    const std::string body_str{ body };
    const std::string header_str{ FORM_CONTENT_TYPE };

    std::string response;
    std::array<char, CURL_ERROR_SIZE> errbuf{};

    // libcurl write callback: append received bytes to the std::string passed
    constexpr auto write_cb =
      +[](char *ptr, std::size_t size, std::size_t nmemb, void *userdata) noexcept -> std::size_t {
      const std::size_t bytes = size * nmemb;

      static_cast<std::string *>(userdata)->append(ptr, bytes);

      return bytes;
    };

    curl_easy_setopt(curl.get(), CURLOPT_URL, url_str.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body_str.size()));
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, errbuf.data());

    constexpr auto slist_deleter = [](curl_slist *l) noexcept {
      if (l) curl_slist_free_all(l);
    };
    std::unique_ptr<curl_slist, decltype(slist_deleter)> headers{ curl_slist_append(nullptr, header_str.c_str()) };
    if (!headers) throw std::runtime_error{ "curl_slist_append failed" };
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());

    if (const CURLcode rc = curl_easy_perform(curl.get()); rc != CURLE_OK) {
      const char *detail = errbuf[0] ? errbuf.data() : curl_easy_strerror(rc);
      throw std::runtime_error{ fmt::format("HTTP POST failed: {}", detail) };
    }

    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300) {
      throw std::runtime_error{ fmt::format("Token endpoint returned HTTP {}: {}", status, response) };
    }

    return response;
  }

  JwtResult build_jwt(const config::FirebaseSettings &fb)
  {
    static constexpr auto header = R"({"alg":"RS256","typ":"JWT"})"sv;

    const auto now = std::chrono::system_clock::now();
    const auto iat = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto exp = iat + JWT_LIFETIME.count();

    const std::string payload = fmt::format(FMT_COMPILE(R"({{"iss":"{}","scope":"{}","aud":"{}","iat":{},"exp":{}}})"),
      fb.client_email,
      fb.scope,
      fb.token_uri,
      iat,
      exp);

    const std::string header_b64 = base64url_encode(header);
    const std::string payload_b64 = base64url_encode(payload);
    const std::string signing_input = fmt::format(FMT_COMPILE("{}.{}"), header_b64, payload_b64);

    std::vector<unsigned char> sig = rs256_sign(fb.private_key, signing_input);
    const std::string signature_b64 =
      base64url_encode(std::string_view{ reinterpret_cast<const char *>(sig.data()), sig.size() });

    return { .token = fmt::format(FMT_COMPILE("{}.{}"), signing_input, signature_b64), .issued_at = now };
  }


}// anonymous namespace

AccessToken fetch_access_token(const phantomchat::config::FirebaseSettings &fb)
{
  if (fb.private_key.empty()) throw std::invalid_argument{ "firebase_settings.private_key is empty" };
  if (fb.client_email.empty()) throw std::invalid_argument{ "firebase_settings.client_email is empty" };
  if (fb.token_uri.empty()) throw std::invalid_argument{ "firebase_settings.token_uri is empty" };
  static constexpr auto GRANT_TYPE = "urn:ietf:params:oauth:grant-type:jwt-bearer"sv;

  const auto [jwt, issued_at] = build_jwt(fb);

  // url_encode reuses the same easy handle for both values.
  constexpr auto curl_deleter = [](CURL *c) noexcept {
    if (c) curl_easy_cleanup(c);
  };
  std::unique_ptr<CURL, decltype(curl_deleter)> enc_curl{ curl_easy_init() };
  if (!enc_curl) throw std::runtime_error{ "curl_easy_init failed" };

  const std::string body =
    fmt::format("grant_type={}&assertion={}", url_encode(enc_curl.get(), GRANT_TYPE), url_encode(enc_curl.get(), jwt));

  const std::string response = http_post_form(fb.token_uri, body);

  // Response: {"access_token":"...","expires_in":3599 }
  nlohmann::json parsed = nlohmann::json::parse(response);

  if (!parsed.contains("access_token"))
    throw std::runtime_error{ fmt::format("Token response missing access_token: {}", response) };


  // expires_in is optional; fall back to JWT_LIFETIME if absent.
  const auto lifetime = parsed.value("expires_in", JWT_LIFETIME.count());

  return { .token = parsed.at("access_token").get<std::string>(),
    .expires_at = issued_at + std::chrono::seconds{ lifetime } };
}

}// namespace phantomchat::services::firebase
