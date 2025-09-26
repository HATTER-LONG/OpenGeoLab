qt6_policy(SET QTP0001 NEW)
# Disable private module warning
set(QT_NO_PRIVATE_MODULE_WARNING ON)
# Find Qt6 private components for accessing private headers
find_package(Qt6 REQUIRED COMPONENTS QuickPrivate)
# Enable private module warning
set(QT_NO_PRIVATE_MODULE_WARNING OFF)

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
qt6_add_qml_module(qwkquickplugin
    URI "QWKQuickWindow"
    STATIC
    PLUGIN_TARGET qwkquickplugin
    DEPENDENCIES QtQuick
     OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/QWKQuickWindow
)
# Create the static library
target_sources(qwkquickplugin PRIVATE ${_src})

# Set target properties
set_target_properties(qwkquickplugin PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_definitions(qwkquickplugin 
    PRIVATE 
        QWK_QUICK_LIBRARY
)

# Link Qt6 libraries
target_link_libraries(qwkquickplugin 
    PUBLIC 
        Qt6::Core
        Qt6::Quick 
        Qt6::Qml
        Qt6::QuickPrivate 
    PRIVATE
        QWindowKit::Core
)
# Set include directories
target_include_directories(qwkquickplugin
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# Compiler-specific options
if(MSVC)
    target_compile_options(qwkquickplugin PRIVATE /W3)
else()
    target_compile_options(qwkquickplugin PRIVATE -Wall -Wextra)
endif()

# Create an alias target for easier usage in parent projects
# add_library(FramelessWindow::FramelessWindow ALIAS FramelessWindow)