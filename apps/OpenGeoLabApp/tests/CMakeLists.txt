set(opengeolab_app_test_sources
    test_embedded_python_runtime.cpp ../src/OpenGeoLabController.cpp
    ../src/EmbeddedPythonRuntime.cpp ../include/ogl/app/OpenGeoLabController.hpp
    ../include/ogl/app/EmbeddedPythonRuntime.hpp
)

add_executable(opengeolab_embedded_python_smoke_test ${opengeolab_app_test_sources})

set_target_properties(opengeolab_embedded_python_smoke_test PROPERTIES AUTOMOC ON)

target_compile_features(opengeolab_embedded_python_smoke_test PRIVATE cxx_std_20)
target_compile_definitions(opengeolab_embedded_python_smoke_test PRIVATE Py_NO_LINK_LIB)
set(opengeolab_test_python_no_defaultlib_option /NODEFAULTLIB:python313.lib)
if(NOT MSVC)
  set(opengeolab_test_python_no_defaultlib_option -Xlinker /NODEFAULTLIB:python313.lib)
endif()

target_link_options(
  opengeolab_embedded_python_smoke_test PRIVATE ${opengeolab_test_python_no_defaultlib_option}
)

target_include_directories(
  opengeolab_embedded_python_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
  opengeolab_embedded_python_smoke_test PRIVATE Catch2::Catch2WithMain OGL::Command pybind11::embed
                                                Qt6::Core
)

add_test(NAME opengeolab_embedded_python_smoke_test COMMAND opengeolab_embedded_python_smoke_test)
