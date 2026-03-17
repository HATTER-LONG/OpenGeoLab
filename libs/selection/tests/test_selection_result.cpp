#include <catch2/catch_test_macros.hpp>

#include <ogl/render/RenderFrame.hpp>
#include <ogl/selection/SelectionResult.hpp>

TEST_CASE("selection evaluation returns deterministic box hits", "[selection][unit]") {
    const OGL::Scene::SceneGraph scene_graph(
        "SelectionUnit::scene", "SelectionUnit",
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
            {.nodeId = "node-3",
             .displayName = "Node 3",
             .renderPrimitive = "solid-body",
             .conceptualBodyIndex = 3,
             .selectable = true},
        });
    const auto render_frame = OGL::Render::buildRenderFrame(scene_graph, nlohmann::json::object());

    const auto selection_result = OGL::Selection::evaluateSelection(
        scene_graph, render_frame, {{"mode", "box"}, {"selectionCount", 2}, {"startIndex", 1}});

    CHECK(selection_result.mode() == "box");
    CHECK(selection_result.frameId() == render_frame.frameId());
    REQUIRE(selection_result.hits().size() == 2);
    CHECK(selection_result.hits()[0].nodeId == "node-2");
    CHECK(selection_result.hits()[0].selectionType == "box-select");
    CHECK(selection_result.hits()[1].nodeId == "node-3");
    CHECK(selection_result.hits()[1].hitRank == 2);
    CHECK(selection_result.toJson().at("hitCount") == 2);
}
