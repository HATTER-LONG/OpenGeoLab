set(scene_public_headers
    include/opengeolab/scene/bounding_box3d.hpp
    include/opengeolab/scene/display_mode.hpp
    include/opengeolab/scene/geometry_scene_bridge.hpp
    include/opengeolab/scene/pick_component.hpp
    include/opengeolab/scene/pick_id.hpp
    include/opengeolab/scene/render_component.hpp
    include/opengeolab/scene/render_mesh_data.hpp
    include/opengeolab/scene/scene_graph.hpp
    include/opengeolab/scene/scene_node.hpp
    include/opengeolab/scene/topology_index.hpp)

set(scene_sources
    src/bounding_box3d.cpp
    src/geometry_scene_bridge.cpp
    src/scene_graph.cpp
    src/scene_node.cpp
    src/topology_index.cpp)

opengeolab_add_module(
    opengeolab_scene
    ALIAS_NAME
    Scene
    SOURCES
    ${scene_sources}
    PUBLIC_HEADERS
    ${scene_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::Geometry
    glm::glm)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_bounding_box3d_test
        SOURCES test/bounding_box3d_test.cpp
        LINKS OpenGeoLab::Scene)

    opengeolab_add_doctest_test(
        opengeolab_pick_id_test
        SOURCES test/pick_id_test.cpp
        LINKS OpenGeoLab::Scene)

    opengeolab_add_doctest_test(
        opengeolab_scene_node_test
        SOURCES test/scene_node_test.cpp
        LINKS OpenGeoLab::Scene)

    opengeolab_add_doctest_test(
        opengeolab_scene_graph_test
        SOURCES test/scene_graph_test.cpp
        LINKS OpenGeoLab::Scene)

    opengeolab_add_doctest_test(
        opengeolab_topology_index_test
        SOURCES test/topology_index_test.cpp
        LINKS OpenGeoLab::Scene)

    opengeolab_add_doctest_test(
        opengeolab_geometry_scene_bridge_test
        SOURCES test/geometry_scene_bridge_test.cpp
        LINKS OpenGeoLab::Scene)
endif ()
