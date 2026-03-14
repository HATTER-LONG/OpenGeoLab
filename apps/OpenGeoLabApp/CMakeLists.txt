set(opengeolab_app_headers include/ogl/app/OpenGeoLabController.hpp
                           include/ogl/app/EmbeddedPythonRuntime.hpp
)
set(opengeolab_app_sources src/main.cpp src/OpenGeoLabController.cpp src/EmbeddedPythonRuntime.cpp)
set(opengeolab_app_qml
    qml/Main.qml
    qml/theme/AppTheme.qml
    qml/components/ActionButton.qml
    qml/components/AppIcon.qml
    qml/components/CodePanel.qml
    qml/components/RibbonTile.qml
    qml/components/SectionCard.qml
    qml/components/StatChip.qml
    qml/sections/AppHeader.qml
    qml/sections/HeaderMenuPanel.qml
    qml/sections/HeaderRibbonGroup.qml
    qml/sections/OutputStrip.qml
    qml/sections/SidebarPanel.qml
    qml/sections/ViewportPanel.qml
)

set(opengeolab_app_resources
    qml/theme/qmldir
    qml/components/qmldir
    qml/sections/qmldir
    icons/ai_chat.svg
    icons/ai_suggestion.svg
    icons/box.svg
    icons/clear.svg
    icons/cylinder.svg
    icons/dark.svg
    icons/eye.svg
    icons/export.svg
    icons/export_record.svg
    icons/import.svg
    icons/light.svg
    icons/menu.svg
    icons/mesh.svg
    icons/offset.svg
    icons/query.svg
    icons/record.svg
    icons/replay.svg
    icons/smooth_mesh.svg
    icons/sphere.svg
    icons/torus.svg
    icons/trim.svg
)

set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/theme/qmldir PROPERTIES QT_RESOURCE_ALIAS theme/qmldir)
set_source_files_properties(qml/theme/AppTheme.qml PROPERTIES QT_RESOURCE_ALIAS theme/AppTheme.qml)
set_source_files_properties(qml/components/qmldir PROPERTIES QT_RESOURCE_ALIAS components/qmldir)
set_source_files_properties(
  qml/components/ActionButton.qml PROPERTIES QT_RESOURCE_ALIAS components/ActionButton.qml
)
set_source_files_properties(
  qml/components/AppIcon.qml PROPERTIES QT_RESOURCE_ALIAS components/AppIcon.qml
)
set_source_files_properties(
  qml/components/CodePanel.qml PROPERTIES QT_RESOURCE_ALIAS components/CodePanel.qml
)
set_source_files_properties(
  qml/components/RibbonTile.qml PROPERTIES QT_RESOURCE_ALIAS components/RibbonTile.qml
)
set_source_files_properties(
  qml/components/SectionCard.qml PROPERTIES QT_RESOURCE_ALIAS components/SectionCard.qml
)
set_source_files_properties(
  qml/components/StatChip.qml PROPERTIES QT_RESOURCE_ALIAS components/StatChip.qml
)
set_source_files_properties(qml/sections/qmldir PROPERTIES QT_RESOURCE_ALIAS sections/qmldir)
set_source_files_properties(
  qml/sections/AppHeader.qml PROPERTIES QT_RESOURCE_ALIAS sections/AppHeader.qml
)
set_source_files_properties(
  qml/sections/HeaderMenuPanel.qml PROPERTIES QT_RESOURCE_ALIAS sections/HeaderMenuPanel.qml
)
set_source_files_properties(
  qml/sections/HeaderRibbonGroup.qml PROPERTIES QT_RESOURCE_ALIAS sections/HeaderRibbonGroup.qml
)
set_source_files_properties(
  qml/sections/OutputStrip.qml PROPERTIES QT_RESOURCE_ALIAS sections/OutputStrip.qml
)
set_source_files_properties(
  qml/sections/SidebarPanel.qml PROPERTIES QT_RESOURCE_ALIAS sections/SidebarPanel.qml
)
set_source_files_properties(
  qml/sections/ViewportPanel.qml PROPERTIES QT_RESOURCE_ALIAS sections/ViewportPanel.qml
)

