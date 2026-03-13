add_library(ogl_scene INTERFACE)
add_library(OpenGeoLab::scene ALIAS ogl_scene)

target_link_libraries(ogl_scene INTERFACE ogl_geometry ogl_mesh)

set_target_properties(ogl_scene PROPERTIES FOLDER libs)
