include(GenerateExportHeader)


add_library(phantom_core CacheFileProvider.cpp FileProvider.cpp PhantomRequest.cpp RoomManager.cpp
 JsonMapper.cpp
 AppSettings.cpp
 HelperFunctions.cpp)



add_library(phantomchat::phantom_core ALIAS phantom_core)

target_link_libraries(phantom_core PRIVATE phantomchat_options phantomchat_warnings)

target_link_system_libraries(phantom_core PUBLIC nlohmann_json::nlohmann_json)
target_link_system_libraries(phantom_core PUBLIC sodium)

target_include_directories(phantom_core ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                                                  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

if (phantomchat_BUILD_FUZZ_TESTS)
  target_link_libraries(phantom_core PRIVATE -fsanitize=fuzzer-no-link)
  target_compile_options(phantom_core PRIVATE -fsanitize=fuzzer-no-link)
endif()

target_compile_features(phantom_core PUBLIC cxx_std_${CMAKE_CXX_STANDARD})

phantomchat_configure_linker(phantom_core)

set_target_properties(
  phantom_core
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(phantom_core EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/phantomchat/phantom_core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(phantom_core PUBLIC PHANTOM_CORE_STATIC_DEFINE)
endif()
