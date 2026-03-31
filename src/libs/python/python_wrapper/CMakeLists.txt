pybind11_add_module(opengeolab_pywrapper src/python_wrapper_module.cpp)
target_compile_features(opengeolab_pywrapper PRIVATE cxx_std_20)
# Suppress pyconfig.h auto-link pragma (same rationale as python_embed).
target_compile_definitions(
    opengeolab_pywrapper PRIVATE $<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>
                                 $<$<PLATFORM_ID:Windows>:Py_ENABLE_SHARED>)
target_link_libraries(opengeolab_pywrapper PRIVATE pybind11::pybind11
                                                   OpenGeoLab::Command)
set_target_properties(
    opengeolab_pywrapper
    PROPERTIES FOLDER "python" LIBRARY_OUTPUT_DIRECTORY
                               "${CMAKE_BINARY_DIR}/bin"
               RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
opengeolab_copy_runtime_dlls(opengeolab_pywrapper)

# Copy all DLLs from OpenCASCADE binary directory to the build output.
# TARGET_RUNTIME_DLLS only captures direct imported targets but misses
# third-party transitive dependencies (freetype, zlib) that OCC needs.

if (WIN32
    AND DEFINED OpenCASCADE_BINARY_DIR
    AND EXISTS "${OpenCASCADE_BINARY_DIR}")
    file(GLOB _occ_runtime_dlls "${OpenCASCADE_BINARY_DIR}/*.dll")
    foreach (_dll IN LISTS _occ_runtime_dlls)
        add_custom_command(
            TARGET opengeolab_pywrapper
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}"
                    "$<TARGET_FILE_DIR:opengeolab_pywrapper>"
            VERBATIM)
    endforeach ()
endif ()

install(
    TARGETS opengeolab_pywrapper
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
