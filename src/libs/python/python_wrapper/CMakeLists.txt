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
    PROPERTIES FOLDER "python"
               LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
               RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
opengeolab_copy_runtime_dlls(opengeolab_pywrapper)

# Keep OCC runtime DLLs in sync even when pywrapper itself does not need a
# rebuild. OCC is developed and installed out-of-tree, so relying on
# opengeolab_pywrapper POST_BUILD hooks misses external runtime updates.

set(_occ_runtime_dir "")
if (DEFINED OpenCASCADE_BINARY_DIR AND EXISTS "${OpenCASCADE_BINARY_DIR}")
    set(_occ_runtime_dir "${OpenCASCADE_BINARY_DIR}")
elseif (TARGET TKernel)
    get_target_property(_occ_tkernel_location TKernel IMPORTED_LOCATION)
    if (NOT _occ_tkernel_location)
        get_target_property(_occ_tkernel_configs TKernel
                            IMPORTED_CONFIGURATIONS)
        foreach (_occ_cfg IN LISTS _occ_tkernel_configs)
            string(TOUPPER "${_occ_cfg}" _occ_cfg_upper)
            get_target_property(_occ_tkernel_location TKernel
                                "IMPORTED_LOCATION_${_occ_cfg_upper}")
            if (_occ_tkernel_location)
                break()
            endif ()
        endforeach ()
    endif ()

    if (_occ_tkernel_location)
        cmake_path(GET _occ_tkernel_location PARENT_PATH _occ_runtime_dir)
    endif ()
endif ()

if (WIN32
    AND _occ_runtime_dir
    AND EXISTS "${_occ_runtime_dir}")
    file(GLOB _occ_runtime_dlls CONFIGURE_DEPENDS "${_occ_runtime_dir}/*.dll")
    if (_occ_runtime_dlls)
        add_custom_target(
            opengeolab_pywrapper_occ_runtime_sync
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:opengeolab_pywrapper>"
            COMMAND
                ${CMAKE_COMMAND} "-Druntime_dlls=${_occ_runtime_dlls}"
                "-Dtarget_dir=$<TARGET_FILE_DIR:opengeolab_pywrapper>" -P
                "${OPENGEOLAB_MODULE_CMAKE_DIR}/copy_runtime_dlls.cmake"
            DEPENDS ${_occ_runtime_dlls}
            VERBATIM)
        set_target_properties(opengeolab_pywrapper_occ_runtime_sync
                              PROPERTIES FOLDER "python")
        add_dependencies(opengeolab_pywrapper
                         opengeolab_pywrapper_occ_runtime_sync)
    endif ()
endif ()

install(
    TARGETS opengeolab_pywrapper
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
