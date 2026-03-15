include(GenerateExportHeader)

set(ogl_selection_headers include/ogl/selection/SelectionResult.hpp
                           include/ogl/selection/SelectionComponentRegistration.hpp
                           include/ogl/selection/SelectionAction.hpp
                           include/ogl/selection/PickEntityAction.hpp
                           include/ogl/selection/BoxSelectAction.hpp
                           include/ogl/selection/SelectionLogger.hpp
)
set(ogl_selection_sources src/SelectionResult.cpp src/SelectionComponentRegistration.cpp
                          src/PickEntityAction.cpp src/BoxSelectAction.cpp)

add_library(
  ogl_selection ${OPENGEOLAB_LIBRARY_TYPE} ${ogl_selection_headers} ${ogl_selection_sources}
)
add_library(OGL::Selection ALIAS ogl_selection)

generate_export_header(
  ogl_selection EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/selection/export.hpp
  EXPORT_MACRO_NAME OGL_SELECTION_EXPORT
)

target_compile_features(ogl_selection PUBLIC cxx_std_20)

if(NOT OPENGEOLAB_BUILD_SHARED_LIBS)
  target_compile_definitions(ogl_selection PUBLIC OGL_SELECTION_STATIC_DEFINE)
endif()

target_include_directories(
  ogl_selection
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  ogl_selection
  PUBLIC ogl_core ogl_scene ogl_render nlohmann_json::nlohmann_json
  PRIVATE Kangaroo::Kangaroo
)

set_target_properties(ogl_selection PROPERTIES FOLDER libs)

install(
  TARGETS ogl_selection
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/selection/export.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogl/selection
)

if(ENABLE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
