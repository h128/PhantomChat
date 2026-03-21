add_executable(intro main.cpp)

target_link_libraries(
  intro
  PRIVATE PhantomChat::PhantomChat_options
          PhantomChat::PhantomChat_warnings)

target_link_system_libraries(
  intro
  PRIVATE
          fmt::fmt
          spdlog::spdlog
          )


PhantomChat_configure_linker(intro)

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
