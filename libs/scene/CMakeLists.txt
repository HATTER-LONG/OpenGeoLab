include(GenerateExportHeader)

set(ogl_scene_headers include/ogl/scene/PlaceholderSceneGraph.hpp
											include/ogl/scene/SceneComponentRegistration.hpp
)
set(ogl_scene_sources src/PlaceholderSceneGraph.cpp src/SceneComponentRegistration.cpp)

add_library(ogl_scene ${OPENGEOLAB_LIBRARY_TYPE} ${ogl_scene_headers} ${ogl_scene_sources})
add_library(OpenGeoLab::scene ALIAS ogl_scene)

generate_export_header(
	ogl_scene EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/scene/export.hpp
	EXPORT_MACRO_NAME OGL_SCENE_EXPORT
)

target_compile_features(ogl_scene PUBLIC cxx_std_20)

if(NOT OPENGEOLAB_BUILD_SHARED_LIBS)
	target_compile_definitions(ogl_scene PUBLIC OGL_SCENE_STATIC_DEFINE)
endif()

target_include_directories(
	ogl_scene
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
				 $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
				 $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
	ogl_scene
	PUBLIC ogl_core ogl_geometry nlohmann_json::nlohmann_json
	PRIVATE Kangaroo::Kangaroo
)

set_target_properties(ogl_scene PROPERTIES FOLDER libs)

install(
	TARGETS ogl_scene
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/scene/export.hpp
				DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogl/scene
)
