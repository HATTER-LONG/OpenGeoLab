cmake_minimum_required(VERSION 3.14...3.22)

project(
  opengeolab
  VERSION 0.1
  LANGUAGES CXX
)

if(POLICY CMP0207)
  cmake_policy(SET CMP0207 NEW)
endif()

include(GNUInstallDirs)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_CONSOLE "Enable console window on Windows for debugging" ON)
option(ENABLE_TEST "Enable building tests" ON)
option(ENABLE_HDF5 "Enable HDF5 support" OFF)
option(OPENGEOLAB_FETCH_DEPENDENCIES "Fetch missing dependencies with CPM" ON)
option(OPENGEOLAB_BUILD_SHARED_LIBS "Build first-party OpenGeoLab libraries as shared libraries"
       OFF
)

set(OPENGEOLAB_LIBRARY_TYPE STATIC)
if(OPENGEOLAB_BUILD_SHARED_LIBS)
  set(OPENGEOLAB_LIBRARY_TYPE SHARED)
endif()

set(QT_QML_GENERATE_QMLLS_INI ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

set(OPENGEOLAB_FMT_VERSION 12.1.0)
set(OPENGEOLAB_SPDLOG_VERSION 1.17.0)
set(OPENGEOLAB_KANGAROO_VERSION 2.2.2)
set(OPENGEOLAB_NLOHMANN_JSON_VERSION 3.12.0)
set(OPENGEOLAB_PYBIND11_VERSION 3.0.2)
set(OPENGEOLAB_CATCH2_VERSION 3.13.0)
set(PYBIND11_FINDPYTHON ON)

opengeolab_resolve_package(
  fmt
  fmt::fmt
  VERSION
  ${OPENGEOLAB_FMT_VERSION}
  GITHUB_REPOSITORY
  fmtlib/fmt
  GIT_TAG
  ${OPENGEOLAB_FMT_VERSION}
  CPM_OPTIONS
  "FMT_INSTALL YES"
)

opengeolab_resolve_package(
  spdlog
  spdlog::spdlog
  VERSION
  ${OPENGEOLAB_SPDLOG_VERSION}
  GITHUB_REPOSITORY
  gabime/spdlog
  GIT_TAG
  v${OPENGEOLAB_SPDLOG_VERSION}
  CPM_OPTIONS
  "SPDLOG_FMT_EXTERNAL YES"
  "SPDLOG_INSTALL YES"
)

set(LOCAL_KANGAROO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Kangaroo")
if(EXISTS ${LOCAL_KANGAROO_PATH})
  message(STATUS "Found local Kangaroo at ${LOCAL_KANGAROO_PATH}, using local version")
  CPMAddPackage(
    NAME Kangaroo
    SOURCE_DIR ${LOCAL_KANGAROO_PATH}
    OPTIONS "BUILD_SHARED_LIBS ON"
  )
else()
  opengeolab_resolve_package(
    Kangaroo
    Kangaroo::Kangaroo
    VERSION
    ${OPENGEOLAB_KANGAROO_VERSION}
    SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../Kangaroo
    GITHUB_REPOSITORY
    HATTER-LONG/Kangaroo
    GIT_TAG
    v${OPENGEOLAB_KANGAROO_VERSION}
    CPM_OPTIONS
    "KANGAROO_FETCH_DEPENDENCIES OFF"
    "BUILD_SHARED_LIBS ON"
  )
endif()

opengeolab_resolve_package(
  nlohmann_json
  nlohmann_json::nlohmann_json
  VERSION
  ${OPENGEOLAB_NLOHMANN_JSON_VERSION}
  GITHUB_REPOSITORY
  nlohmann/json
  GIT_TAG
  v${OPENGEOLAB_NLOHMANN_JSON_VERSION}
  CPM_OPTIONS
  "JSON_BuildTests OFF"
)

opengeolab_resolve_package(
  pybind11
  pybind11::pybind11
  VERSION
  ${OPENGEOLAB_PYBIND11_VERSION}
  GITHUB_REPOSITORY
  pybind/pybind11
  GIT_TAG
  v${OPENGEOLAB_PYBIND11_VERSION}
)

find_package(OpenCASCADE REQUIRED)

message(STATUS "Using OpenCASCADE from \"${OpenCASCADE_INSTALL_PREFIX}\"")
message(STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
message(STATUS "OpenCASCADE_LIBRARY_DIR=${OpenCASCADE_LIBRARY_DIR}")

set(OpenCASCADE_LIBS
    TKRWMesh
    TKBinXCAF
    TKBin
    TKBinL
    TKXCAF
    TKVCAF
    TKCAF
    TKMesh
    TKShHealing
    TKPrim
    TKTopAlgo
    TKGeomAlgo
    TKBRep
    TKXSBase
    TKDESTEP
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKLCAF
    TKCDF
    TKernel
)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick OpenGL LinguistTools)
qt_standard_project_setup(REQUIRES 6.8)

find_package(gmsh REQUIRED)

set(OPENGEOLAB_GMSH_TARGET)
if(TARGET gmsh::gmsh)
  set(OPENGEOLAB_GMSH_TARGET gmsh::gmsh)
elseif(TARGET gmsh)
  set(OPENGEOLAB_GMSH_TARGET gmsh)
endif()

if(ENABLE_TEST)
  enable_testing()

  opengeolab_resolve_package(
    Catch2
    Catch2::Catch2WithMain
    VERSION
    ${OPENGEOLAB_CATCH2_VERSION}
    GITHUB_REPOSITORY
    catchorg/Catch2
    GIT_TAG
    v${OPENGEOLAB_CATCH2_VERSION}
    CPM_OPTIONS
    "CATCH_BUILD_TESTING OFF"
    "CATCH_INSTALL_DOCS OFF"
  )
endif()

add_subdirectory(libs)
add_subdirectory(python)
add_subdirectory(apps)
