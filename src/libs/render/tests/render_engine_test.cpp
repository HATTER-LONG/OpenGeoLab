/**
 * @file render_engine_test.cpp
 * @brief Unit tests for RenderEngine scene graph binding.
 */
#include <opengeolab/render/render_engine.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Render::RenderEngine;
using OpenGeoLab::Scene::SceneGraph;

TEST_CASE("RenderEngine") {
    SUBCASE("setSceneGraph installs callback on connected graph") {
        RenderEngine engine;
        SceneGraph graph;

        CHECK_FALSE(static_cast<bool>(graph.onChanged));

        engine.setSceneGraph(&graph);

        CHECK(static_cast<bool>(graph.onChanged));
    }

    SUBCASE("setSceneGraph disconnects previously connected graph") {
        RenderEngine engine;
        SceneGraph first_graph;
        SceneGraph second_graph;

        engine.setSceneGraph(&first_graph);
        REQUIRE(static_cast<bool>(first_graph.onChanged));

        engine.setSceneGraph(&second_graph);

        CHECK_FALSE(static_cast<bool>(first_graph.onChanged));
        CHECK(static_cast<bool>(second_graph.onChanged));

        engine.setSceneGraph(nullptr);

        CHECK_FALSE(static_cast<bool>(second_graph.onChanged));
    }
}
