cmake_minimum_required(VERSION 3.25)
project(
    OpenGeoLab
    VERSION 0.1.0
    DESCRIPTION "OpenGeoLab — Modern OpenGL 3D modeling application"
    LANGUAGES C CXX)
if (POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce dynamic CRT (/MD, /MDd) for all targets including third-party deps.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Customize RelWithDebInfo flags: reduce optimization for easier debugging.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO
        "/Zi /Od /DNDEBUG"
        CACHE STRING "C flags for RelWithDebInfo (MSVC)" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "/Zi /Od /DNDEBUG"
        CACHE STRING "CXX flags for RelWithDebInfo (MSVC)" FORCE)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO
        "-O -g -DNDEBUG"
        CACHE STRING "C flags for RelWithDebInfo (Clang/GCC)" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "-O -g -DNDEBUG"
        CACHE STRING "CXX flags for RelWithDebInfo (Clang/GCC)" FORCE)
endif ()

option(OPENGEOLAB_BUILD_TESTS "Build OpenGeoLab tests" ON)
option(OPENGEOLAB_FETCH_DEPENDENCIES "Fetch missing dependencies with CPM" ON)
option(OPENGEOLAB_BUILD_SHARED_LIBS
       "Build first-party OpenGeoLab libraries as shared libraries" ON)
option(OPENGEOLAB_USE_GMSH "Enable gmsh mesh generation support" ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
        RelWithDebInfo
        CACHE STRING "Build type" FORCE)
    message(
        STATUS "Defaulting CMAKE_BUILD_TYPE to RelWithDebInfo (PySide6 compat)")
endif ()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(QT_QML_GENERATE_QMLLS_INI_DIR "${CMAKE_CURRENT_BINARY_DIR}/qml")

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
set(OPENGEOLAB_KANGAROO_VERSION 2.3.1)
set(OPENGEOLAB_NLOHMANN_JSON_VERSION 3.12.0)
set(OPENGEOLAB_DOCTEST_VERSION 2.5.0)
set(OPENGEOLAB_GLM_VERSION 1.0.3)
set(OPENGEOLAB_GLAD_VERSION 2.0.8)
set(OPENGEOLAB_STB_VERSION master)

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
# Python runtime environment
# ---------------------------------------------------------------------------

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module
                                         Development.Embed)

# PySide6 pip wheels only ship release Qt binaries. Debug builds load both
# debug (Qt6Cored.dll) and release Qt DLLs, creating duplicate Qt singletons.
# Default OFF for Debug to avoid this conflict.
set(_pyside6_default ON)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_pyside6_default OFF)
endif ()
option(OPENGEOLAB_ENABLE_PYSIDE6 "Setup PySide6 venv (requires release Qt)"
       ${_pyside6_default})

if (OPENGEOLAB_ENABLE_PYSIDE6)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SetupPySideVenv.cmake)
else ()
    set(OPENGEOLAB_PYTHON_EXECUTABLE
        "${Python3_EXECUTABLE}"
        CACHE FILEPATH "Python executable used by the embedded runtime" FORCE)
    # glad uses FindPython rather than FindPython3.
    set(Python_EXECUTABLE
        "${Python3_EXECUTABLE}"
        CACHE FILEPATH "Python executable used by build-time generators" FORCE)
    message(STATUS "PySide6 venv setup disabled (Debug build or explicit OFF)")
endif ()

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

opengeolab_resolve_package(
    glm
    glm::glm
    VERSION
    ${OPENGEOLAB_GLM_VERSION}
    GITHUB_REPOSITORY
    g-truc/glm
    GIT_TAG
    ${OPENGEOLAB_GLM_VERSION}
    CPM_OPTIONS
    "GLM_ENABLE_CXX_20 ON")

find_package(glad ${OPENGEOLAB_GLAD_VERSION} CONFIG QUIET)
if (TARGET glad::glad)
    message(STATUS "Using installed glad package")
elseif (TARGET glad_gl_core_33)
    add_library(glad::glad ALIAS glad_gl_core_33)
elseif (glad_FOUND)
    glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)
    add_library(glad::glad ALIAS glad_gl_core_33)
elseif (NOT OPENGEOLAB_FETCH_DEPENDENCIES)
    message(
        FATAL_ERROR
            "glad was not found. Provide glad::glad, install glad, or enable OPENGEOLAB_FETCH_DEPENDENCIES."
    )
else ()
    message(STATUS "Fetching glad from GitHub")
    cpmaddpackage(NAME glad GITHUB_REPOSITORY Dav1dde/glad GIT_TAG
                  v${OPENGEOLAB_GLAD_VERSION})

    if (NOT EXISTS "${glad_SOURCE_DIR}/cmake/GladConfig.cmake")
        message(
            FATAL_ERROR
                "Fetched glad package does not provide cmake/GladConfig.cmake")
    endif ()

    set(GLAD_SOURCES_DIR "${glad_SOURCE_DIR}")
    include("${glad_SOURCE_DIR}/cmake/GladConfig.cmake")
    glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)
    add_library(glad::glad ALIAS glad_gl_core_33)
