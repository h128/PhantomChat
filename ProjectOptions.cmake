include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(phantomchat_supports_sanitizers)
  # Emscripten doesn't support sanitizers
  if(EMSCRIPTEN)
    set(SUPPORTS_UBSAN OFF)
    set(SUPPORTS_ASAN OFF)
  elseif((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(phantomchat_setup_options)
  option(phantomchat_ENABLE_HARDENING "Enable hardening" ON)
  option(phantomchat_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    phantomchat_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    phantomchat_ENABLE_HARDENING
    OFF)

  phantomchat_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR phantomchat_PACKAGING_MAINTAINER_MODE)
    option(phantomchat_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(phantomchat_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(phantomchat_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(phantomchat_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(phantomchat_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(phantomchat_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(phantomchat_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(phantomchat_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(phantomchat_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(phantomchat_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(phantomchat_ENABLE_PCH "Enable precompiled headers" OFF)
    option(phantomchat_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(phantomchat_ENABLE_IPO "Enable IPO/LTO" ON)
    option(phantomchat_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(phantomchat_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(phantomchat_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(phantomchat_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(phantomchat_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(phantomchat_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(phantomchat_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(phantomchat_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(phantomchat_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(phantomchat_ENABLE_PCH "Enable precompiled headers" OFF)
    option(phantomchat_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(phantomchat_ENABLE_SANITIZER_ADDRESS OFF CACHE BOOL "Enable address sanitizer" FORCE)
    set(phantomchat_ENABLE_SANITIZER_LEAK OFF CACHE BOOL "Enable leak sanitizer" FORCE)
    set(phantomchat_ENABLE_SANITIZER_UNDEFINED OFF CACHE BOOL "Enable undefined sanitizer" FORCE)
    set(phantomchat_ENABLE_SANITIZER_THREAD OFF CACHE BOOL "Enable thread sanitizer" FORCE)
    set(phantomchat_ENABLE_SANITIZER_MEMORY OFF CACHE BOOL "Enable memory sanitizer" FORCE)
    set(phantomchat_ENABLE_CLANG_TIDY OFF CACHE BOOL "Enable clang-tidy" FORCE)
    set(phantomchat_ENABLE_CPPCHECK OFF CACHE BOOL "Enable cpp-check analysis" FORCE)
    set(phantomchat_ENABLE_COVERAGE OFF CACHE BOOL "Enable coverage reporting" FORCE)
    set(phantomchat_ENABLE_CACHE OFF CACHE BOOL "Enable ccache" FORCE)
    set(phantomchat_BUILD_FUZZ_TESTS OFF CACHE BOOL "Enable fuzz testing executable" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "Enable testing" FORCE)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      phantomchat_ENABLE_IPO
      phantomchat_WARNINGS_AS_ERRORS
      phantomchat_ENABLE_SANITIZER_ADDRESS
      phantomchat_ENABLE_SANITIZER_LEAK
      phantomchat_ENABLE_SANITIZER_UNDEFINED
      phantomchat_ENABLE_SANITIZER_THREAD
      phantomchat_ENABLE_SANITIZER_MEMORY
      phantomchat_ENABLE_UNITY_BUILD
      phantomchat_ENABLE_CLANG_TIDY
      phantomchat_ENABLE_CPPCHECK
      phantomchat_ENABLE_COVERAGE
      phantomchat_ENABLE_PCH
      phantomchat_ENABLE_CACHE)
  endif()

  phantomchat_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (phantomchat_ENABLE_SANITIZER_ADDRESS OR phantomchat_ENABLE_SANITIZER_THREAD OR phantomchat_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(phantomchat_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(phantomchat_global_options)
  if(phantomchat_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    phantomchat_enable_ipo()
  endif()

  phantomchat_supports_sanitizers()

  if(phantomchat_ENABLE_HARDENING AND phantomchat_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    elseif(NOT SUPPORTS_UBSAN 
       OR phantomchat_ENABLE_SANITIZER_UNDEFINED
       OR phantomchat_ENABLE_SANITIZER_ADDRESS
       OR phantomchat_ENABLE_SANITIZER_THREAD
       OR phantomchat_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${phantomchat_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${phantomchat_ENABLE_SANITIZER_UNDEFINED}")
    phantomchat_enable_hardening(phantomchat_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(phantomchat_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(phantomchat_warnings INTERFACE)
  add_library(phantomchat_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  phantomchat_set_project_warnings(
    phantomchat_warnings
    ${phantomchat_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  include(cmake/Linker.cmake)
  # Must configure each target with linker options, we're avoiding setting it globally for now

  if(NOT EMSCRIPTEN)
    include(cmake/Sanitizers.cmake)
    phantomchat_enable_sanitizers(
      phantomchat_options
      ${phantomchat_ENABLE_SANITIZER_ADDRESS}
      ${phantomchat_ENABLE_SANITIZER_LEAK}
      ${phantomchat_ENABLE_SANITIZER_UNDEFINED}
      ${phantomchat_ENABLE_SANITIZER_THREAD}
      ${phantomchat_ENABLE_SANITIZER_MEMORY})
  endif()

  set_target_properties(phantomchat_options PROPERTIES UNITY_BUILD ${phantomchat_ENABLE_UNITY_BUILD})

  if(phantomchat_ENABLE_PCH)
    target_precompile_headers(
      phantomchat_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(phantomchat_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    phantomchat_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(phantomchat_ENABLE_CLANG_TIDY)
    phantomchat_enable_clang_tidy(phantomchat_options ${phantomchat_WARNINGS_AS_ERRORS})
  endif()

  if(phantomchat_ENABLE_CPPCHECK)
    phantomchat_enable_cppcheck(${phantomchat_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(phantomchat_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    phantomchat_enable_coverage(phantomchat_options)
  endif()

  if(phantomchat_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(phantomchat_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(phantomchat_ENABLE_HARDENING AND NOT phantomchat_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    elseif(NOT SUPPORTS_UBSAN 
       OR phantomchat_ENABLE_SANITIZER_UNDEFINED
       OR phantomchat_ENABLE_SANITIZER_ADDRESS
       OR phantomchat_ENABLE_SANITIZER_THREAD
       OR phantomchat_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    phantomchat_enable_hardening(phantomchat_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
