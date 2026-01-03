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

# Catch2: Modern C++ testing framework
CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 3.11.0
)

# ==============================================================================
# Qt AUTOMOC Support for Tests
# ==============================================================================

set(CMAKE_AUTOMOC ON)

# ==============================================================================
# Test Sources Collection
# ==============================================================================

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

# Include module sources for testing (with Qt classes needing MOC)
set(module_sources "")

# Include header files for AUTOMOC to find Q_OBJECT macros
set(module_headers "")

# ==============================================================================
# Test Executable
# ==============================================================================

add_executable(${PROJECT_NAME} ${test_sources} ${module_sources} ${module_headers})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Catch2::Catch2WithMain
          Kangaroo::Kangaroo
          HighFive::HighFive
          Qt6::Core
          Qt6::Gui
          nlohmann_json::nlohmann_json
          ${OpenCASCADE_LIBS}
)

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
endif()

# ==============================================================================
# Test Discovery
# ==============================================================================

enable_testing()

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

# ==============================================================================
# Code Coverage (optional)
# ==============================================================================

if(ENABLE_TEST_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
    target_link_options(${PROJECT_NAME} PRIVATE -fprofile-arcs -ftest-coverage)
  endif()
endif()