endif ()

CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG ${OPENGEOLAB_STB_VERSION}
    DOWNLOAD_ONLY YES)

if (stb_ADDED)
    add_library(stb_image INTERFACE)
    target_include_directories(stb_image INTERFACE "${stb_SOURCE_DIR}")
endif ()

# ---------------------------------------------------------------------------
# OpenCASCADE Technology
# ---------------------------------------------------------------------------
find_package(OpenCASCADE REQUIRED)

# OCC 7.6.x cmake config omits INTERFACE_INCLUDE_DIRECTORIES on imported
# targets.  Patch them so downstream targets get the OCC headers.
if(DEFINED OpenCASCADE_INCLUDE_DIR AND EXISTS "${OpenCASCADE_INCLUDE_DIR}")
    foreach(_occ_lib IN LISTS OpenCASCADE_LIBRARIES)
        if(TARGET ${_occ_lib})
            get_target_property(_inc ${_occ_lib} INTERFACE_INCLUDE_DIRECTORIES)
            if(NOT _inc)
                set_target_properties(
                    ${_occ_lib} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                           "${OpenCASCADE_INCLUDE_DIR}")
            endif()
        endif()
    endforeach()
endif()

# OCC 7.8+ consolidated data-exchange into TKDE / TKDESTEP.  Provide
# compatibility aliases when building against OCC 7.6.x.
if(NOT TARGET TKDE)
    add_library(TKDE INTERFACE IMPORTED)
    set(_tkde_fallback TKXSBase)
    foreach(_t IN ITEMS TKIGES TKSTEPBase TKSTEPAttr TKSTEP209 TKSTEP TKSTL TKVRML)
        if(TARGET ${_t})
            list(APPEND _tkde_fallback ${_t})
        endif()
    endforeach()
    target_link_libraries(TKDE INTERFACE ${_tkde_fallback})
endif()
if(NOT TARGET TKDESTEP)
    add_library(TKDESTEP INTERFACE IMPORTED)
    set(_tkdestep_fallback "")
    foreach(_t IN ITEMS TKSTEPBase TKSTEPAttr TKSTEP209 TKSTEP TKXDESTEP)
        if(TARGET ${_t})
            list(APPEND _tkdestep_fallback ${_t})
        endif()
    endforeach()
    if(_tkdestep_fallback)
        target_link_libraries(TKDESTEP INTERFACE ${_tkdestep_fallback})
    endif()
endif()

if(OPENGEOLAB_USE_GMSH)
    find_package(gmsh REQUIRED)
    if(TARGET gmsh::shared AND NOT TARGET gmsh)
        add_library(gmsh INTERFACE IMPORTED)
        target_link_libraries(gmsh INTERFACE gmsh::shared)
    endif()
endif()

# ---------------------------------------------------------------------------
# Python: embedded runtime + pybind11 bridge
# ---------------------------------------------------------------------------

set(OPENGEOLAB_PYBIND11_VERSION 3.0.2)
opengeolab_resolve_package(
    pybind11
    pybind11::pybind11
    VERSION
    ${OPENGEOLAB_PYBIND11_VERSION}
    GITHUB_REPOSITORY
    pybind/pybind11
    GIT_TAG
    v${OPENGEOLAB_PYBIND11_VERSION})

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------

if (OPENGEOLAB_BUILD_TESTS)
    include(CTest)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/testing.cmake)
    enable_testing()

    # Temporarily disable clang-tidy for third-party doctest.
    set(_saved_cxx_clang_tidy "${CMAKE_CXX_CLANG_TIDY}")
    set(_saved_c_clang_tidy "${CMAKE_C_CLANG_TIDY}")
    set(CMAKE_CXX_CLANG_TIDY "")
    set(CMAKE_C_CLANG_TIDY "")

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

    set(CMAKE_CXX_CLANG_TIDY "${_saved_cxx_clang_tidy}")
    set(CMAKE_C_CLANG_TIDY "${_saved_c_clang_tidy}")
endif ()

# ---------------------------------------------------------------------------
# Sub-projects
# ---------------------------------------------------------------------------
add_subdirectory(src/libs/core)
add_subdirectory(src/libs/io)
add_subdirectory(src/libs/geometry)
add_subdirectory(src/libs/scene)
add_subdirectory(src/libs/mesh)
add_subdirectory(src/libs/render)
add_subdirectory(src/libs/command)
add_subdirectory(src/libs/python)
add_subdirectory(src/app)
