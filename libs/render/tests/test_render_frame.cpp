#include <catch2/catch_test_macros.hpp>

#include <ogl/render/RenderFrame.hpp>

#include <vector>

TEST_CASE("render frame clamps viewport size and honors highlighted node", "[render][unit]") {
    const OGL::Scene::SceneGraph scene_graph(
        "RenderUnit::scene", "RenderUnit",
        {
            {.nodeId = "node-1",
             .displayName = "Node 1",
             .renderPrimitive = "solid-body",
             .conceptualBodyIndex = 1,
             .selectable = true},
            {.nodeId = "node-2",
             .displayName = "Node 2",
             .renderPrimitive = "wire-overlay",
             .conceptualBodyIndex = 2,
             .selectable = true},
        });

    const auto render_frame = OGL::Render::buildRenderFrame(
        scene_graph, {{"viewportWidth", 1}, {"viewportHeight", 0}, {"highlightNodeId", "node-2"}});

    CHECK(render_frame.frameId() == "RenderUnit::scene::frame");
    CHECK(render_frame.sceneId() == "RenderUnit::scene");
    CHECK(render_frame.viewportWidth() == 64);
    CHECK(render_frame.viewportHeight() == 64);
    REQUIRE(render_frame.drawItems().size() == 2);
    CHECK_FALSE(render_frame.drawItems()[0].highlighted);
    CHECK(render_frame.drawItems()[1].highlighted);
    CHECK(render_frame.toJson().at("drawItemCount") == 2);
}
