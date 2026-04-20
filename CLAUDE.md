# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is

PhantomChat is a lightweight, end-to-end encrypted WebSocket chat server written in C++23. It requires no database — rooms and sessions are ephemeral in-memory. Clients exchange public keys and receive encrypted room keys; all message content is encrypted client-side using libsodium `crypto_box`.

## Build Commands

**Configure:**
```bash
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
```

**Build:**
```bash
cmake --build ./build -j
```

**Run tests:**
```bash
cd ./build && ctest -R "unittests" && cd ../
```

**Docker:**
```bash
docker build -f ./.devcontainer/Dockerfile --tag=phantomchat:latest .
docker run -it phantomchat:latest
```

CMake presets are available via `CMakePresets.json` (requires CMake 3.21+). The top-level build enables sanitizers, clang-tidy, and cppcheck by default; pass `-DENABLE_SANITIZER_ADDRESS=OFF` etc. to disable for faster iteration.

## Architecture

The project is split into two CMake targets:

- **`phantom_core`** (static library) — all business logic: room management, cryptography, Firebase push notifications, JSON contracts, configuration loading, and utility helpers.
- **`phantom_server`** (executable) — WebSocket server setup, HTTP document handling, and static file serving. Thin layer that wires `phantom_core` services to uWebSockets handlers.

### Request Lifecycle

1. Client connects via WebSocket to the `/room` endpoint.
2. Client sends a `JoinOrCreateRoom` JSON request with its public key.
3. `SocketRequestHandler` dispatches the command to `RoomManager`.
4. `RoomManager` (singleton, `shared_mutex`) creates or retrieves a `CryptoRoom`, generates a symmetric room key encrypted with libsodium `crypto_box`, and returns it to the caller.
5. Subsequent `SendMessage` / `SignalCall` commands are routed through the same handler and fanned out to room subscribers via uWebSockets pub/sub.

### Key Source Locations

| Path | Purpose |
|---|---|
| `src/phantom_server/main.cpp` | Entry point; spawns worker threads, wires uWebSockets routes |
| `src/phantom_server/request_handlers/SocketRequestHandler.cpp` | WebSocket message router |
| `src/phantom_core/services/RoomManager.cpp` | Room lifecycle (thread-safe singleton) |
| `src/phantom_core/services/CryptoRoom.cpp` | libsodium key generation and encryption |
| `src/phantom_core/services/Firebase*` | FCM push notification integration (OAuth2 access token + CURL HTTP) |
| `include/phantomchat/contracts/PhantomRequests.h` | JSON protocol contracts (requests, responses, events) |
| `src/phantom_server/appsettings.json` | Runtime config (port, SSL, CORS, worker threads, Firebase) |
| `Dependencies.cmake` | All third-party dependency declarations (CPM/FetchContent) |
| `ProjectOptions.cmake` | Sanitizers, analyzers, hardening, IPO/LTO toggles |

### Threading Model

- Multiple WebSocket listener threads (count from `appsettings.json`).
- Separate background threads for document upload/download processing and event logging.
- Inter-thread communication uses **lock-free queues** (`concurrentqueue`).
- `RoomManager` uses `std::shared_mutex` for concurrent read / exclusive write.

### Configuration

`src/phantom_server/appsettings.json` is loaded at startup. Key fields: `port`, `ssl` (cert/key paths), `cors_origins`, `worker_threads`, `firebase` (service account credentials for push notifications).

## Key Dependencies

| Library | Version | Purpose |
|---|---|---|
| uWebSockets | v20.76.0 | WebSocket server |
| libsodium | cmake wrapper | `crypto_box` E2E encryption |
| nlohmann_json | 3.12.0 | JSON serialization |
| OpenSSL | system | SSL/TLS + HMAC-SHA256 (Firebase auth) |
| CURL | 8.19.0 | Firebase FCM HTTP requests |
| spdlog | 1.17.0 | Logging |
| Catch2 | 3.13.0 | Unit tests |
| concurrentqueue | 1.0.4 | Lock-free inter-thread queues |

## Code Style

- `.clang-format` and `.clang-tidy` configs are present at the repo root — run clang-format before committing.
- C++23 features are in use; do not regress to older standards.
- The project targets GCC and Clang (both tested in CI via `.gitlab-ci.yml`).