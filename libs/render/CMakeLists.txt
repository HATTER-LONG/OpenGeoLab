add_library(ogl_render INTERFACE)
add_library(OpenGeoLab::render ALIAS ogl_render)

target_link_libraries(ogl_render INTERFACE ogl_scene)

set_target_properties(ogl_render PROPERTIES FOLDER libs)