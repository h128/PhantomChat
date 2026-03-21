add_executable(intro main.cpp)

target_link_libraries(
  intro
  PRIVATE phantomchat::phantomchat_options
          phantomchat::phantomchat_warnings)

target_link_system_libraries(
  intro
  PRIVATE
          fmt::fmt
          spdlog::spdlog
          )


phantomchat_configure_linker(intro)

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
