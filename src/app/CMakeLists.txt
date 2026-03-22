qt_add_executable(opengeolab_app src/main.cpp src/translation_manager.cpp)

target_compile_features(opengeolab_app PRIVATE cxx_std_20)
target_include_directories(opengeolab_app
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/include/opengeolab/app")
target_link_libraries(
    opengeolab_app
    PRIVATE Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Widgets
            Qt6::OpenGL)

qt_add_qml_module(
    opengeolab_app
    URI
    OpenGeoLab.App
    VERSION
    1.0
    SOURCES
    include/opengeolab/app/translation_manager.h
    QML_FILES
    resource/qml/Main.qml
    resource/qml/RibbonConfig.qml
    resource/qml/theme/AppTheme.qml
    resource/qml/sections/AppHeader.qml
    resource/qml/sections/HeaderMenuPanel.qml
    resource/qml/sections/HeaderRibbonGroup.qml
    resource/qml/sections/SidebarPanel.qml
    resource/qml/sections/ViewportPanel.qml
    resource/qml/components/AppIcon.qml
    resource/qml/components/ActionButton.qml
    resource/qml/components/RibbonTile.qml
    resource/qml/components/SectionCard.qml
    resource/qml/components/LogLevelChip.qml
    resource/qml/components/LogEventsView.qml
    resource/qml/components/ActivityPanel.qml
    resource/qml/components/TerminalView.qml
    RESOURCES
    resource/icons/menu.svg
    resource/icons/box.svg
    resource/icons/cylinder.svg
    resource/icons/sphere.svg
    resource/icons/torus.svg
    resource/icons/trim.svg
    resource/icons/offset.svg
    resource/icons/query.svg
    resource/icons/mesh.svg
    resource/icons/smoothMesh.svg
    resource/icons/aiSuggest.svg
    resource/icons/aiChat.svg
    resource/icons/import.svg
    resource/icons/export.svg
    resource/icons/darkTheme.svg
    resource/icons/lightTheme.svg
    resource/icons/language.svg
    resource/icons/record.svg
    resource/icons/replay.svg
    resource/icons/exportRecord.svg
    resource/icons/clear.svg
    resource/icons/pulse.svg
    resource/icons/list.svg
    resource/icons/terminal.svg
    resource/icons/closePanel.svg
    resource/icons/trash.svg
    resource/icons/funnel.svg
    resource/icons/chevronDown.svg
    resource/icons/play.svg)

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
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:opengeolab_app>/qt.conf"
    CONTENT "[Paths]\nPrefix = ${_qt_prefix_fwd}\n")

qt_add_translations(
    opengeolab_app
    TS_FILES resource/translations/opengeolab_zh_CN.ts
    RESOURCE_PREFIX "/translations")

install(TARGETS opengeolab_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
