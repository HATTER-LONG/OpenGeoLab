set(python_sources
  src/EmbeddedPythonRuntime.cpp
)

set(python_public_headers
  include/opengeolab/python/EmbeddedPythonRuntime.hpp
)

cmake_path(GET Python3_EXECUTABLE PARENT_PATH OPENGEOLAB_PYTHON_HOME)
file(TO_CMAKE_PATH "${OPENGEOLAB_PYTHON_HOME}" OPENGEOLAB_PYTHON_HOME_CMAKE)
file(TO_CMAKE_PATH "${Python3_EXECUTABLE}" OPENGEOLAB_PYTHON_EXECUTABLE_CMAKE)
file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" OPENGEOLAB_PROJECT_SOURCE_DIR_CMAKE)
file(
  TO_CMAKE_PATH
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
  OPENGEOLAB_RUNTIME_OUTPUT_DIRECTORY_CMAKE
)

opengeolab_add_module(
  ogl_python
  ALIAS_NAME
    Python
  SOURCES
    ${python_sources}
  PUBLIC_HEADERS
    ${python_public_headers}
  PUBLIC_LINKS
    OpenGeoLab::Command
    pybind11::embed
    Python3::Python
)
target_compile_definitions(
  ogl_python
  PRIVATE
    OPENGEOLAB_PYTHON_HOME="${OPENGEOLAB_PYTHON_HOME_CMAKE}"
    OPENGEOLAB_PYTHON_EXECUTABLE="${OPENGEOLAB_PYTHON_EXECUTABLE_CMAKE}"
)

if(OPENGEOLAB_BUILD_PYTHON_WRAPPER)
  pybind11_add_module(
    opengeolab_pywrapper
    src/PyWrapperModule.cpp
  )
  target_compile_features(opengeolab_pywrapper PRIVATE cxx_std_20)
  target_link_libraries(
    opengeolab_pywrapper
    PRIVATE
      OpenGeoLab::Command
      OpenGeoLab::Geometry
      pybind11::pybind11
  )
  set_target_properties(
    opengeolab_pywrapper
    PROPERTIES
      FOLDER "python"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  install(
    TARGETS opengeolab_pywrapper
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
endif()

if(OPENGEOLAB_BUILD_TESTS)
  if(NOT TARGET opengeolab_pywrapper)
    message(
      FATAL_ERROR
        "ogl_python_test requires the opengeolab_pywrapper module. Enable OPENGEOLAB_BUILD_PYTHON_WRAPPER."
    )
  endif()

  opengeolab_add_doctest_test(
    ogl_python_test
    SOURCES
      test/EmbeddedPythonRuntimeTest.cpp
    LINKS
      OpenGeoLab::Python
  )
  target_compile_definitions(
    ogl_python_test
    PRIVATE
      OPENGEOLAB_PROJECT_SOURCE_DIR="${OPENGEOLAB_PROJECT_SOURCE_DIR_CMAKE}"
      OPENGEOLAB_TEST_APPLICATION_ROOT="${OPENGEOLAB_RUNTIME_OUTPUT_DIRECTORY_CMAKE}"
  )
  add_dependencies(ogl_python_test opengeolab_pywrapper)
  opengeolab_copy_runtime_dlls(ogl_python_test)
endif()
