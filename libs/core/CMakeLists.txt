include(GenerateExportHeader)

set(ogl_core_headers include/ogl/core/IService.hpp include/ogl/core/ComponentRequestDispatcher.hpp
                     include/ogl/core/ComponentDispatcherLogger.hpp
)
set(ogl_core_sources src/ComponentRequestDispatcher.cpp)

add_library(ogl_core ${OPENGEOLAB_LIBRARY_TYPE} ${ogl_core_headers} ${ogl_core_sources})
add_library(OGL::Core ALIAS ogl_core)

generate_export_header(
  ogl_core EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/core/export.hpp
  EXPORT_MACRO_NAME OGL_CORE_EXPORT
)

target_compile_features(ogl_core PUBLIC cxx_std_20)

if(NOT OPENGEOLAB_BUILD_SHARED_LIBS)
  target_compile_definitions(ogl_core PUBLIC OGL_CORE_STATIC_DEFINE)
endif()

target_include_directories(
  ogl_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ogl_core PUBLIC Kangaroo::Kangaroo nlohmann_json::nlohmann_json)

set_target_properties(ogl_core PROPERTIES FOLDER libs)

install(
  TARGETS ogl_core
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/core/export.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogl/core
)
