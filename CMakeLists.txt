cmake_minimum_required(VERSION 3.14...3.22)

project(
  OpenGeoLab
  VERSION 0.1
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(QT_QML_GENERATE_QMLLS_INI ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# --- Import tools ----

include(./cmake/tools.cmake)

# ---- Dependencies ----

include(./cmake/CPM.cmake)

# Parse command line options
CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.0.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

# Kangaroo is infra tools library. Check if Kangaroo exists locally, if not download from GitHub
set(LOCAL_KANGAROO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Kangaroo")
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
    VERSION 2.1.1
    OPTIONS "BUILD_SHARED_LIBS ON"
  )
endif()

# qwindowkit is a Qt abstraction library for frameless window
CPMAddPackage(
  NAME qwindowkit
  GITHUB_REPOSITORY "stdware/qwindowkit"
  GIT_TAG 1.4.0
  OPTIONS "QWINDOWKIT_BUILD_WIDGETS OFF" "QWINDOWKIT_BUILD_EXAMPLES OFF"
          "QWINDOWKIT_BUILD_TESTS OFF" "QWINDOWKIT_BUILD_QUICK ON"
)

# Qt6 setup
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.8)

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

qt_add_executable(${PROJECT_NAME} ${sources} ${headers})

qt_add_resources(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/source/resource/shared.qrc)
qt_add_qml_module(
  ${PROJECT_NAME}
  URI
  ${PROJECT_NAME}
  VERSION
  1.0
  OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/qml/${PROJECT_NAME}
  QML_FILES
  source/Main.qml
  source/widgets/BarButton.qml
  source/widgets/FramelessWindow.qml
)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE
)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE Qt6::Quick Kangaroo::Kangaroo cxxopts fmt::fmt QWindowKit::Quick
)

target_include_directories(
  ${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
                          "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

include(GNUInstallDirs)
install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
