add_executable(opengeolab_python_smoke_test_host test_opengeolab_bridge_host.cpp)

target_compile_features(opengeolab_python_smoke_test_host PRIVATE cxx_std_20)
target_compile_definitions(opengeolab_python_smoke_test_host PRIVATE Py_NO_LINK_LIB)
target_compile_definitions(
  opengeolab_python_smoke_test_host
  PRIVATE OPENGEOLAB_PYTHON_TEST_MODULE_DIR="$<TARGET_FILE_DIR:opengeolab>"
          OPENGEOLAB_PYTHON_TEST_SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test_opengeolab_bridge.py"
)

set(opengeolab_python_test_no_defaultlib_option /NODEFAULTLIB:python313.lib)
if(NOT MSVC)
  set(opengeolab_python_test_no_defaultlib_option -Xlinker /NODEFAULTLIB:python313.lib)
endif()

target_link_options(
  opengeolab_python_smoke_test_host PRIVATE ${opengeolab_python_test_no_defaultlib_option}
)

target_link_libraries(
  opengeolab_python_smoke_test_host PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json
                                            pybind11::embed
)

add_test(NAME opengeolab_python_smoke_test COMMAND opengeolab_python_smoke_test_host)

set_target_properties(opengeolab_python_smoke_test_host PROPERTIES FOLDER python)
