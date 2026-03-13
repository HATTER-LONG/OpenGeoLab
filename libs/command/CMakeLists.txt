add_library(ogl_command INTERFACE)
add_library(OpenGeoLab::command ALIAS ogl_command)

target_link_libraries(ogl_command INTERFACE ogl_core)

set_target_properties(ogl_command PROPERTIES FOLDER libs)