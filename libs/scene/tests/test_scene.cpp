#include <catch2/catch_test_macros.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/scene/SceneComponentRegistration.hpp>

#include <nlohmann/json.hpp>

#include <string>

TEST_CASE("scene request carries graph data", "[scene][smoke]") {
    OGL::Scene::registerSceneComponents();

    const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
        "scene", "buildScene",
        nlohmann::json{{"modelName", "SceneSmokeModel"}, {"bodyCount", 3}, {"source", "test"}});

    REQUIRE(response.success);
    CHECK(response.action == "buildScene");

    const auto scene_graph = response.payload.value("sceneGraph", nlohmann::json::object());
    CHECK(scene_graph.value("modelName", std::string{}) == "SceneSmokeModel");
    CHECK(scene_graph.value("nodeCount", 0) == 3);
}
