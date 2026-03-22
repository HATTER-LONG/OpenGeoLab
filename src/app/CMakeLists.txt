qt_add_executable(opengeolab_app
                  src/main.cpp
                  src/process_service.cpp
                  include/opengeolab/app/process_service.hpp)

target_compile_features(opengeolab_app PRIVATE cxx_std_20)
target_include_directories(opengeolab_app
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
    opengeolab_app
    PRIVATE Qt6::Core
            Qt6::Concurrent
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Widgets
            Qt6::OpenGL
            OpenGeoLab::Command
            OpenGeoLab::Python
            OpenGeoLab::Render
            Coin::Coin)

qt_add_qml_module(
    opengeolab_app URI OpenGeoLab.App VERSION 1.0
    QML_FILES resource/qml/Main.qml
              resource/qml/ActionButton.qml
              resource/qml/ResponsePanel.qml
    SOURCES src/coin_quick_item.cpp
            include/opengeolab/app/coin_quick_item.hpp
)

if (NOT TARGET opengeolab_pywrapper)
    message(
        FATAL_ERROR "opengeolab_app requires the opengeolab_pywrapper module.")
endif ()
add_dependencies(opengeolab_app opengeolab_pywrapper)

# Use console subsystem during development so errors are visible.
# For release packaging, set OPENGEOLAB_WIN32_APP=ON.
option(OPENGEOLAB_WIN32_APP
       "Build the app as a Windows GUI executable (no console)" OFF)

if(OPENGEOLAB_WIN32_APP)
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app"
                                                    WIN32_EXECUTABLE TRUE)
else()
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app")
endif()

# --- Qt prefix for plugin discovery and DLL search ---------------------------
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
file(TO_CMAKE_PATH "${_qt_prefix}" _qt_prefix_fwd)

# Pass the Qt bin directory as a plain C string.  main.cpp widens it to
# wchar_t at runtime for SetDllDirectoryW.  This avoids embedding wide-string
# literals in CMake compile definitions (which is MSVC-specific).
if(WIN32)
    target_compile_definitions(
        opengeolab_app
        PRIVATE "OPENGEOLAB_QT_BIN_DIR=\"${_qt_prefix_fwd}/bin\"")
endif()

# Copy runtime DLLs (Qt, Python, project libraries) next to the executable
# so the loader finds the correct versions before PATH.
opengeolab_copy_runtime_dlls(opengeolab_app)

# Generate qt.conf so Qt finds plugins and QML modules from the install prefix.
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:opengeolab_app>/qt.conf"
    CONTENT "[Paths]\nPrefix = ${_qt_prefix_fwd}\n"
)

add_custom_command(
    TARGET opengeolab_app
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opengeolab_app>/python"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opengeolab_app>/plugins/python"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resource/python/opengeolab_runtime.py"
        "$<TARGET_FILE_DIR:opengeolab_app>/python/opengeolab_runtime.py"
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/plugins/python"
        "$<TARGET_FILE_DIR:opengeolab_app>/plugins/python"
    VERBATIM)

install(TARGETS opengeolab_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
