add_library(ogl_selection INTERFACE)
add_library(OpenGeoLab::selection ALIAS ogl_selection)

target_link_libraries(ogl_selection INTERFACE ogl_scene)

set_target_properties(ogl_selection PROPERTIES FOLDER libs)