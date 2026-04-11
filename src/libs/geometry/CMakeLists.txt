set(geometry_public_headers
    include/opengeolab/geometry/geometry_module.hpp
    include/opengeolab/geometry/create_box_action.hpp
    include/opengeolab/geometry/create_cylinder_action.hpp
    include/opengeolab/geometry/create_sphere_action.hpp
    include/opengeolab/geometry/create_torus_action.hpp
    include/opengeolab/geometry/delete_entity_action.hpp
    include/opengeolab/geometry/delete_shape_action.hpp
    include/opengeolab/geometry/describe_topology_action.hpp
    include/opengeolab/geometry/import_brep_action.hpp
    include/opengeolab/geometry/import_step_action.hpp
    include/opengeolab/geometry/list_shapes_action.hpp
    include/opengeolab/geometry/list_sub_shapes_action.hpp
    include/opengeolab/geometry/query_entity_info_action.hpp
    include/opengeolab/geometry/query_shape_action.hpp
    include/opengeolab/geometry/shape_entry.hpp
    include/opengeolab/geometry/shape_store.hpp
    include/opengeolab/geometry/tessellate_action.hpp
    include/opengeolab/geometry/tessellator.hpp
    include/opengeolab/geometry/topology_utils.hpp)
set(geometry_sources
    src/geometry_module.cpp
    src/create_box_action.cpp
    src/create_cylinder_action.cpp
    src/create_sphere_action.cpp
    src/create_torus_action.cpp
    src/delete_entity_action.cpp
    src/delete_shape_action.cpp
    src/describe_topology_action.cpp
    src/import_brep_action.cpp
    src/import_step_action.cpp
    src/list_shapes_action.cpp
    src/list_sub_shapes_action.cpp
    src/query_entity_info_action.cpp
    src/query_shape_action.cpp
    src/shape_store.cpp
    src/tessellate_action.cpp
    src/tessellator.cpp
    src/topology_utils.cpp)

opengeolab_add_module(
    opengeolab_geometry
    ALIAS_NAME
    Geometry
    SOURCES
    ${geometry_sources}
    PUBLIC_HEADERS
    ${geometry_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    TKernel
    TKMath
    TKG3d
    TKGeomBase
    TKBRep
    TKTopAlgo
    TKPrim
    TKMesh
    TKDESTEP
    TKDE
    TKXSBase
    TKShHealing
    TKBO)

# OCC runtime DLLs: geometry is the primary OCC consumer, so keep all OCC DLLs
# in the output directory.  This covers the geometry library, its tests, and any
# downstream target that links OpenGeoLab::Geometry.
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
            opengeolab_occ_runtime_sync
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:opengeolab_geometry>"
            COMMAND
                ${CMAKE_COMMAND} "-Druntime_dlls=${_occ_runtime_dlls}"
                "-Dtarget_dir=$<TARGET_FILE_DIR:opengeolab_geometry>" -P
                "${OPENGEOLAB_MODULE_CMAKE_DIR}/copy_runtime_dlls.cmake"
            DEPENDS ${_occ_runtime_dlls}
            COMMENT "Synchronizing OCC runtime DLLs"
            VERBATIM)
        set_target_properties(opengeolab_occ_runtime_sync PROPERTIES FOLDER
                                                                     "lib")
        add_dependencies(opengeolab_geometry opengeolab_occ_runtime_sync)
    endif ()
endif ()

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_geometry_test
        SOURCES
        test/geometry_module_test.cpp
        test/delete_entity_action_test.cpp
        test/describe_topology_action_test.cpp
        test/query_entity_info_action_test.cpp
        test/shape_store_test.cpp
        test/tessellator_test.cpp
        test/topology_utils_test.cpp
        LINKS
        OpenGeoLab::Geometry)
endif ()
