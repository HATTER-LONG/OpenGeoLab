set(mesh_public_headers
    include/opengeolab/mesh/mesh_module.hpp
    include/opengeolab/mesh/mesh_types.hpp
    include/opengeolab/mesh/mesh_entry.hpp
    include/opengeolab/mesh/mesh_store.hpp
    include/opengeolab/mesh/mesh_params.hpp
    include/opengeolab/mesh/gmsh_bridge.hpp
    include/opengeolab/mesh/mesh_visual_builder.hpp
    include/opengeolab/mesh/generate_surface_mesh_action.hpp
    include/opengeolab/mesh/generate_volume_mesh_action.hpp
    include/opengeolab/mesh/delete_mesh_action.hpp
    include/opengeolab/mesh/query_mesh_action.hpp
    include/opengeolab/mesh/list_meshes_action.hpp)
set(mesh_sources
    src/mesh_module.cpp
    src/mesh_entry.cpp
    src/mesh_store.cpp
    src/mesh_params.cpp
    src/gmsh_bridge.cpp
    src/mesh_visual_builder.cpp
    src/generate_surface_mesh_action.cpp
    src/generate_volume_mesh_action.cpp
    src/delete_mesh_action.cpp
    src/query_mesh_action.cpp
    src/list_meshes_action.cpp)

opengeolab_add_module(
    opengeolab_mesh
    ALIAS_NAME
    Mesh
    SOURCES
    ${mesh_sources}
    PUBLIC_HEADERS
    ${mesh_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    gmsh::shared
    PRIVATE_LINKS
    OpenGeoLab::Geometry
    TKernel
    TKMath
    TKBRep)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_mesh_types_test SOURCES test/mesh_types_test.cpp LINKS
        OpenGeoLab::Mesh)
    opengeolab_add_doctest_test(
        opengeolab_mesh_entry_test SOURCES test/mesh_entry_test.cpp LINKS
        OpenGeoLab::Mesh)
    opengeolab_add_doctest_test(
        opengeolab_mesh_store_test SOURCES test/mesh_store_test.cpp LINKS
        OpenGeoLab::Mesh)
    opengeolab_add_doctest_test(
        opengeolab_gmsh_bridge_test SOURCES test/gmsh_bridge_test.cpp LINKS
        OpenGeoLab::Mesh TKPrim)
endif ()
