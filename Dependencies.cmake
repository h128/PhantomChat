include(cmake/CPM.cmake)
include(FetchContent)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(phantomchat_setup_dependencies)
  # For each dependency, see if it's
  # already been provided to us by a parent project
  if(NOT TARGET fmtlib::fmtlib)
    cpmaddpackage("gh:fmtlib/fmt#12.1.0")
  endif()

  if(NOT TARGET spdlog::spdlog)
    cpmaddpackage(
      NAME
      spdlog
      VERSION
      1.17.0
      GITHUB_REPOSITORY
      "gabime/spdlog"
      OPTIONS
      "SPDLOG_FMT_EXTERNAL ON")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.12.0")
  endif()

  # If zlib already exists (system or previous FetchContent), use it
  find_package(ZLIB QUIET)

  if(ZLIB_FOUND)
    message(STATUS "Using existing ZLIB: ${ZLIB_INCLUDE_DIRS}")
    add_library(myZlib INTERFACE IMPORTED)
    target_include_directories(myZlib INTERFACE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(myZlib INTERFACE ${ZLIB_LIBRARIES})
  else()
    # Otherwise, fetch and build zlib manually
    FetchContent_Declare(
      zlib_content
      GIT_REPOSITORY https://github.com/madler/zlib.git
      GIT_TAG v1.3.2
    )
    FetchContent_MakeAvailable(zlib_content)

    file(GLOB_RECURSE zlibSources CONFIGURE_DEPENDS
      ${zlib_content_SOURCE_DIR}/*.c
    )
    list(FILTER zlibSources EXCLUDE REGEX ".*/contrib/.*")

    add_library(myZlib STATIC ${zlibSources})
    target_include_directories(myZlib PUBLIC ${zlib_content_SOURCE_DIR})
  endif()

  if(NOT TARGET uSockets)
    FetchContent_Declare(
      uSockets_content
      GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
      GIT_TAG v0.8.8
    )
    FetchContent_MakeAvailable(uSockets_content)

    # If FetchContent failed or variable is empty, set manually
    if(NOT uSockets_content_SOURCE_DIR OR uSockets_content_SOURCE_DIR STREQUAL "")
      set(uSockets_content_SOURCE_DIR "build/_deps/usockets_content-src")
      set(uSockets_content_BINARY_DIR "build/_deps/usockets_content-build")
    endif()

    message(STATUS "uSockets source dir: ${uSockets_content_SOURCE_DIR}")

    file(GLOB_RECURSE uSocketsSources ${uSockets_content_SOURCE_DIR}/src/*.c)
    add_library(uSockets STATIC ${uSocketsSources})
    target_include_directories(uSockets PUBLIC ${uSockets_content_SOURCE_DIR}/src)
    target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL) # Disable SSL if not needed
  endif()

  if(NOT TARGET uWS)
    FetchContent_Declare(
      uWebSockets_content
      GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
      GIT_TAG v20.76.0
    )
    FetchContent_MakeAvailable(uWebSockets_content)

    if(NOT uWebSockets_content_SOURCE_DIR OR uWebSockets_content_SOURCE_DIR STREQUAL "")
      set(uWebSockets_content_SOURCE_DIR "build/_deps/uwebsockets_content-src")
      set(uWebSockets_content_BINARY_DIR "build/_deps/uwebsockets_content-build")
    endif()

    # Create interface library for uWebSockets (header-only)
    add_library(uWS INTERFACE)
    target_include_directories(uWS INTERFACE ${uWebSockets_content_SOURCE_DIR}/src)
    target_link_libraries(uWS INTERFACE uSockets myZlib)
  endif()
endfunction()
