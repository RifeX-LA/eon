find_package(Threads REQUIRED)
find_package(TBB QUIET)
option(EON_TEST_ASAN "Build runtime tests with AddressSanitizer" OFF)
if(EON_TEST_ASAN AND MSVC)
  # MSVC AddressSanitizer is incompatible with the default Debug /RTC1 flag.
  string(REGEX REPLACE "/RTC[1su]+" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()
if(EON_TEST_ASAN)
  # Build GoogleTest with identical STL container annotations on MSVC.
  set(GTest_FOUND FALSE)
  if(MSVC)
    add_compile_options(/fsanitize=address /Zi)
  else()
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  endif()
else()
  find_package(GTest 1.17 CONFIG QUIET)
endif()
if(NOT GTest_FOUND)
  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "Use the same MSVC runtime as eon tests" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(googletest
    URL https://codeload.github.com/google/googletest/zip/refs/tags/v1.17.0
    URL_HASH SHA256=40d4ec942217dcc84a9ebe2a68584ada7d4a33a8ee958755763278ea1c5e18ff
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(googletest)
endif()
include(GoogleTest)
foreach(module concepts utility spark string nullable scope_guard tpl chrono mt lifetime asio)
  add_executable(test_${module} ${module}.cpp)
  target_link_libraries(test_${module} PRIVATE eon Threads::Threads GTest::gtest_main)
  if(TARGET TBB::tbb)
    # libstdc++ uses TBB for the parallel standard algorithms exercised by tests.
    target_link_libraries(test_${module} PRIVATE TBB::tbb)
  endif()
  if(EON_TEST_ASAN)
    if(MSVC)
      target_link_options(test_${module} PRIVATE /INCREMENTAL:NO)
    else()
      target_link_options(test_${module} PRIVATE -fsanitize=address)
    endif()
  endif()
  if(MSVC)
    target_compile_options(test_${module} PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(test_${module} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  gtest_discover_tests(test_${module} DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)
endforeach()

find_path(EON_BOOST_INCLUDE_DIR boost/asio.hpp
  HINTS ${Boost_INCLUDE_DIRS} "$ENV{BOOST_ROOT}" PATH_SUFFIXES include REQUIRED)
target_include_directories(test_asio SYSTEM PRIVATE "${EON_BOOST_INCLUDE_DIR}")
# A Boost installation prefix may also contain another version of GoogleTest.
# Keep headers consistent with the GTest target that we actually link.
target_include_directories(test_asio BEFORE PRIVATE
  "$<TARGET_PROPERTY:GTest::gtest,INTERFACE_INCLUDE_DIRECTORIES>")
target_compile_definitions(test_asio PRIVATE BOOST_ALL_NO_LIB)
if(WIN32)
  target_link_libraries(test_asio PRIVATE ws2_32 mswsock)
endif()

target_sources(test_lifetime PRIVATE lifetime_peer.cpp)

# Compile every header independently: test.hpp must not hide missing includes.
file(GLOB_RECURSE eon_headers RELATIVE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/*.hpp")
set(header_checks)
foreach(header IN LISTS eon_headers)
  if(header MATCHES "^(asio|chrono|concepts|lifetime|mt|nullable|scope_guard|spark|tpl|utility)(/|\\.hpp$)")
    string(REPLACE "/" "_" check_name "${header}")
    set(check_file "${CMAKE_CURRENT_BINARY_DIR}/headers/${check_name}.cpp")
    file(WRITE "${check_file}" "#include <eon/${header}>\n")
    list(APPEND header_checks "${check_file}")
  endif()
endforeach()
add_library(test_header_checks OBJECT ${header_checks})
target_link_libraries(test_header_checks PRIVATE eon)
target_include_directories(test_header_checks SYSTEM PRIVATE "${EON_BOOST_INCLUDE_DIR}")
target_compile_definitions(test_header_checks PRIVATE BOOST_ALL_NO_LIB)

# Build every test and the compile-time checks before running the whole suite.
set(eon_test_targets test_header_checks)
foreach(module concepts utility spark string nullable scope_guard tpl chrono mt lifetime asio)
  list(APPEND eon_test_targets test_${module})
endforeach()
add_custom_target(check
  COMMAND "${CMAKE_CTEST_COMMAND}" -C "$<CONFIG>" --output-on-failure --no-tests=error
  DEPENDS ${eon_test_targets}
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
  USES_TERMINAL
  COMMENT "Running all eon tests")
