ogl_add_catch2_test(opengeolab_python_test_host
  SOURCES test_opengeolab_bridge_host.cpp
  LINKS   nlohmann_json::nlohmann_json pybind11::embed
  CTEST_NAME opengeolab_python_test
)

ogl_add_catch2_test(opengeolab_python_interop_test
  SOURCES test_python_json_interop.cpp
  LINKS   OGL::PythonInterop pybind11::embed
)

target_compile_definitions(
  opengeolab_python_test_host
  PRIVATE Py_NO_LINK_LIB
          OPENGEOLAB_PYTHON_TEST_MODULE_DIR="$<TARGET_FILE_DIR:opengeolab>"
          OPENGEOLAB_PYTHON_TEST_SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test_opengeolab_bridge.py"
)

set(opengeolab_python_test_no_defaultlib_option /NODEFAULTLIB:python313.lib)
if(NOT MSVC)
  set(opengeolab_python_test_no_defaultlib_option -Xlinker /NODEFAULTLIB:python313.lib)
endif()

target_link_options(
  opengeolab_python_test_host PRIVATE ${opengeolab_python_test_no_defaultlib_option}
)

target_compile_definitions(opengeolab_python_interop_test PRIVATE Py_NO_LINK_LIB)
target_link_options(
  opengeolab_python_interop_test PRIVATE ${opengeolab_python_test_no_defaultlib_option}
)

set_target_properties(opengeolab_python_test_host PROPERTIES FOLDER python)
set_target_properties(opengeolab_python_interop_test PROPERTIES FOLDER python)
