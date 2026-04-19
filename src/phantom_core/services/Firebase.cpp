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

  std::string http_post_form(std::string_view url,
    std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
  {
    auto curl_deleter = [](CURL *c) { curl_easy_cleanup(c); };
    std::unique_ptr<CURL, decltype(curl_deleter)> curl{ curl_easy_init() };

    if (!curl) throw std::runtime_error{ "curl_easy_init failed" };

    // Build the MIME form
    auto mime_deleter = [](curl_mime *m) { curl_mime_free(m); };
    std::unique_ptr<curl_mime, decltype(mime_deleter)> mime{ curl_mime_init(curl.get()) };
    if (!mime) throw std::runtime_error{ "curl_mime_init failed" };

    for (const auto &[name, value] : fields) {
      curl_mimepart *part = curl_mime_addpart(mime.get());
      if (!part) throw std::runtime_error{ "curl_mime_addpart failed" };
      curl_mime_name(part, name.data());
      curl_mime_data(part, value.data(), value.size());
    }

    std::string response{};

    constexpr auto write_cb =
      +[](char *ptr, std::size_t size, std::size_t nmemb, void *userdata) noexcept -> std::size_t {
      const std::size_t bytes = size * nmemb;
      static_cast<std::string *>(userdata)->append(ptr, bytes);// std::string response
      return bytes;
    };

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.data());
    curl_easy_setopt(curl.get(), CURLOPT_MIMEPOST, mime.get());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);

    if (const CURLcode rc = curl_easy_perform(curl.get()); rc != CURLE_OK)
      throw std::runtime_error{ fmt::format("HTTP POST failed: {}", curl_easy_strerror(rc)) };

    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300)
      throw std::runtime_error{ fmt::format("Token endpoint returned HTTP {}: {}", status, response) };

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

    std::vector<unsigned char> signature = rs256_sign(signing_input, fb.private_key);
    const std::string signature_b64 =
      base64url_encode(std::string_view{ reinterpret_cast<const char *>(signature.data()), signature.size() });

    return { .token = fmt::format(FMT_COMPILE("{}.{}"), signing_input, signature_b64), .issued_at = now };
  }


}// anonymous namespace

AccessToken fetch_access_token(const phantomchat::config::FirebaseSettings &fb)
{
  if (fb.private_key.empty()) throw std::invalid_argument{ "firebase_settings.private_key is empty" };
  if (fb.client_email.empty()) throw std::invalid_argument{ "firebase_settings.client_email is empty" };
  if (fb.token_uri.empty()) throw std::invalid_argument{ "firebase_settings.token_uri is empty" };

  static constexpr auto GRANT_TYPE = "urn:ietf:params:oauth:grant-type:jwt-bearer"sv;

  const auto &[jwt, issued_at] = build_jwt(fb);

  // MIME handles percent-encoding internally
  const std::string response = http_post_form(fb.token_uri,
    {
      { "grant_type", GRANT_TYPE },
      { "assertion", jwt },
    });

  nlohmann::json parsed = nlohmann::json::parse(response);
  const auto lifetime = parsed.value("expires_in", JWT_LIFETIME.count());

  return { .token = parsed.at("access_token").get<std::string>(),
    .expires_at = issued_at + std::chrono::seconds{ lifetime } };
}

}// namespace phantomchat::services::firebase
