pybind11_add_module(opengeolab_pywrapper src/python_wrapper_module.cpp)
target_compile_features(opengeolab_pywrapper PRIVATE cxx_std_20)

# Suppress pyconfig.h auto-link pragma (same rationale as python_embed).
target_compile_definitions(
    opengeolab_pywrapper
    PRIVATE $<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>
            $<$<PLATFORM_ID:Windows>:Py_ENABLE_SHARED>)
target_link_libraries(opengeolab_pywrapper PRIVATE pybind11::pybind11)
set_target_properties(
    opengeolab_pywrapper
    PROPERTIES FOLDER "python"
               LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
               RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
install(
    TARGETS opengeolab_pywrapper
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
