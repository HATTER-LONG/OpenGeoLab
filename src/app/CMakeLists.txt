qt_add_executable(
    opengeolab_app
    src/main.cpp
    src/translation_manager.cpp
    src/progress_tracker.cpp
    src/main_thread_executor.cpp
    src/notification_service.cpp
    src/request_service.cpp)

target_compile_features(opengeolab_app PRIVATE cxx_std_20)
target_include_directories(
    opengeolab_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
                           "${CMAKE_CURRENT_SOURCE_DIR}/include/opengeolab/app")
target_link_libraries(
    opengeolab_app
    PRIVATE Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::Widgets
            Qt6::OpenGL
            Qt6::Concurrent
            OpenGeoLab::Base
            OpenGeoLab::Python)

qt_add_qml_module(
    opengeolab_app
    URI
    OpenGeoLab.App
    VERSION
    1.0
    SOURCES
    include/opengeolab/app/translation_manager.h
    include/opengeolab/app/progress_tracker.hpp
    include/opengeolab/app/main_thread_executor.hpp
    include/opengeolab/app/notification_service.hpp
    include/opengeolab/app/request_service.hpp
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

# Qt prefix for plugin discovery and DLL search
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
file(TO_CMAKE_PATH "${_qt_prefix}" _qt_prefix_fwd)

if (WIN32)
    target_compile_definitions(
        opengeolab_app
        PRIVATE "OPENGEOLAB_QT_BIN_DIR=\"${_qt_prefix_fwd}/bin\"")
endif ()

opengeolab_copy_runtime_dlls(opengeolab_app)

# Ensure opengeolab_pywrapper is built before the app so its .pyd is available.
if (NOT TARGET opengeolab_pywrapper)
    message(
        FATAL_ERROR "opengeolab_app requires the opengeolab_pywrapper module.")
endif ()
add_dependencies(opengeolab_app opengeolab_pywrapper)

# Copy Python runtime scripts and plugins to the output directory.
add_custom_command(
    TARGET opengeolab_app
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opengeolab_app>/python"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:opengeolab_app>/plugins"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resource/python/opengeolab_runtime.py"
        "$<TARGET_FILE_DIR:opengeolab_app>/python/opengeolab_runtime.py"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/plugins"
            "$<TARGET_FILE_DIR:opengeolab_app>/plugins"
    COMMENT "Copying Python runtime scripts; Copying Python plugins"
    VERBATIM)

# Generate qt.conf so Qt finds plugins and QML modules from the install prefix.
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:opengeolab_app>/qt.conf"
    CONTENT "[Paths]\nPrefix = ${_qt_prefix_fwd}\n")

qt_add_translations(
    opengeolab_app TS_FILES resource/translations/opengeolab_zh_CN.ts
    RESOURCE_PREFIX "/translations")

install(TARGETS opengeolab_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
