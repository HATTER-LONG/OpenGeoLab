set(scene_public_headers
    include/opengeolab/scene/render_mesh_data.hpp
    include/opengeolab/scene/bounding_box.hpp
    include/opengeolab/scene/scene_node.hpp
    include/opengeolab/scene/scene_graph.hpp)

set(scene_sources src/scene_graph.cpp)

opengeolab_add_module(
    opengeolab_scene
    ALIAS_NAME
    Scene
    SOURCES
    ${scene_sources}
    PUBLIC_HEADERS
    ${scene_public_headers}
    PUBLIC_LINKS
    glm::glm)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_scene_graph_test SOURCES tests/scene_graph_test.cpp LINKS
        OpenGeoLab::Scene)
endif ()
