#include <catch2/catch_test_macros.hpp>
#include <phantomchat/utils/FileProvider.hpp>

using namespace phantomchat::utils;

// ---------------------------------------------------------------------------
// mimeType — known extensions
// ---------------------------------------------------------------------------
TEST_CASE("mimeType returns correct type for html", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("index.html") == "text/html; charset=utf-8");
  REQUIRE(fp.mimeType("index.htm") == "text/html; charset=utf-8");
}

TEST_CASE("mimeType returns correct type for css and js", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("style.css") == "text/css; charset=utf-8");
  REQUIRE(fp.mimeType("app.js") == "application/javascript; charset=utf-8");
}

TEST_CASE("mimeType returns correct type for json and ndjson", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("data.json") == "application/json; charset=utf-8");
  REQUIRE(fp.mimeType("stream.ndjson") == "application/x-ndjson; charset=utf-8");
}

TEST_CASE("mimeType returns correct type for image formats", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("photo.jpg") == "image/jpeg");
  REQUIRE(fp.mimeType("photo.jpeg") == "image/jpeg");
  REQUIRE(fp.mimeType("anim.gif") == "image/gif");
  REQUIRE(fp.mimeType("logo.png") == "image/png");
  REQUIRE(fp.mimeType("icon.bmp") == "image/bmp");
  REQUIRE(fp.mimeType("icon.ico") == "image/x-icon");
  REQUIRE(fp.mimeType("image.svg") == "image/svg+xml");
  REQUIRE(fp.mimeType("image.webp") == "image/webp");
}

TEST_CASE("mimeType returns correct type for font formats", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("font.otf") == "font/otf");
  REQUIRE(fp.mimeType("font.ttf") == "font/ttf");
  REQUIRE(fp.mimeType("font.woff") == "font/woff");
  REQUIRE(fp.mimeType("font.woff2") == "font/woff2");
  REQUIRE(fp.mimeType("font.sfnt") == "font/sfnt");
}

TEST_CASE("mimeType returns correct type for txt", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("readme.txt") == "text/plain; charset=utf-8");
}

// ---------------------------------------------------------------------------
// mimeType — case insensitivity
// ---------------------------------------------------------------------------
TEST_CASE("mimeType is case-insensitive for extensions", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("index.HTML") == "text/html; charset=utf-8");
  REQUIRE(fp.mimeType("style.CSS") == "text/css; charset=utf-8");
  REQUIRE(fp.mimeType("logo.PNG") == "image/png");
  REQUIRE(fp.mimeType("photo.JPG") == "image/jpeg");
  REQUIRE(fp.mimeType("app.JS") == "application/javascript; charset=utf-8");
}

// ---------------------------------------------------------------------------
// mimeType — path handling
// ---------------------------------------------------------------------------
TEST_CASE("mimeType uses only the final extension for multi-dot paths", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("app.min.js") == "application/javascript; charset=utf-8");
  REQUIRE(fp.mimeType("archive.tar.gz") == "application/octet-stream");
}

TEST_CASE("mimeType works with full paths", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("/var/www/html/index.html") == "text/html; charset=utf-8");
  REQUIRE(fp.mimeType("/static/assets/logo.png") == "image/png");
}

// ---------------------------------------------------------------------------
// mimeType — unknown / missing extension fallback
// ---------------------------------------------------------------------------
TEST_CASE("mimeType falls back to octet-stream for unknown extension", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("archive.zip") == "application/octet-stream");
  REQUIRE(fp.mimeType("binary.exe") == "application/octet-stream");
  REQUIRE(fp.mimeType("data.xyz") == "application/octet-stream");
}

TEST_CASE("mimeType falls back to octet-stream when there is no extension", "[fileprovider][mimetype]")
{
  FileProvider fp;
  REQUIRE(fp.mimeType("Makefile") == "application/octet-stream");
  REQUIRE(fp.mimeType("") == "application/octet-stream");
}
