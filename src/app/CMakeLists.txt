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
            OpenGeoLab::Python)

qt_add_qml_module(
    opengeolab_app URI OpenGeoLab.App VERSION 1.0
    QML_FILES resource/qml/Main.qml
              resource/qml/ActionButton.qml
              resource/qml/ResponsePanel.qml
)

if (NOT TARGET opengeolab_pywrapper)
    message(
        FATAL_ERROR "opengeolab_app requires the opengeolab_pywrapper module.")
endif ()
add_dependencies(opengeolab_app opengeolab_pywrapper)

# Use console subsystem during development so errors are visible.
# For release packaging, set OPENGEOLAB_WIN32_APP=ON.
option(OPENGEOLAB_WIN32_APP "Build the app as a Windows GUI executable (no console)" OFF)

if(OPENGEOLAB_WIN32_APP)
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app"
                                                    WIN32_EXECUTABLE TRUE)
else()
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app")
endif()

# Pass the Qt prefix bin directory at compile time so main.cpp can register it
# as a DLL search path before QApplication creation.  This prevents Windows
# from accidentally loading a different Qt version from PATH (e.g. Qt Creator).
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
file(TO_CMAKE_PATH "${_qt_prefix}" _qt_prefix_fwd)
target_compile_definitions(
    opengeolab_app
    PRIVATE "OPENGEOLAB_QT_PREFIX=L\"${_qt_prefix_fwd}/bin\"")

# Copy direct Qt runtime DLLs so the executable directory wins over PATH.
if(WIN32)
    add_custom_command(
        TARGET opengeolab_app
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_RUNTIME_DLLS:opengeolab_app>
            "$<TARGET_FILE_DIR:opengeolab_app>"
        COMMAND_EXPAND_LISTS VERBATIM
    )
endif()

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
