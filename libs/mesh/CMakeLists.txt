add_library(ogl_mesh INTERFACE)
add_library(OpenGeoLab::mesh ALIAS ogl_mesh)

set_target_properties(ogl_mesh PROPERTIES FOLDER libs)
