cmake_minimum_required(VERSION 3.25)

project(
    OpenGeoLab
    VERSION 0.2.0
    DESCRIPTION "Layered single-shell OpenGeoLab scaffold"
    LANGUAGES CXX)

if (POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif ()

include(GNUInstallDirs)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

option(OPENGEOLAB_BUILD_TESTS "Build OpenGeoLab smoke tests" ON)
option(OPENGEOLAB_FETCH_DEPENDENCIES "Fetch missing dependencies with CPM" ON)
option(OPENGEOLAB_BUILD_SHARED_LIBS
       "Build first-party OpenGeoLab libraries as shared libraries" ON)
option(
    OPENGEOLAB_INSTALL_PACKAGE_EXPORT
    "Install a CMake package export for first-party targets when dependencies are externally packaged"
    OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to RelWithDebInfo for single-config generators.
# PySide6 (pip) ships release Qt DLLs (Qt6Core.dll without 'd' suffix).
# Debug builds load Qt6Cored.dll, causing two separate Qt singletons and
# breaking QApplication.instance() in PySide6. RelWithDebInfo gives debug
# symbols while using release Qt, ensuring a single shared Qt instance.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
  message(STATUS "Defaulting CMAKE_BUILD_TYPE to RelWithDebInfo (PySide6 compat)")
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

set(OPENGEOLAB_LIBRARY_TYPE STATIC)
if (OPENGEOLAB_BUILD_SHARED_LIBS)
    set(OPENGEOLAB_LIBRARY_TYPE SHARED)
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

foreach (output_config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${output_config}" output_config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${output_config_upper}
        "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${output_config_upper}
        "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${output_config_upper}
        "${CMAKE_BINARY_DIR}/lib")
endforeach ()

set(OPENGEOLAB_PLUGIN_SOURCE_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins"
    CACHE PATH "Source tree plugin root")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenGeoLabModule.cmake)

set(OPENGEOLAB_FMT_VERSION 12.0.0)
set(OPENGEOLAB_SPDLOG_VERSION 1.17.0)
set(OPENGEOLAB_KANGAROO_VERSION 2.2.2)
set(OPENGEOLAB_PYBIND11_VERSION 3.0.2)
set(OPENGEOLAB_DOCTEST_VERSION 2.4.12)
set(OPENGEOLAB_NLOHMANN_JSON_VERSION 3.12.0)
set(PYBIND11_FINDPYTHON ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Concurrent Core Gui Qml Quick
                                         QuickControls2 Widgets)
if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()
qt_standard_project_setup(REQUIRES 6.9)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module
                                         Development.Embed)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SetupPySideVenv.cmake)

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
    "BUILD_SHARED_LIBS ON"
    "FMT_INSTALL YES"
    "FMT_DOC OFF"
    "FMT_TEST OFF")

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
    "SPDLOG_BUILD_SHARED ON"
    "SPDLOG_INSTALL YES")

set(LOCAL_KANGAROO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Kangaroo")
if (EXISTS "${LOCAL_KANGAROO_PATH}")
    message(
        STATUS
            "Found local Kangaroo at ${LOCAL_KANGAROO_PATH}, using local version"
    )
    cpmaddpackage(
        NAME
        Kangaroo
        SOURCE_DIR
        ${LOCAL_KANGAROO_PATH}
        OPTIONS
        "KANGAROO_FETCH_DEPENDENCIES OFF"
        "BUILD_SHARED_LIBS ON"
        "BUILD_TESTING OFF"
        "BUILD_DOCUMENTATION OFF")
    if (NOT TARGET Kangaroo::Kangaroo)
        message(
            FATAL_ERROR
                "Local Kangaroo package did not expose Kangaroo::Kangaroo")
    endif ()
else ()
    opengeolab_resolve_package(
        Kangaroo
        Kangaroo::Kangaroo
        VERSION
        ${OPENGEOLAB_KANGAROO_VERSION}
        GITHUB_REPOSITORY
        HATTER-LONG/Kangaroo
        GIT_TAG
        v${OPENGEOLAB_KANGAROO_VERSION}
        CPM_OPTIONS
        "KANGAROO_FETCH_DEPENDENCIES OFF"
        "BUILD_SHARED_LIBS ON"
        "BUILD_TESTING OFF"
        "BUILD_DOCUMENTATION OFF")
endif ()

opengeolab_resolve_package(
    pybind11
    pybind11::pybind11
    VERSION
    ${OPENGEOLAB_PYBIND11_VERSION}
    GITHUB_REPOSITORY
    pybind/pybind11
    GIT_TAG
    v${OPENGEOLAB_PYBIND11_VERSION})

opengeolab_resolve_package(
    nlohmann_json
    nlohmann_json::nlohmann_json
    VERSION
    ${OPENGEOLAB_NLOHMANN_JSON_VERSION}
    GITHUB_REPOSITORY
    nlohmann/json
    GIT_TAG
    v${OPENGEOLAB_NLOHMANN_JSON_VERSION})

if (OPENGEOLAB_BUILD_TESTS)
    include(CTest)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/testing.cmake)
    enable_testing()

    opengeolab_resolve_package(
        doctest
        doctest::doctest_with_main
        VERSION
        ${OPENGEOLAB_DOCTEST_VERSION}
        GITHUB_REPOSITORY
        doctest/doctest
        GIT_TAG
        v${OPENGEOLAB_DOCTEST_VERSION}
        CPM_OPTIONS
        "DOCTEST_WITH_TESTS OFF"
        "DOCTEST_NO_INSTALL ON")
endif ()

add_subdirectory(src/libs/geometry)

add_subdirectory(src/libs/command)

add_subdirectory(src/libs/python)

add_subdirectory(src/app)

if (OPENGEOLAB_INSTALL_PACKAGE_EXPORT
    AND (TARGET ogl_geometry
         OR TARGET ogl_command
         OR TARGET opengeolab_python_embed))
    install(
        EXPORT OpenGeoLabTargets
        NAMESPACE OpenGeoLab::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/OpenGeoLab")
endif ()
