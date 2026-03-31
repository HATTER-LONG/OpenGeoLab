/**
 * @file scene_module_test.cpp
 * @brief Unit tests for SceneModule actions
 */

#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/set_visibility_action.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Scene::Tests {

TEST_SUITE("SetVisibilityAction") {

    TEST_CASE("single node set invisible") {
        SceneGraph graph;
        auto* node = graph.addNode("Box_1");
        REQUIRE(node != nullptr);
        REQUIRE(node->isVisible());

        SetVisibilityAction action(graph);
        nlohmann::json param = {{"nodes", {{{"nodeId", node->id()}, {"visible", false}}}}};
        auto result = action.execute(param, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "set_visibility");
        CHECK(result["updated"] == 1);
        CHECK(result["skipped"] == 0);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("batch set visibility") {
        SceneGraph graph;
        auto* a = graph.addNode("A");
        auto* b = graph.addNode("B");
        auto* c = graph.addNode("C");

        SetVisibilityAction action(graph);
        nlohmann::json param = {{"nodes",
                                 {{{"nodeId", a->id()}, {"visible", false}},
                                  {{"nodeId", b->id()}, {"visible", false}},
                                  {{"nodeId", c->id()}, {"visible", true}}}}};
        auto result = action.execute(param, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 2);
        CHECK(result["skipped"] == 0);
        CHECK_FALSE(a->isVisible());
        CHECK_FALSE(b->isVisible());
        CHECK(c->isVisible());
    }

    TEST_CASE("node not found increments skipped") {
        SceneGraph graph;
        SetVisibilityAction action(graph);
        nlohmann::json param = {{"nodes", {{{"nodeId", 999}, {"visible", false}}}}};
        auto result = action.execute(param, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
        CHECK(result["skipped"] == 1);
    }

    TEST_CASE("empty nodes array succeeds") {
        SceneGraph graph;
        SetVisibilityAction action(graph);
        nlohmann::json param = {{"nodes", nlohmann::json::array()}};
        auto result = action.execute(param, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
        CHECK(result["skipped"] == 0);
    }

    TEST_CASE("no actual change yields updated=0") {
        SceneGraph graph;
        auto* node = graph.addNode("Box_1");
        REQUIRE(node != nullptr);
        REQUIRE(node->isVisible());

        SetVisibilityAction action(graph);
        nlohmann::json param = {{"nodes", {{{"nodeId", node->id()}, {"visible", true}}}}};
        auto result = action.execute(param, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
        CHECK(result["skipped"] == 0);
    }

    TEST_CASE("describe returns valid schema") {
        SceneGraph graph;
        SetVisibilityAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "set_visibility");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("params"));
        CHECK(desc.contains("returns"));
    }

} // TEST_SUITE

} // namespace OpenGeoLab::Scene::Tests
