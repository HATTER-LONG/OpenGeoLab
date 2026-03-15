#include <catch2/catch_test_macros.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/render/RenderComponentRegistration.hpp>

#include <nlohmann/json.hpp>

TEST_CASE("render request carries scene and frame data", "[render][smoke]") {
    OGL::Render::registerRenderComponents();

    const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
        "render", "buildFrame",
        nlohmann::json{{"modelName", "RenderSmokeModel"},
                       {"bodyCount", 3},
                       {"viewportWidth", 1440},
                       {"viewportHeight", 900},
                       {"source", "test"}});

    REQUIRE(response.success);
    CHECK(response.action == "buildFrame");

    const auto scene_graph = response.payload.value("sceneGraph", nlohmann::json::object());
    CHECK(scene_graph.value("nodeCount", 0) == 3);

    const auto render_frame = response.payload.value("renderFrame", nlohmann::json::object());
    CHECK(render_frame.value("drawItemCount", 0) == 3);
}
