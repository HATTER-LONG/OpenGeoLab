# Set version if not provided
if(NOT DEFINED QWINDOWKIT_VERSION)
    set(QWINDOWKIT_VERSION "1.4.0")
endif()

project(QWKQuick
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components (including private headers)
find_package(Qt6 REQUIRED COMPONENTS Core Quick Qml)
# Find Qt6 private components for accessing private headers
find_package(Qt6 REQUIRED COMPONENTS QuickPrivate)

# qwindowkit is a Qt abstraction library for frameless window. depends QWindowKit::Core
CPMAddPackage(
  NAME qwindowkit
  GITHUB_REPOSITORY "stdware/qwindowkit"
  GIT_TAG 1.4.0
  OPTIONS "QWINDOWKIT_BUILD_WIDGETS OFF" "QWINDOWKIT_BUILD_EXAMPLES OFF"
          "QWINDOWKIT_BUILD_TESTS OFF" "QWINDOWKIT_BUILD_QUICK OFF" "QWINDOWKIT_BUILD_STATIC ON"
)

# Define source files
set(_src
    qwkquickglobal.h
    qwkquickglobal.cpp
    quickitemdelegate_p.h
    quickitemdelegate.cpp
    quickwindowagent.h
    quickwindowagent_p.h
    quickwindowagent.cpp
)

# Add platform-specific sources
if(WIN32)
    list(APPEND _src quickwindowagent_win.cpp)
elseif(APPLE)
    list(APPEND _src quickwindowagent_mac.cpp)
endif()
# Create the static library
add_library(QWKQuick STATIC ${_src})

# Set target properties
set_target_properties(QWKQuick PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Define compile definitions for static library
target_compile_definitions(QWKQuick 
    PRIVATE 
        QWK_QUICK_LIBRARY
        QWK_QUICK_STATIC
    PUBLIC
        QWK_QUICK_STATIC
)

# Link Qt6 libraries
target_link_libraries(QWKQuick 
    PUBLIC 
        Qt6::Core
        Qt6::Quick 
        Qt6::Qml
        Qt6::QuickPrivate  # Required for private headers like qquickitem_p.h
    PUBLIC
        QWindowKit::Core
)
# Set include directories
target_include_directories(QWKQuick
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# Compiler-specific options
if(MSVC)
    target_compile_options(QWKQuick PRIVATE /W3)
else()
    target_compile_options(QWKQuick PRIVATE -Wall -Wextra)
endif()

# Create an alias target for easier usage in parent projects
add_library(QWKQuick::QWKQuick ALIAS QWKQuick)