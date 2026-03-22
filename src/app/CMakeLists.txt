qt_add_executable(opengeolab_app src/main.cpp)

target_compile_features(opengeolab_app PRIVATE cxx_std_20)
target_include_directories(opengeolab_app
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
    opengeolab_app
    PRIVATE Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Widgets
            Qt6::OpenGL)

qt_add_qml_module(opengeolab_app URI OpenGeoLab.App VERSION 1.0
                  QML_FILES resource/qml/Main.qml)

# Use console subsystem during development so errors are visible.
option(OPENGEOLAB_WIN32_APP
       "Build the app as a Windows GUI executable (no console)" OFF)

if (OPENGEOLAB_WIN32_APP)
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app"
                                                    WIN32_EXECUTABLE TRUE)
else ()
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app")
endif ()

# Qt prefix for plugin discovery and DLL search
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
file(TO_CMAKE_PATH "${_qt_prefix}" _qt_prefix_fwd)

if (WIN32)
    target_compile_definitions(
        opengeolab_app
        PRIVATE "OPENGEOLAB_QT_BIN_DIR=\"${_qt_prefix_fwd}/bin\"")
endif ()

opengeolab_copy_runtime_dlls(opengeolab_app)

# Generate qt.conf so Qt finds plugins and QML modules from the install prefix.
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:opengeolab_app>/qt.conf"
     CONTENT "[Paths]\nPrefix = ${_qt_prefix_fwd}\n")

install(TARGETS opengeolab_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
