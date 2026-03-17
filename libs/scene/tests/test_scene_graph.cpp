#include <catch2/catch_test_macros.hpp>

#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/scene/SceneGraph.hpp>

TEST_CASE("scene graph builds stable node ids from geometry bodies", "[scene][unit]") {
    const OGL::Geometry::GeometryModel model(
        {.modelName = "SceneUnit", .bodyCount = 2, .source = "unit-test"});

    const auto scene_graph = OGL::Scene::buildSceneGraph(model);

    CHECK(scene_graph.sceneId() == "SceneUnit::scene");
    CHECK(scene_graph.modelName() == "SceneUnit");
    REQUIRE(scene_graph.nodes().size() == 2);
    CHECK(scene_graph.nodes()[0].nodeId == "SceneUnit::body_1");
    CHECK(scene_graph.nodes()[0].renderPrimitive == "solid-body");
    CHECK(scene_graph.nodes()[1].nodeId == "SceneUnit::body_2");
    CHECK(scene_graph.nodes()[1].renderPrimitive == "wire-overlay");
    CHECK(scene_graph.toJson().at("nodeCount") == 2);
}
