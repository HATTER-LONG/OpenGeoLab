include(GenerateExportHeader)

set(ogl_command_headers include/ogl/command/CommandService.hpp)
set(ogl_command_sources src/CommandService.cpp)

add_library(ogl_command ${OPENGEOLAB_LIBRARY_TYPE} ${ogl_command_headers} ${ogl_command_sources})
add_library(OGL::Command ALIAS ogl_command)

generate_export_header(
  ogl_command EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/command/export.hpp
  EXPORT_MACRO_NAME OGL_COMMAND_EXPORT
)

target_compile_features(ogl_command PUBLIC cxx_std_20)

if(NOT OPENGEOLAB_BUILD_SHARED_LIBS)
  target_compile_definitions(ogl_command PUBLIC OGL_COMMAND_STATIC_DEFINE)
endif()

target_include_directories(
  ogl_command
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  ogl_command PUBLIC ogl_core ogl_geometry ogl_scene ogl_render ogl_selection
                     nlohmann_json::nlohmann_json
)

set_target_properties(ogl_command PROPERTIES FOLDER libs)

install(
  TARGETS ogl_command
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/command/export.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogl/command
)

if(ENABLE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
