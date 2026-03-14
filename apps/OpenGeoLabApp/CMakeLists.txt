set(opengeolab_app_headers include/ogl/app/OpenGeoLabController.hpp
                           include/ogl/app/EmbeddedPythonRuntime.hpp
)
set(opengeolab_app_sources src/main.cpp src/OpenGeoLabController.cpp src/EmbeddedPythonRuntime.cpp)
set(opengeolab_app_qml qml/Main.qml)

set_source_files_properties(${opengeolab_app_qml} PROPERTIES QT_RESOURCE_ALIAS Main.qml)

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
