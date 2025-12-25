cmake_minimum_required(VERSION 3.14...3.22)

project(
  opengeolab
  VERSION 0.1
  LANGUAGES CXX
)

# ==============================================================================
# C++ Standard Configuration
# ==============================================================================

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Build Options
# ==============================================================================

option(ENABLE_CONSOLE "Enable console window on Windows for debugging" ON)

# ==============================================================================
# Qt Configuration
# ==============================================================================

# Enable QML language server integration for better IDE support
set(QT_QML_GENERATE_QMLLS_INI ON)

# ==============================================================================
# Output Directory Configuration
# ==============================================================================

# Consolidate all runtime and library outputs to bin/ for easier deployment
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Ensure consistent output directory across all build configurations
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# ==============================================================================
# CMake Tools and Package Management
# ==============================================================================

include(./cmake/tools.cmake)
include(./cmake/CPM.cmake)

# ==============================================================================
# External Dependencies
# ==============================================================================

# ------------------------------------------------------------------------------
# CPM-managed Dependencies
# ------------------------------------------------------------------------------

# cxxopts: Command-line argument parsing library
CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.0.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

# Kangaroo: Infrastructure and utility library Check for local version first, otherwise download
# from GitHub
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
    VERSION 2.2.1
    OPTIONS "BUILD_SHARED_LIBS ON"
  )
endif()

# HighFive: Modern C++ interface for HDF5 Requires HDF5 to be pre-installed on the system
CPMAddPackage(
  NAME HighFive
  GITHUB_REPOSITORY highfive-devs/highfive
  VERSION 3.2.0
  OPTIONS "HIGHFIVE_UNIT_TESTS OFF" "HIGHFIVE_EXAMPLES OFF" "HIGHFIVE_BUILD_DOCS OFF"
)

# ------------------------------------------------------------------------------
# Pre-installed Dependencies
# ------------------------------------------------------------------------------

# OpenCASCADE: 3D CAD modeling kernel (must be pre-installed)
find_package(OpenCASCADE REQUIRED)

message(STATUS "Using OpenCASCADE from \"${OpenCASCADE_INSTALL_PREFIX}\"")
message(STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
message(STATUS "OpenCASCADE_LIBRARY_DIR=${OpenCASCADE_LIBRARY_DIR}")
include_directories(${OpenCASCADE_INCLUDE_DIR})
link_directories(${OpenCASCADE_LIBRARY_DIR})

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

# Qt 6: GUI framework with QML support
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick)
qt_standard_project_setup(REQUIRES 6.8)

# ==============================================================================
# Source Files Collection
# ==============================================================================

# Note: Using file globbing is convenient but may not detect new files automatically in all
# generators. Consider explicitly listing files for production builds or when CMake doesn't detect
# changes properly.

file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# ==============================================================================
# Executable Target Configuration
# ==============================================================================

qt_add_executable(${PROJECT_NAME} ${sources} ${headers})

# ==============================================================================
# QML Module Configuration
# ==============================================================================

# Collect all SVG icon resources automatically (using relative paths)
file(GLOB ICON_RESOURCES_ABS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/*.svg")

# Convert absolute paths to relative paths for Qt resource system
set(ICON_RESOURCES "")
foreach(icon_file ${ICON_RESOURCES_ABS})
  file(RELATIVE_PATH icon_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${icon_file}")
  list(APPEND ICON_RESOURCES "${icon_rel}")
endforeach()

# Register QML module with custom OpenGL 3D rendering components Only UI-related files (with
# QML_ELEMENT) need to be listed here
qt_add_qml_module(
  ${PROJECT_NAME}
  URI
  OpenGeoLab
  VERSION
  1.0
  RESOURCE_PREFIX
  /opengeolab
  NO_RESOURCE_TARGET_PATH
  QML_FILES
  resources/qml/Main.qml
  resources/qml/RibbonMenu.qml
  resources/qml/RibbonActions.qml
  RESOURCES
  ${ICON_RESOURCES}
)

# ==============================================================================
# Compiler Options
# ==============================================================================

# Enforce standards conformance on MSVC for better cross-platform compatibility
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# ==============================================================================
# Target Properties
# ==============================================================================

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES # macOS bundle configuration
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE $<NOT:$<BOOL:${ENABLE_CONSOLE}>>
             OUTPUT_NAME OpenGeoLabApp
)

# ==============================================================================
# Link Dependencies
# ==============================================================================

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Qt6::Core
          Qt6::Gui
          Qt6::Qml
          Qt6::Quick
          Kangaroo::Kangaroo
          cxxopts
          fmt::fmt
          HighFive::HighFive
          ${OpenCASCADE_LIBS}
)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
          "${CMAKE_CURRENT_SOURCE_DIR}/include/ui" # For Qt moc generated code
          "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ==============================================================================
# Installation Configuration
# ==============================================================================

include(GNUInstallDirs)

install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ==============================================================================
# Qt Deployment Script Generation
# ==============================================================================

# Generate deployment script for easy application distribution This script handles Qt library and
# plugin deployment
qt_generate_deploy_qml_app_script(
  TARGET
  ${PROJECT_NAME}
  OUTPUT_SCRIPT
  deploy_script
  MACOS_BUNDLE_POST_BUILD
  NO_UNSUPPORTED_PLATFORM_ERROR
  DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)

install(SCRIPT ${deploy_script})

# ==============================================================================
# Testing Configuration
# ==============================================================================

enable_testing()
add_subdirectory(test)
