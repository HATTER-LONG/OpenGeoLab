/**
 * @file split_mesh_action_test.cpp
 * @brief SplitMeshAction unit tests
 */

#include "../src/action/split_mesh_action.hpp"

#include <opengeolab/mesh/mesh_element.hpp>
#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_node.hpp>
#include <opengeolab/mesh/mesh_store.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using OpenGeoLab::Mesh::MeshElement;
using OpenGeoLab::Mesh::MeshElementType;
using OpenGeoLab::Mesh::MeshEntry;
using OpenGeoLab::Mesh::MeshNode;
using OpenGeoLab::Mesh::MeshStore;
using OpenGeoLab::Mesh::SplitMeshAction;

static MeshEntry makeTestTriangle() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}},
    };
    MeshElement tri{};
    tri.type = MeshElementType::Triangle;
    tri.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    entry.elements = {tri};
    return entry;
}

TEST_CASE("SplitMeshAction: describe() structure") {
    MeshStore store;
    SplitMeshAction action(store);
    const auto desc = action.describe();

    CHECK(desc["name"] == "split_mesh");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc.contains("returns"));
    CHECK(desc["params"].contains("shapeId"));
    CHECK(desc["params"].contains("selections"));
    CHECK(desc["params"].contains("mode"));
    CHECK(desc["returns"].contains("ok"));
    CHECK(desc["returns"].contains("action"));
    CHECK(desc["returns"].contains("shapeId"));
    CHECK(desc["returns"].contains("summary"));
}

TEST_CASE("SplitMeshAction: missing shapeId") {
    MeshStore store;
    SplitMeshAction action(store);
    const auto result =
        action.execute({{"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);
    CHECK(result["ok"] == false);
    CHECK(result["action"] == "split_mesh");
}

TEST_CASE("SplitMeshAction: missing selections") {
    MeshStore store;
    SplitMeshAction action(store);
    const auto result = action.execute({{"shapeId", 1}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("SplitMeshAction: unknown shapeId") {
    MeshStore store;
    SplitMeshAction action(store);
    const auto result = action.execute(
        {{"shapeId", 99}, {"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("SplitMeshAction: invalid mode string") {
    MeshStore store;
    store.setMesh(1, makeTestTriangle());
    SplitMeshAction action(store);
    const auto result = action.execute({{"shapeId", 1},
                                        {"selections", {{{"type", "edge"}, {"localId", 1}}}},
                                        {"mode", "invalid_mode"}},
                                       nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("SplitMeshAction: empty selections array") {
    MeshStore store;
    store.setMesh(1, makeTestTriangle());
    SplitMeshAction action(store);
    const auto result =
        action.execute({{"shapeId", 1}, {"selections", nlohmann::json::array()}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("SplitMeshAction: successful edge split") {
    MeshStore store;
    store.setMesh(1, makeTestTriangle());

    const auto* topo = store.getTopology(1);
    REQUIRE(topo != nullptr);
    REQUIRE(!topo->edges.empty());

    SplitMeshAction action(store);
    const auto result = action.execute(
        {{"shapeId", 1}, {"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "split_mesh");
    CHECK(result["shapeId"] == 1);
    CHECK(result.contains("summary"));

    const auto* entry = store.find(1);
    REQUIRE(entry != nullptr);
    CHECK(entry->elements.size() > 1);
    CHECK(entry->nodes.size() > 3);
}

TEST_CASE("SplitMeshAction: successful node split TriaThree") {
    MeshStore store;
    store.setMesh(1, makeTestTriangle());

    SplitMeshAction action(store);
    const auto result = action.execute({{"shapeId", 1},
                                        {"selections",
                                         {{{"type", "node"}, {"localId", 1}},
                                          {{"type", "node"}, {"localId", 2}},
                                          {{"type", "node"}, {"localId", 3}}}},
                                        {"mode", "tria_three"}},
                                       nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["shapeId"] == 1);

    const auto* entry = store.find(1);
    REQUIRE(entry != nullptr);
    CHECK(entry->elements.size() == 3);
    CHECK(entry->nodes.size() == 4);
}

TEST_CASE("SplitMeshAction: accepts Tri6 elements") {
    MeshStore store;
    MeshEntry entry;
    entry.shapeId = 99;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 0.0F, 0.0F}}, MeshNode{{1.0F, 2.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}}, MeshNode{{1.5F, 1.0F, 0.0F}}, MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    MeshElement tri6{};
    tri6.type = MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};
    store.setMesh(99, std::move(entry));

    SplitMeshAction action(store);
    const auto result = action.execute(
        {{"shapeId", 99}, {"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);
    CHECK(result["ok"].get<bool>() == true);
}

TEST_CASE("SplitMeshAction: rejects Quad9 elements") {
    MeshStore store;
    MeshEntry entry;
    entry.shapeId = 100;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 2.0F, 0.0F}},
        MeshNode{{0.0F, 2.0F, 0.0F}}, MeshNode{{1.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 1.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}}, MeshNode{{0.0F, 1.0F, 0.0F}}, MeshNode{{1.0F, 1.0F, 0.0F}},
    };
    MeshElement quad9{};
    quad9.type = MeshElementType::Quad9;
    quad9.nodeLocalIds = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    entry.elements = {quad9};
    store.setMesh(100, std::move(entry));

    SplitMeshAction action(store);
    const auto result = action.execute(
        {{"shapeId", 100}, {"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);
    CHECK(result["ok"].get<bool>() == false);
    CHECK(result["summary"].get<std::string>().find("Quad9") != std::string::npos);
}
