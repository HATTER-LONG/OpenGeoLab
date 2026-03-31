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

    TEST_CASE("missing node id increments skipped without changing root visibility") {
        SceneGraph graph;
        REQUIRE(graph.root() != nullptr);
        REQUIRE(graph.root()->isVisible());

        SetVisibilityAction action(graph);
        nlohmann::json param = {{"nodes", {{{"visible", false}}}}};
        auto result = action.execute(param, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
        CHECK(result["skipped"] == 1);
        CHECK(graph.root()->isVisible());
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

#include <opengeolab/scene/list_nodes_action.hpp>

namespace OpenGeoLab::Scene::Tests {

TEST_SUITE("ListNodesAction") {

    TEST_CASE("empty scene returns no nodes") {
        SceneGraph graph;
        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "list_nodes");
        CHECK(result["nodes"].size() == 0);
    }

    TEST_CASE("lists multiple nodes with correct fields") {
        SceneGraph graph;
        auto* a = graph.addNode("Box_1");
        auto* b = graph.addNode("Cyl_1");

        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        auto nodes = result["nodes"];
        REQUIRE(nodes.size() == 2);

        bool found_a = false;
        bool found_b = false;
        for(const auto& n : nodes) {
            CHECK(n.contains("nodeId"));
            CHECK(n.contains("name"));
            CHECK(n.contains("visible"));
            CHECK(n.contains("parentId"));
            if(n["nodeId"] == a->id()) {
                CHECK(n["name"] == "Box_1");
                CHECK(n["visible"] == true);
                CHECK(n["parentId"] == 0);
                found_a = true;
            }
            if(n["nodeId"] == b->id()) {
                CHECK(n["name"] == "Cyl_1");
                CHECK(n["visible"] == true);
                CHECK(n["parentId"] == 0);
                found_b = true;
            }
        }
        CHECK(found_a);
        CHECK(found_b);
    }

    TEST_CASE("visibility reflected in list_nodes") {
        SceneGraph graph;
        auto* node = graph.addNode("Box_1");
        graph.setNodeVisible(node->id(), false);

        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        REQUIRE(result["nodes"].size() == 1);
        CHECK(result["nodes"][0]["visible"] == false);
    }

    TEST_CASE("describe returns valid schema") {
        SceneGraph graph;
        ListNodesAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "list_nodes");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("params"));
        CHECK(desc.contains("returns"));
    }

} // TEST_SUITE

} // namespace OpenGeoLab::Scene::Tests

#include <opengeolab/core/module_data_event.hpp>
#include <opengeolab/scene/scene_module.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

namespace OpenGeoLab::Scene::Tests {

TEST_SUITE("SceneModule") {

    TEST_CASE("module name is scene") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule module(factory);

        CHECK(module.moduleName() == "scene");
    }

    TEST_CASE("sceneGraph accessor returns owned graph") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule module(factory);

        auto* node = module.sceneGraph().addNode("TestNode");
        REQUIRE(node != nullptr);
        CHECK(module.sceneGraph().findNode(node->id()) == node);
    }

    TEST_CASE("set_visibility dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule module(factory);

        auto* node = module.sceneGraph().addNode("Box");
        REQUIRE(node->isVisible());

        nlohmann::json request = {
            {"action", "set_visibility"},
            {"param", {{"nodes", {{{"nodeId", node->id()}, {"visible", false}}}}}}};
        auto result = module.process(request, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("list_nodes dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule module(factory);

        module.sceneGraph().addNode("A");
        module.sceneGraph().addNode("B");

        nlohmann::json request = {{"action", "list_nodes"}, {"param", nlohmann::json::object()}};
        auto result = module.process(request, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["nodes"].size() == 2);
    }

    TEST_CASE("dataChanged emitted on set_visibility mutation") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule module(factory);

        auto* node = module.sceneGraph().addNode("Box");

        int signal_count = 0;
        auto conn = module.dataChanged.connect([&](Core::ModuleDataEvent) { ++signal_count; });

        signal_count = 0;

        nlohmann::json request = {
            {"action", "set_visibility"},
            {"param", {{"nodes", {{{"nodeId", node->id()}, {"visible", false}}}}}}};
        auto result = module.process(request, nullptr);

        CHECK(result["ok"] == true);
        CHECK(signal_count > 0);
    }

} // TEST_SUITE

} // namespace OpenGeoLab::Scene::Tests
