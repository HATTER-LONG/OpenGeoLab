# ---------------------------------------------------------------------------
# OpenGeoLab Mesh Module
# ---------------------------------------------------------------------------

set(mesh_public_headers
    include/opengeolab/mesh/mesh_element_type.hpp
    include/opengeolab/mesh/mesh_node.hpp
    include/opengeolab/mesh/mesh_element.hpp
    include/opengeolab/mesh/mesh_entry.hpp
    include/opengeolab/mesh/split_mode.hpp
    include/opengeolab/mesh/split_result.hpp
    include/opengeolab/mesh/mesh_topology.hpp
    include/opengeolab/mesh/mesh_split_algorithm.hpp
    include/opengeolab/mesh/mesh_store.hpp
    include/opengeolab/mesh/mesh_module.hpp)

set(mesh_sources src/mesh_store.cpp src/mesh_module.cpp
                  src/mesh_scene_bridge.cpp src/mesh_render_builder.cpp
                  src/mesh_topology.cpp
                  src/mesh_split_algorithm.cpp
                  src/action/clear_mesh_action.cpp
                  src/action/query_mesh_info_action.cpp)

set(mesh_private_links "")

if(OPENGEOLAB_USE_GMSH)
    list(APPEND mesh_sources src/action/generate_mesh_action.cpp)
    list(APPEND mesh_private_links gmsh)
endif()

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
    OpenGeoLab::Geometry
    OpenGeoLab::Scene
    PRIVATE_LINKS
    ${mesh_private_links})

if(OPENGEOLAB_USE_GMSH)
    target_compile_definitions(opengeolab_mesh PRIVATE OPENGEOLAB_USE_GMSH)
endif()

target_include_directories(opengeolab_mesh PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_mesh_test
        SOURCES
        test/mesh_store_test.cpp
        test/mesh_render_builder_test.cpp
        test/mesh_topology_test.cpp
        test/mesh_split_algorithm_test.cpp
        LINKS
        OpenGeoLab::Mesh)
    target_include_directories(opengeolab_mesh_test
                               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(OPENGEOLAB_USE_GMSH)
        target_compile_definitions(opengeolab_mesh_test PRIVATE OPENGEOLAB_USE_GMSH)
    endif()
endif()
