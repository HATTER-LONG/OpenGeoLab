include(GenerateExportHeader)

set(ogl_render_headers include/ogl/render/PlaceholderRenderFrame.hpp
											 include/ogl/render/RenderComponentRegistration.hpp
)
set(ogl_render_sources src/PlaceholderRenderFrame.cpp src/RenderComponentRegistration.cpp)

add_library(ogl_render ${OPENGEOLAB_LIBRARY_TYPE} ${ogl_render_headers} ${ogl_render_sources})
add_library(OpenGeoLab::render ALIAS ogl_render)

generate_export_header(
	ogl_render EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/render/export.hpp
	EXPORT_MACRO_NAME OGL_RENDER_EXPORT
)

target_compile_features(ogl_render PUBLIC cxx_std_20)

if(NOT OPENGEOLAB_BUILD_SHARED_LIBS)
	target_compile_definitions(ogl_render PUBLIC OGL_RENDER_STATIC_DEFINE)
endif()

target_include_directories(
	ogl_render
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
				 $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
				 $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
	ogl_render
	PUBLIC ogl_core ogl_scene nlohmann_json::nlohmann_json
	PRIVATE Kangaroo::Kangaroo
)

set_target_properties(ogl_render PROPERTIES FOLDER libs)

install(
	TARGETS ogl_render
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ogl/render/export.hpp
				DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogl/render
)