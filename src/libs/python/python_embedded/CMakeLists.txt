set(python_embed_sources src/embedded_python_runtime.cpp)

set(python_embed_public_headers
    include/opengeolab/python/embedded_python_runtime.hpp)

cmake_path(GET Python3_EXECUTABLE PARENT_PATH OPENGEOLAB_PYTHON_HOME)
file(TO_CMAKE_PATH "${OPENGEOLAB_PYTHON_HOME}" OPENGEOLAB_PYTHON_HOME_CMAKE)
file(TO_CMAKE_PATH "${Python3_EXECUTABLE}" OPENGEOLAB_PYTHON_EXECUTABLE_CMAKE)
file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" OPENGEOLAB_PROJECT_SOURCE_DIR_CMAKE)
file(TO_CMAKE_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
     OPENGEOLAB_RUNTIME_OUTPUT_DIRECTORY_CMAKE)

opengeolab_add_module(
    opengeolab_python_embed
    ALIAS_NAME
    Python
    SOURCES
    ${python_embed_sources}
    PUBLIC_HEADERS
    ${python_embed_public_headers}
    PUBLIC_LINKS
    # OpenGeoLab::Command
    pybind11::embed
    Python3::Python)
target_compile_definitions(
    opengeolab_python_embed
    PRIVATE OPENGEOLAB_PYTHON_HOME="${OPENGEOLAB_PYTHON_HOME_CMAKE}"
            OPENGEOLAB_PYTHON_EXECUTABLE="${OPENGEOLAB_PYTHON_EXECUTABLE_CMAKE}"
)
opengeolab_copy_runtime_dlls(opengeolab_python_embed)
