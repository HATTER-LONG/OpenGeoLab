# ---------------------------------------------------------------------------
# OpenGeoLab Application
# ---------------------------------------------------------------------------

find_package(Qt6 6.9 REQUIRED COMPONENTS Concurrent Core5Compat Svg
                                         QuickLayouts Widgets)

qt_add_executable(opengeolab_app src/main.cpp src/translation_manager.cpp
                  src/request_service.cpp)

target_compile_features(opengeolab_app PRIVATE cxx_std_20)
target_include_directories(
    opengeolab_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
                           "${CMAKE_CURRENT_SOURCE_DIR}/include/opengeolab/app")
target_link_libraries(
    opengeolab_app
    PRIVATE Qt6::Concurrent
            Qt6::Core
            Qt6::Core5Compat
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::QuickLayouts
            Qt6::Svg
            Qt6::Widgets
            OpenGeoLab::Python_Embed)

qt_add_qml_module(
    opengeolab_app
    URI
    OpenGeoLab.App
    VERSION
    1.0
    SOURCES
    include/opengeolab/app/translation_manager.h
    include/opengeolab/app/request_service.h
    QML_FILES
    resource/qml/Main.qml
    resource/qml/RibbonConfig.qml
    resource/qml/MenuConfig.qml
    resource/qml/theme/AppTheme.qml
    resource/qml/sections/AppHeader.qml
    resource/qml/sections/HeaderMenuPanel.qml
    resource/qml/sections/HeaderRibbonGroup.qml
    resource/qml/sections/SidebarPanel.qml
    resource/qml/sections/ViewportPanel.qml
    resource/qml/sections/ActivityOverlay.qml
    resource/qml/components/AppIcon.qml
    resource/qml/components/ActionButton.qml
    resource/qml/components/PluginRibbonGroup.qml
    resource/qml/components/RibbonTile.qml
    resource/qml/components/SectionCard.qml
    resource/qml/components/LogLevelChip.qml
    resource/qml/components/StatChip.qml
    resource/qml/components/LogEventsView.qml
    resource/qml/components/ActivityPanel.qml
    resource/qml/components/TerminalView.qml
    resource/qml/components/ProgressCard.qml
    resource/qml/components/BoxListItem.qml
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
    resource/icons/play.svg
    resource/icons/pluginA.svg
    resource/icons/pluginB.svg
    resource/icons/pluginC.svg
    resource/icons/pluginD.svg
    resource/icons/pluginE.svg
    resource/icons/pluginF.svg
    resource/icons/pluginG.svg
    resource/icons/pluginH.svg
    resource/icons/pluginI.svg
    resource/icons/pluginJ.svg
    resource/icons/cubeOutline.svg
    resource/icons/hourglassOutline.svg
    resource/icons/checkmarkCircleOutline.svg
    resource/icons/closeCircleOutline.svg)

# Use console subsystem during development so errors are visible.
option(OPENGEOLAB_WIN32_APP
       "Build the app as a Windows GUI executable (no console)" OFF)

if (OPENGEOLAB_WIN32_APP)
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app"
                                                    WIN32_EXECUTABLE TRUE)
else ()
    set_target_properties(opengeolab_app PROPERTIES FOLDER "app")
endif ()

# Qt prefix for plugin discovery and DLL search.
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
file(TO_CMAKE_PATH "${_qt_prefix}" _qt_prefix_fwd)

opengeolab_copy_runtime_dlls(opengeolab_app)

if (WIN32)
    target_compile_definitions(
        opengeolab_app
        PRIVATE "OPENGEOLAB_QT_BIN_DIR=\"${_qt_prefix_fwd}/bin\"")
endif ()

# Generate qt.conf so Qt finds plugins and QML modules from the install prefix.
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:opengeolab_app>/qt.conf"
    CONTENT "[Paths]\nPrefix = ${_qt_prefix_fwd}\n")

install(TARGETS opengeolab_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# ---------------------------------------------------------------------------
# Deploy Python runtime script and plugins next to the executable.
# ---------------------------------------------------------------------------
set(_app_bin_dir "$<TARGET_FILE_DIR:opengeolab_app>")

add_custom_command(
    TARGET opengeolab_app
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_bin_dir}/python"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resource/python/opengeolab_runtime.py"
        "${_app_bin_dir}/python/opengeolab_runtime.py"
    COMMENT "Copying Python runtime script")

# Copy plugins directory if it exists.
set(_plugin_source_dir "${CMAKE_SOURCE_DIR}/plugins")
if (EXISTS "${_plugin_source_dir}")
    add_custom_command(
        TARGET opengeolab_app
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_plugin_source_dir}"
                "${_app_bin_dir}/plugins"
        COMMENT "Copying plugins directory")
endif ()

qt_add_translations(
    opengeolab_app TS_FILES resource/translations/opengeolab_zh_CN.ts
    RESOURCE_PREFIX "/translations")
