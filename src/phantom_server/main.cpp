#include <App.h>
#include <fmt/core.h>


int main()
{
  fmt::print("Hello, {}...\n", "PhantomServer");
  uWS::App()
    .get("/*",
      [](auto *res, auto *req) {
        fmt::print("Received request for {}\n", req->getUrl());
        res->writeHeader("Content-Type", "text/plain");
        res->end("Hello from PhantomChat!");
      })
    .listen(8080,
      [](auto *listenSocket) {
        if (listenSocket) {
          std::cout << "Server listening on http://localhost:8080" << std::endl;
        } else {
          std::cerr << "Failed to listen on port 8080" << std::endl;
        }
      })
    .run();

  return 0;
}