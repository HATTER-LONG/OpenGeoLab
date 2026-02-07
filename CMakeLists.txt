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
option(ENABLE_TEST "Enable building tests" OFF)
option(ENABLE_HDF5 "Enable HDF5 support" OFF)
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
    VERSION 2.2.2
    OPTIONS "BUILD_SHARED_LIBS ON"
  )
endif()

# HighFive: Modern C++ interface for HDF5 Requires HDF5 to be pre-installed on the system
if(ENABLE_HDF5)
  CPMAddPackage(
    NAME HighFive
    GITHUB_REPOSITORY highfive-devs/highfive
    VERSION 3.2.0
    OPTIONS "HIGHFIVE_UNIT_TESTS OFF" "HIGHFIVE_EXAMPLES OFF" "HIGHFIVE_BUILD_DOCS OFF"
  )
endif()

CPMAddPackage("gh:nlohmann/json@3.12.0")

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
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick OpenGL)
qt_standard_project_setup(REQUIRES 6.8)

find_package(gmsh REQUIRED)

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

# QML singletons must be declared for the generated qmldir/type system.
set_source_files_properties(
  resources/qml/Theme.qml resources/qml/MainPages.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

# Collect icon resources automatically (using relative paths)
file(GLOB ICON_RESOURCES_ABS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/*.svg"
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/*.png"
)

# Convert absolute paths to relative paths for Qt resource system
set(ICON_RESOURCES "")
foreach(icon_file ${ICON_RESOURCES_ABS})
  file(RELATIVE_PATH icon_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${icon_file}")
  list(APPEND ICON_RESOURCES "${icon_rel}")
endforeach()

qt_add_qml_module(
  ${PROJECT_NAME}
  URI
  OpenGeoLab
  VERSION
  1.0
  RESOURCE_PREFIX
  /opengeolab
  NO_RESOURCE_TARGET_PATH
  SOURCES
  QML_FILES
  resources/qml/Main.qml
  resources/qml/MainPages.qml
  resources/qml/Theme.qml
  resources/qml/ThemedIcon.qml
  resources/qml/RibbonMenu/RibbonToolBar.qml
  resources/qml/RibbonMenu/RibbonFileMenu.qml
  resources/qml/RibbonMenu/RibbonFileMenuButton.qml
  resources/qml/RibbonMenu/RibbonTabContent.qml
  resources/qml/RibbonMenu/RibbonLargeButton.qml
  resources/qml/RibbonMenu/RibbonConfig.qml
  resources/qml/Pages/ImportModel.qml
  resources/qml/Pages/ProgressOverlay.qml
  resources/qml/Pages/CornerOverlay.qml
  resources/qml/Pages/LogPanel.qml
  resources/qml/Pages/AddBoxPage.qml
  resources/qml/Pages/AddCylinderPage.qml
  resources/qml/Pages/AddSpherePage.qml
  resources/qml/Pages/AddTorusPage.qml
  resources/qml/Pages/AIChatPage.qml
  resources/qml/Pages/AISuggestPage.qml
  resources/qml/Pages/CoordinateField.qml
  resources/qml/Pages/FunctionPageBase.qml
  resources/qml/Pages/GenerateMeshPage.qml
  resources/qml/Pages/OffsetPage.qml
  resources/qml/Pages/ParamField.qml
  resources/qml/Pages/SmoothMeshPage.qml
  resources/qml/Pages/TrimPage.qml
  resources/qml/Pages/GeoQueryPage.qml
  resources/qml/util/BaseButton.qml
  resources/qml/util/BaseComboBox.qml
  resources/qml/util/Selector.qml
  resources/qml/util/SelectableButton.qml
  resources/qml/util/ChipButton.qml
  resources/qml/util/ViewportPickButton.qml
  resources/qml/util/ViewToolbar.qml
  resources/qml/util/ViewToolButton.qml
  resources/qml/util/DocumentSideBar.qml
  resources/qml/util/PartListItem.qml
  resources/qml/util/EntityCountRow.qml
  resources/qml/util/DimensionInput.qml
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
          Qt6::OpenGL
          Kangaroo::Kangaroo
          fmt::fmt
          $<$<BOOL:${ENABLE_HDF5}>:HighFive::HighFive>
          nlohmann_json::nlohmann_json
          ${OpenCASCADE_LIBS}
          gmsh::shared
)

# Allow runtime control down to trace level (do not compile-out trace/debug).
target_compile_definitions(${PROJECT_NAME} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(
  ${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
                          "${CMAKE_CURRENT_SOURCE_DIR}/include/app" # For Qt moc generated code
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

if(ENABLE_TEST)
  enable_testing()
  add_subdirectory(test)
endif(ENABLE_TEST)
