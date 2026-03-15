include(GenerateExportHeader)

set(ogl_geometry_headers include/ogl/geometry/GeometryModel.hpp
                         include/ogl/geometry/GeometryComponentRegistration.hpp
                         include/ogl/geometry/GeometryAction.hpp
                         include/ogl/geometry/InspectModelAction.hpp
                         include/ogl/geometry/CreateBoxAction.hpp
                         include/ogl/geometry/CreateCylinderAction.hpp
                         include/ogl/geometry/CreateSphereAction.hpp
                         include/ogl/geometry/CreateTorusAction.hpp
                         include/ogl/geometry/GeometryLogger.hpp
)
set(ogl_geometry_sources
    src/GeometryModel.cpp
    src/GeometryComponentRegistration.cpp
    src/InspectModelAction.cpp
    src/CreateBoxAction.cpp
    src/CreateCylinderAction.cpp
    src/CreateSphereAction.cpp
    src/CreateTorusAction.cpp
)

add_library(ogl_geometry ${OPENGEOLAB_LIBRARY_TYPE} ${ogl_geometry_headers} ${ogl_geometry_sources})
add_library(OGL::Geometry ALIAS ogl_geometry)

generate_export_header(
  ogl_geometry EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/geometry/export.hpp
  EXPORT_MACRO_NAME OGL_GEOMETRY_EXPORT
)

target_compile_features(ogl_geometry PUBLIC cxx_std_20)

if(NOT OPENGEOLAB_BUILD_SHARED_LIBS)
  target_compile_definitions(ogl_geometry PUBLIC OGL_GEOMETRY_STATIC_DEFINE)
endif()

target_include_directories(
  ogl_geometry
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  ogl_geometry
  PUBLIC ogl_core nlohmann_json::nlohmann_json
  PRIVATE Kangaroo::Kangaroo
)

set_target_properties(ogl_geometry PROPERTIES FOLDER libs)

install(
  TARGETS ogl_geometry
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/geometry/export.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogl/geometry
)

if(ENABLE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()

