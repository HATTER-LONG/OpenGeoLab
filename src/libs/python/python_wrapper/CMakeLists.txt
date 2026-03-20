pybind11_add_module(opengeolab_pywrapper src/python_wrapper_module.cpp)
target_compile_features(opengeolab_pywrapper PRIVATE cxx_std_20)
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
