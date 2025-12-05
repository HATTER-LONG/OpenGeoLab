cmake_minimum_required(VERSION 3.14...3.22)

project(OpenGeoLabTests LANGUAGES CXX)

# ==============================================================================
# Test Options
# ==============================================================================

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)

# ==============================================================================
# CMake Tools and Dependencies
# ==============================================================================

# Only include CPM if not already included by parent project
if(NOT COMMAND CPMAddPackage)
  include(../cmake/tools.cmake)
  include(../cmake/CPM.cmake)
endif()

# doctest: Fast C++ testing framework
CPMAddPackage("gh:doctest/doctest@2.4.12")

# Format.cmake: Auto-formatting support
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")

# Kangaroo: Only add if not already available from parent project
if(NOT TARGET Kangaroo::Kangaroo)
  set(LOCAL_KANGAROO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Kangaroo")
  if(EXISTS ${LOCAL_KANGAROO_PATH})
    message(STATUS "Found local Kangaroo at ${LOCAL_KANGAROO_PATH}, using local version")
    CPMAddPackage(
      NAME Kangaroo
      SOURCE_DIR ${LOCAL_KANGAROO_PATH}
      OPTIONS "BUILD_SHARED_LIBS ON"
    )
  else()
    message(STATUS "Local Kangaroo not found, downloading from GitHub")
    CPMAddPackage(
      NAME Kangaroo
      GITHUB_REPOSITORY HATTER-LONG/Kangaroo
      VERSION 2.2.1
      OPTIONS "BUILD_SHARED_LIBS ON"
    )
  endif()
endif()

# ==============================================================================
# Test Sources Collection
# ==============================================================================

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

# ==============================================================================
# Test Executable
# ==============================================================================

add_executable(${PROJECT_NAME} ${test_sources})

target_link_libraries(${PROJECT_NAME} PRIVATE doctest::doctest Kangaroo::Kangaroo fmt::fmt)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(
  ${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include"
                          "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

# ==============================================================================
# Compiler Warnings
# ==============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra)
  # Allow Microsoft extensions when using Clang on Windows
  if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-language-extension-token)
  endif()
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  target_compile_definitions(${PROJECT_NAME} PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

# ==============================================================================
# Test Discovery
# ==============================================================================

enable_testing()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${PROJECT_NAME})

# ==============================================================================
# Code Coverage (optional)
# ==============================================================================

if(ENABLE_TEST_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
    target_link_options(${PROJECT_NAME} PRIVATE -fprofile-arcs -ftest-coverage)
  endif()
endif()
