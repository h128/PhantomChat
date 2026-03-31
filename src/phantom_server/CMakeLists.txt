add_executable(PhantomServer
  main.cpp
  processors/DocumentProcessor.cpp
  request_handlers/StaticFileHandler.cpp
  request_handlers/DocumentHandler.cpp
  request_handlers/SocketRequestHandler.cpp)

target_link_libraries(
  PhantomServer
  PRIVATE phantomchat::phantom_core
          phantomchat::phantomchat_options
          phantomchat::phantomchat_warnings)

target_link_system_libraries(
  PhantomServer
  PRIVATE
          fmt::fmt
          spdlog::spdlog
          nlohmann_json::nlohmann_json
          uWS
          concurrentqueue
          )


phantomchat_configure_linker(PhantomServer)

target_include_directories(PhantomServer PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
