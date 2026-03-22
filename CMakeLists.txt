cmake_minimum_required(VERSION 3.25)
project(
    OpenGeoLab
    VERSION 0.1.0
    DESCRIPTION "OpenGeoLab — Modern OpenGL 3D modeling application"
    LANGUAGES CXX)

if (POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif ()

include(GNUInstallDirs)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

option(OPENGEOLAB_BUILD_TESTS "Build OpenGeoLab tests" ON)
option(OPENGEOLAB_FETCH_DEPENDENCIES "Fetch missing dependencies with CPM" ON)
option(OPENGEOLAB_BUILD_SHARED_LIBS
       "Build first-party OpenGeoLab libraries as shared libraries" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to RelWithDebInfo for single-config generators.
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
        RelWithDebInfo
        CACHE STRING "Build type" FORCE)
    message(STATUS "Defaulting CMAKE_BUILD_TYPE to RelWithDebInfo")
endif ()

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

# ---------------------------------------------------------------------------
# Tooling & CPM
# ---------------------------------------------------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenGeoLabModule.cmake)

# ---------------------------------------------------------------------------
# Dependency versions
# ---------------------------------------------------------------------------

set(OPENGEOLAB_FMT_VERSION 12.0.0)
set(OPENGEOLAB_SPDLOG_VERSION 1.17.0)
set(OPENGEOLAB_KANGAROO_VERSION 2.2.2)
set(OPENGEOLAB_NLOHMANN_JSON_VERSION 3.12.0)
set(OPENGEOLAB_DOCTEST_VERSION 2.4.12)

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------

find_package(
    Qt6 6.9 REQUIRED
    COMPONENTS Concurrent
               Core
               Gui
               LinguistTools
               Qml
               Quick
               QuickControls2
               Widgets
               OpenGL)
if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()
qt_standard_project_setup(REQUIRES 6.9)

# ---------------------------------------------------------------------------
# Third-party: fmt, spdlog, Kangaroo, nlohmann_json, GLM
# ---------------------------------------------------------------------------

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
    nlohmann_json
    nlohmann_json::nlohmann_json
    VERSION
    ${OPENGEOLAB_NLOHMANN_JSON_VERSION}
    GITHUB_REPOSITORY
    nlohmann/json
    GIT_TAG
    v${OPENGEOLAB_NLOHMANN_JSON_VERSION})

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------

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

# ---------------------------------------------------------------------------
# Sub-projects
# ---------------------------------------------------------------------------

add_subdirectory(src/app)
