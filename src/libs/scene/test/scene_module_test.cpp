/**
 * @file scene_module_test.cpp
 * @brief Tests for SetVisibilityAction, ListNodesAction, and SceneModule.
 */

#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_module.hpp>
#include <opengeolab/scene/set_visibility_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Scene::ListNodesAction;
using OpenGeoLab::Scene::NodeId;
using OpenGeoLab::Scene::SceneGraph;
using OpenGeoLab::Scene::SceneModule;
using OpenGeoLab::Scene::SceneNode;
using OpenGeoLab::Scene::SetVisibilityAction;

TEST_SUITE("SetVisibilityAction") {
    TEST_CASE("single node set invisible") {
        SceneGraph graph;
        const NodeId node_id = graph.addNode("A");
        REQUIRE(node_id != 0);
        SceneNode* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        graph.setNodeSource(node_id, "geometry", 100);

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"id", 100}, {"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK(result["skipped"] == 0);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("batch set visibility") {
        SceneGraph graph;
        const NodeId a_id = graph.addNode("A");
        const NodeId b_id = graph.addNode("B");
        const NodeId c_id = graph.addNode("C");
        REQUIRE(a_id != 0);
        REQUIRE(b_id != 0);
        REQUIRE(c_id != 0);
        SceneNode* a = graph.findNode(a_id);
        SceneNode* b = graph.findNode(b_id);
        SceneNode* c = graph.findNode(c_id);
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(c != nullptr);
        graph.setNodeSource(a_id, "geometry", 10);
        graph.setNodeSource(b_id, "geometry", 20);
        graph.setNodeSource(c_id, "geometry", 30);

        SetVisibilityAction action(graph);
        auto result = action.execute({{"type", "geometry"},
                                      {"nodes",
                                       {{{"id", 10}, {"visible", false}},
                                        {{"id", 20}, {"visible", false}},
                                        {{"id", 30}, {"visible", true}}}}},
                                     nullptr);

        CHECK(result["updated"] == 2);
        CHECK(result["skipped"] == 1);
        CHECK_FALSE(a->isVisible());
        CHECK_FALSE(b->isVisible());
        CHECK(c->isVisible());
    }

    TEST_CASE("source not found increments skipped") {
        SceneGraph graph;
        SetVisibilityAction action(graph);

        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"id", 999}, {"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["skipped"] == 1);
        CHECK(result["updated"] == 0);
    }

    TEST_CASE("missing id increments skipped") {
        SceneGraph graph;
        graph.addNode("A");

        SetVisibilityAction action(graph);
        auto result =
            action.execute({{"type", "geometry"}, {"nodes", {{{"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["skipped"] == 1);
        CHECK(result["updated"] == 0);
    }

    TEST_CASE("empty nodes array succeeds") {
        SceneGraph graph;
        SetVisibilityAction action(graph);

        auto result =
            action.execute({{"type", "geometry"}, {"nodes", nlohmann::json::array()}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
        CHECK(result["skipped"] == 0);
    }

    TEST_CASE("no actual change yields updated=0") {
        SceneGraph graph;
        const NodeId node_id = graph.addNode("A");
        REQUIRE(node_id != 0);
        SceneNode* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        graph.setNodeSource(node_id, "geometry", 42);
        CHECK(node->isVisible());

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"id", 42}, {"visible", true}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
    }

    TEST_CASE("type=node uses internal nodeId") {
        SceneGraph graph;
        const NodeId node_id = graph.addNode("A");
        REQUIRE(node_id != 0);
        SceneNode* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "node"}, {"nodes", {{{"id", node_id}, {"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("describe returns valid schema") {
        SceneGraph graph;
        SetVisibilityAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "set_visibility");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("type"));
        CHECK(desc["params"].contains("nodes"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"]["ok"]["description"] ==
              "true when the action completes successfully.");
    }
}

TEST_SUITE("ListNodesAction") {
    TEST_CASE("empty scene returns no nodes") {
        SceneGraph graph;
        ListNodesAction action(graph);

        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "list_nodes");
        CHECK(result["nodes"].size() == 0);
    }

    TEST_CASE("lists multiple nodes with source info") {
        SceneGraph graph;
        const NodeId a_id = graph.addNode("Alpha");
        const NodeId b_id = graph.addNode("Beta");
        REQUIRE(a_id != 0);
        REQUIRE(b_id != 0);
        SceneNode* a = graph.findNode(a_id);
        SceneNode* b = graph.findNode(b_id);
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);
        graph.setNodeSource(a_id, "geometry", 10);
        graph.setNodeSource(b_id, "geometry", 20);

        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        auto& nodes = result["nodes"];
        CHECK(nodes.size() == 2);

        bool found_alpha = false;
        bool found_beta = false;
        for(const auto& n : nodes) {
            CHECK(n.contains("sourceType"));
            CHECK(n.contains("sourceId"));
            CHECK(n.contains("name"));
            CHECK(n.contains("visible"));
            if(n["name"] == "Alpha") {
                found_alpha = true;
                CHECK(n["sourceType"] == "geometry");
                CHECK(n["sourceId"] == 10);
                CHECK(n["visible"] == true);
            }
            if(n["name"] == "Beta") {
                found_beta = true;
                CHECK(n["sourceType"] == "geometry");
                CHECK(n["sourceId"] == 20);
                CHECK(n["visible"] == true);
            }
        }
        CHECK(found_alpha);
        CHECK(found_beta);
    }

    TEST_CASE("visibility reflected in list_nodes") {
        SceneGraph graph;
        const NodeId node_id = graph.addNode("X");
        REQUIRE(node_id != 0);
        SceneNode* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        graph.setNodeSource(node_id, "geometry", 5);
        graph.setNodeVisible(node_id, false);

        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        CHECK(result["nodes"].size() == 1);
        CHECK(result["nodes"][0]["visible"] == false);
    }

    TEST_CASE("describe returns valid schema") {
        SceneGraph graph;
        ListNodesAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "list_nodes");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"]["ok"]["description"] ==
              "true when the action completes successfully.");
    }
}

TEST_SUITE("SceneModule") {
    TEST_CASE("module name is scene") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        CHECK(mod.moduleName() == "scene");
    }

    TEST_CASE("sceneGraph accessor returns owned graph") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        const NodeId node_id = mod.sceneGraph().addNode("test");
        REQUIRE(node_id != 0);
        SceneNode* node = mod.sceneGraph().findNode(node_id);
        REQUIRE(node != nullptr);
        CHECK(mod.sceneGraph().findNode(node_id) == node);
    }

    TEST_CASE("set_visibility dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        const NodeId node_id = mod.sceneGraph().addNode("A");
        REQUIRE(node_id != 0);
        SceneNode* node = mod.sceneGraph().findNode(node_id);
        REQUIRE(node != nullptr);
        mod.sceneGraph().setNodeSource(node_id, "geometry", 77);

        auto result = mod.process(
            {{"action", "set_visibility"},
             {"param", {{"type", "geometry"}, {"nodes", {{{"id", 77}, {"visible", false}}}}}}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("list_nodes dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        const NodeId node_id = mod.sceneGraph().addNode("B");
        REQUIRE(node_id != 0);
        SceneNode* node = mod.sceneGraph().findNode(node_id);
        REQUIRE(node != nullptr);
        mod.sceneGraph().setNodeSource(node_id, "geometry", 88);

        auto result = mod.process({{"action", "list_nodes"}, {"param", {}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["nodes"].size() == 1);
        CHECK(result["nodes"][0]["sourceId"] == 88);
    }

    TEST_CASE("dataChanged emitted on set_visibility mutation") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        const NodeId node_id = mod.sceneGraph().addNode("C");
        REQUIRE(node_id != 0);
        SceneNode* node = mod.sceneGraph().findNode(node_id);
        REQUIRE(node != nullptr);
        mod.sceneGraph().setNodeSource(node_id, "geometry", 99);

        int signal_count = 0;
        auto conn =
            mod.dataChanged.connect([&](OpenGeoLab::Core::ModuleDataEvent) { ++signal_count; });

        [[maybe_unused]] auto result = mod.process(
            {{"action", "set_visibility"},
             {"param", {{"type", "geometry"}, {"nodes", {{{"id", 99}, {"visible", false}}}}}}},
            nullptr);

        CHECK(signal_count > 0);
    }
}
