add_library(glad STATIC src/gl.c)
target_include_directories(
    glad PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
set_target_properties(glad PROPERTIES FOLDER "third_party"
                                      POSITION_INDEPENDENT_CODE ON)
