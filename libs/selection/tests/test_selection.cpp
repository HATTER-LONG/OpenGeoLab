#include <catch2/catch_test_macros.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <nlohmann/json.hpp>

TEST_CASE("selection request carries scene render and hit data", "[selection][smoke]") {
    OGL::Selection::registerSelectionComponents();

    SECTION("pickEntity returns one hit") {
        const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
            "selection", "pickEntity",
            nlohmann::json{{"modelName", "SelectionSmokeModel"},
                           {"bodyCount", 4},
                           {"viewportWidth", 1024},
                           {"viewportHeight", 768},
                           {"screenX", 90},
                           {"screenY", 30},
                           {"source", "test"}});

        REQUIRE(response.success);
        CHECK(response.action == "pickEntity");

        const auto scene_graph = response.payload.value("sceneGraph", nlohmann::json::object());
        CHECK(scene_graph.value("nodeCount", 0) == 4);

        const auto render_frame = response.payload.value("renderFrame", nlohmann::json::object());
        CHECK(render_frame.value("drawItemCount", 0) == 4);

        const auto selection_result =
            response.payload.value("selectionResult", nlohmann::json::object());
        CHECK(selection_result.value("hitCount", 0) == 1);
    }

    SECTION("boxSelect returns multiple hits") {
        const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
            "selection", "boxSelect",
            nlohmann::json{{"modelName", "SelectionSmokeModel"},
                           {"bodyCount", 4},
                           {"viewportWidth", 1024},
                           {"viewportHeight", 768},
                           {"selectionCount", 3},
                           {"source", "test"}});

        REQUIRE(response.success);
        CHECK(response.action == "boxSelect");

        const auto selection_result =
            response.payload.value("selectionResult", nlohmann::json::object());
        CHECK(selection_result.value("hitCount", 0) == 3);
        CHECK(selection_result.value("mode", std::string{}) == "box");
    }
}
