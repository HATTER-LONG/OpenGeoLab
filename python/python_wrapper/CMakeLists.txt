find_package(
  Python
  COMPONENTS Interpreter Development.Module
  REQUIRED
)

set(opengeolab_python_bridge_headers include/ogl/python_wrapper/OpenGeoLabPythonBridge.hpp)
set(opengeolab_python_bridge_sources src/OpenGeoLabPythonBridge.cpp)

add_library(
  opengeolab_python_bridge STATIC ${opengeolab_python_bridge_headers}
                                  ${opengeolab_python_bridge_sources}
)
add_library(OpenGeoLab::python_bridge ALIAS opengeolab_python_bridge)

target_compile_features(opengeolab_python_bridge PUBLIC cxx_std_20)

target_include_directories(opengeolab_python_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  opengeolab_python_bridge PUBLIC ogl_core ogl_geometry nlohmann_json::nlohmann_json
)

pybind11_add_module(opengeolab MODULE src/module.cpp)

target_compile_definitions(opengeolab PRIVATE Py_NO_LINK_LIB)
target_link_options(opengeolab PRIVATE -Xlinker /NODEFAULTLIB:python313.lib)

target_link_libraries(opengeolab PRIVATE opengeolab_python_bridge)

install(
  TARGETS opengeolab
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
)

set_target_properties(opengeolab_python_bridge opengeolab PROPERTIES FOLDER python)
