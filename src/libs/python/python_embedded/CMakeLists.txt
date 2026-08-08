set(python_embed_sources src/embedded_python_runtime.cpp)

set(python_embed_public_headers
    include/opengeolab/python_embed/embedded_python_runtime.hpp)

if (NOT OPENGEOLAB_PYTHON_EXECUTABLE)
    set(OPENGEOLAB_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
endif ()

# Virtual environments have different layouts on Windows and Unix. Ask the
# selected interpreter for its base prefix instead of deriving PYTHONHOME from
# the executable's parent directory.
execute_process(
    COMMAND "${OPENGEOLAB_PYTHON_EXECUTABLE}" -c
            "import sys; print(sys.base_prefix)"
    RESULT_VARIABLE _python_prefix_result
    OUTPUT_VARIABLE OPENGEOLAB_PYTHON_HOME
    ERROR_VARIABLE _python_prefix_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT _python_prefix_result EQUAL 0 OR OPENGEOLAB_PYTHON_HOME STREQUAL "")
    message(
        FATAL_ERROR
            "Failed to query Python base prefix from ${OPENGEOLAB_PYTHON_EXECUTABLE}: "
            "${_python_prefix_error}")
endif ()

file(TO_CMAKE_PATH "${OPENGEOLAB_PYTHON_HOME}" OPENGEOLAB_PYTHON_HOME_CMAKE)
file(TO_CMAKE_PATH "${OPENGEOLAB_PYTHON_EXECUTABLE}"
     OPENGEOLAB_PYTHON_EXECUTABLE_CMAKE)
file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" OPENGEOLAB_PROJECT_SOURCE_DIR_CMAKE)
file(TO_CMAKE_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
     OPENGEOLAB_RUNTIME_OUTPUT_DIRECTORY_CMAKE)

opengeolab_add_module(
    opengeolab_python_embed
    ALIAS_NAME
    Python_Embed
    SOURCES
    ${python_embed_sources}
    PUBLIC_HEADERS
    ${python_embed_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    pybind11::embed
    Python3::Python)

# Suppress pyconfig.h auto-link pragma on Windows.
target_compile_definitions(
    opengeolab_python_embed
    PUBLIC $<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>
           $<$<PLATFORM_ID:Windows>:Py_ENABLE_SHARED>
    PRIVATE OPENGEOLAB_PYTHON_HOME="${OPENGEOLAB_PYTHON_HOME_CMAKE}"
            OPENGEOLAB_PYTHON_EXECUTABLE="${OPENGEOLAB_PYTHON_EXECUTABLE_CMAKE}"
)

if (DEFINED OPENGEOLAB_PYVENV_SITE_PACKAGES)
    target_compile_definitions(
        opengeolab_python_embed
        PRIVATE
            OPENGEOLAB_PYVENV_SITE_PACKAGES="${OPENGEOLAB_PYVENV_SITE_PACKAGES}"
    )
endif ()

opengeolab_copy_runtime_dlls(opengeolab_python_embed)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_python_embed_api_test SOURCES
        tests/embedded_python_runtime_api_test.cpp LINKS
        OpenGeoLab::Python_Embed)
endif ()