set(opengeolab_executable_type)
if(WIN32 AND NOT ENABLE_CONSOLE)
  set(opengeolab_executable_type WIN32)
endif()

qt_add_executable(
  OpenGeoLabApp MANUAL_FINALIZATION ${opengeolab_executable_type} ${opengeolab_app_sources}
  ${opengeolab_app_headers}
)

qt_add_qml_module(
  OpenGeoLabApp
  URI
  OpenGeoLab
  VERSION
  1.0
  QML_FILES
  ${opengeolab_app_qml}
  RESOURCES
  ${opengeolab_app_resources}
)

target_include_directories(OpenGeoLabApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(OpenGeoLabApp PRIVATE Py_NO_LINK_LIB)
set(opengeolab_python_no_defaultlib_option /NODEFAULTLIB:python313.lib)
if(NOT MSVC)
  set(opengeolab_python_no_defaultlib_option -Xlinker /NODEFAULTLIB:python313.lib)
endif()

target_link_options(OpenGeoLabApp PRIVATE ${opengeolab_python_no_defaultlib_option})

target_link_libraries(
  OpenGeoLabApp
  PRIVATE Kangaroo::Kangaroo
          OGL::Command
          pybind11::embed
          Qt6::Core
          Qt6::Gui
          Qt6::Qml
          Qt6::Quick
          Qt6::OpenGL
)

set_target_properties(
  OpenGeoLabApp
  PROPERTIES WIN32_EXECUTABLE $<BOOL:$<AND:$<BOOL:${WIN32}>,$<NOT:$<BOOL:${ENABLE_CONSOLE}>>>>
             MACOSX_BUNDLE TRUE
)

install(
  TARGETS OpenGeoLabApp RUNTIME_DEPENDENCY_SET opengeolab_runtime_deps
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/OpenGeoLab/
  DESTINATION qml/OpenGeoLab
  FILES_MATCHING
  PATTERN "qmldir"
  PATTERN "*.qml"
  PATTERN "*.qmltypes"
)

if(WIN32
   AND TARGET Kangaroo::Kangaroo
   AND NOT TARGET Kangaroo
)
  get_target_property(opengeolab_kangaroo_type Kangaroo::Kangaroo TYPE)
  if(opengeolab_kangaroo_type STREQUAL "SHARED_LIBRARY")
    install(FILES $<TARGET_FILE:Kangaroo::Kangaroo> DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
  unset(opengeolab_kangaroo_type)
endif()

install(
  RUNTIME_DEPENDENCY_SET
  opengeolab_runtime_deps
  DESTINATION
  ${CMAKE_INSTALL_BINDIR}
  DIRECTORIES
  $<TARGET_FILE_DIR:Qt6::Core>
  PRE_EXCLUDE_REGEXES
  "api-ms-.*"
  "ext-ms-.*"
  POST_EXCLUDE_REGEXES
  ".*/system32/.*"
  ".*/Windows/.*"
)

qt_generate_deploy_qml_app_script(
  TARGET OpenGeoLabApp OUTPUT_SCRIPT opengeolab_deploy_script NO_UNSUPPORTED_PLATFORM_ERROR
)

set(opengeolab_deploy_wrapper
    "${CMAKE_CURRENT_BINARY_DIR}/opengeolab_deploy_wrapper_$<CONFIG>.cmake"
)
file(
  GENERATE
  OUTPUT ${opengeolab_deploy_wrapper}
  CONTENT
    "if(NOT DEFINED QT_DEPLOY_PREFIX)\n  set(QT_DEPLOY_PREFIX \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")\nendif()\n\nif(NOT IS_ABSOLUTE \"\${QT_DEPLOY_PREFIX}\")\n  get_filename_component(QT_DEPLOY_PREFIX \"\${QT_DEPLOY_PREFIX}\" ABSOLUTE BASE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")\nendif()\n\ninclude(\"${opengeolab_deploy_script}\")\n"
)

install(SCRIPT ${opengeolab_deploy_wrapper})

qt_finalize_executable(OpenGeoLabApp)

if(ENABLE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
