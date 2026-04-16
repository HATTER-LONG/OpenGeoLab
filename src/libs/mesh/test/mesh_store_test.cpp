/**
 * @file mesh_store_test.cpp
 * @brief MeshStore unit tests
 */

#include <opengeolab/mesh/mesh_module.hpp>
#include <opengeolab/mesh/mesh_store.hpp>

#include "../src/action/clear_mesh_action.hpp"
#ifdef OPENGEOLAB_USE_GMSH
#include "../src/action/generate_mesh_action.hpp"
#endif
#include "../src/action/query_mesh_info_action.hpp"

#include <opengeolab/core/progress_callback.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>

#include <BRepPrimAPI_MakeBox.hxx>

#include <algorithm>

using OpenGeoLab::Mesh::MeshElement;
using OpenGeoLab::Mesh::MeshElementType;
using OpenGeoLab::Mesh::MeshEntry;
using OpenGeoLab::Mesh::MeshNode;
using OpenGeoLab::Mesh::MeshStore;
using OpenGeoLab::Mesh::MeshTopology;

namespace {

MeshEntry makeTriangleMesh(uint32_t shape_id) {
    MeshEntry entry;
    entry.shapeId = shape_id;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}},
        MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    MeshElement tri;
    tri.type = MeshElementType::Triangle;
    tri.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    entry.elements = {tri};
    return entry;
}

} // namespace

TEST_CASE("MeshStore: setMesh and find") {
    MeshStore store;
    CHECK(store.empty());
    CHECK(store.size() == 0);

    store.setMesh(1, makeTriangleMesh(1));
    CHECK_FALSE(store.empty());
    CHECK(store.size() == 1);

    const auto* entry = store.find(1);
    REQUIRE(entry != nullptr);
    CHECK(entry->shapeId == 1);
    CHECK(entry->nodes.size() == 3);
    CHECK(entry->elements.size() == 1);
    CHECK(entry->elements[0].type == MeshElementType::Triangle);
}

TEST_CASE("MeshStore: find returns nullptr for missing shapeId") {
    MeshStore store;
    CHECK(store.find(42) == nullptr);
}

TEST_CASE("MeshStore: setMesh replaces existing data") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));

    auto replacement = makeTriangleMesh(1);
    replacement.nodes.push_back(MeshNode{{2.0F, 0.0F, 0.0F}});
    store.setMesh(1, std::move(replacement));

    CHECK(store.size() == 1);
    const auto* entry = store.find(1);
    REQUIRE(entry != nullptr);
    CHECK(entry->nodes.size() == 4);
}

TEST_CASE("MeshStore: removeMesh") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    store.setMesh(2, makeTriangleMesh(2));
    CHECK(store.size() == 2);

    CHECK(store.removeMesh(1));
    CHECK(store.size() == 1);
    CHECK(store.find(1) == nullptr);
    CHECK(store.find(2) != nullptr);

    CHECK_FALSE(store.removeMesh(99));
}

TEST_CASE("MeshStore: clear") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    store.setMesh(2, makeTriangleMesh(2));

    store.clear();
    CHECK(store.empty());
    CHECK(store.find(1) == nullptr);
    CHECK(store.find(2) == nullptr);
}

TEST_CASE("MeshStore: allShapeIds") {
    MeshStore store;
    store.setMesh(3, makeTriangleMesh(3));
    store.setMesh(7, makeTriangleMesh(7));

    auto ids = store.allShapeIds();
    CHECK(ids.size() == 2);
    std::sort(ids.begin(), ids.end());
    CHECK(ids[0] == 3);
    CHECK(ids[1] == 7);
}

TEST_CASE("MeshStore: signals fire on mutations") {
    MeshStore store;
    uint32_t added_id = 0;
    uint32_t removed_id = 0;
    bool cleared = false;

    const auto added_connection =
        store.meshAdded.connect([&](uint32_t id, const MeshEntry&) { added_id = id; });
    const auto removed_connection =
        store.meshRemoved.connect([&](uint32_t id) { removed_id = id; });
    const auto cleared_connection = store.storeCleared.connect([&]() { cleared = true; });
    static_cast<void>(added_connection);
    static_cast<void>(removed_connection);
    static_cast<void>(cleared_connection);

    store.setMesh(5, makeTriangleMesh(5));
    CHECK(added_id == 5);

    store.removeMesh(5);
    CHECK(removed_id == 5);

    store.setMesh(1, makeTriangleMesh(1));
    store.clear();
    CHECK(cleared);
}

TEST_CASE("MeshStore: version increments on setMesh") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    REQUIRE(store.find(1) != nullptr);
    const auto v1 = store.find(1)->version;

    store.setMesh(1, makeTriangleMesh(1));
    REQUIRE(store.find(1) != nullptr);
    const auto v2 = store.find(1)->version;
    CHECK(v2 > v1);
}

TEST_CASE("ClearMeshAction clears one mesh or all meshes") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    store.setMesh(2, makeTriangleMesh(2));

    OpenGeoLab::Mesh::ClearMeshAction action(store);

    const auto single_result =
        action.execute({{"shapeId", 1U}}, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(single_result["ok"] == true);
    CHECK(single_result["action"] == "clear_mesh");
    CHECK(single_result["cleared"] == 1);
    CHECK(store.find(1) == nullptr);
    CHECK(store.find(2) != nullptr);

    const auto all_result = action.execute({}, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(all_result["ok"] == true);
    CHECK(all_result["action"] == "clear_mesh");
    CHECK(all_result["cleared"] == 1);
    CHECK(store.empty());
}

TEST_CASE("QueryMeshInfoAction returns node, edge and element information") {
    MeshStore store;
    store.setMesh(7, makeTriangleMesh(7));

    OpenGeoLab::Mesh::QueryMeshInfoAction action(store);
    const auto result =
        action.execute({{"entities",
                         {{{"shapeId", 7U}, {"type", "MeshNode"}, {"localId", 2U}},
                          {{"shapeId", 7U}, {"type", "MeshEdge"}, {"localId", 1U}},
                          {{"shapeId", 7U}, {"type", "MeshElement"}, {"localId", 1U}}}}},
                       OpenGeoLab::Core::NO_PROGRESS_CALLBACK);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "query_mesh_info");
    REQUIRE(result["entities"].is_array());
    REQUIRE(result["entities"].size() == 3);
    CHECK(result["entities"][0]["position"][0] == doctest::Approx(1.0));
    CHECK(result["entities"][1]["type"] == "MeshEdge");
    CHECK(result["entities"][2]["elementType"] == "Triangle");
    CHECK(result["entities"][2]["nodeLocalIds"] == nlohmann::json::array({1, 2, 3}));
}

#ifdef OPENGEOLAB_USE_GMSH
TEST_CASE("GenerateMeshAction generates mesh for a face entity") {
    OpenGeoLab::Mesh::MeshStore mesh_store;
    OpenGeoLab::Geometry::ShapeStore shape_store;
    const auto shape_id = shape_store.add("box", BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape());

    OpenGeoLab::Mesh::GenerateMeshAction action(mesh_store, shape_store);
    const auto result = action.execute(
        {{"entities", {{{"shapeId", shape_id}, {"type", "GeoFace"}, {"localId", 1U}}}},
         {"elementSize", 0.5},
         {"dimension", 2},
         {"elementType", "triangle"},
         {"algorithm", "delaunay"}},
        OpenGeoLab::Core::NO_PROGRESS_CALLBACK);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "generate_mesh");
    REQUIRE(result["results"].is_array());
    REQUIRE(result["results"].size() == 1);
    CHECK(result["results"][0]["shapeId"] == shape_id);
    CHECK(result["results"][0]["nodeCount"].get<std::size_t>() > 0);
    CHECK(result["results"][0]["elementCount"].get<std::size_t>() > 0);

    const auto* entry = mesh_store.find(shape_id);
    REQUIRE(entry != nullptr);
    CHECK_FALSE(entry->nodes.empty());
    CHECK_FALSE(entry->elements.empty());
}
#endif

TEST_CASE("MeshModule registers mesh actions and generate_mesh after bridge init") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Mesh::MeshModule module(factory);
    auto desc = module.describe();
    REQUIRE(desc["actions"].is_array());
    CHECK(desc["actions"].size() == 2);

    OpenGeoLab::Scene::SceneGraph scene;
    OpenGeoLab::Geometry::ShapeStore shape_store;
    module.initBridge(scene, shape_store);

    desc = module.describe();
#ifdef OPENGEOLAB_USE_GMSH
    CHECK(desc["actions"].size() == 3);
#else
    CHECK(desc["actions"].size() == 2);
#endif
}

TEST_CASE("MeshStore: getTopology returns cached topology after setMesh") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));

    const auto* topo = store.getTopology(1);
    REQUIRE(topo != nullptr);
    CHECK(topo->edges.size() == 3);
}

TEST_CASE("MeshStore: getTopology returns nullptr for missing shapeId") {
    MeshStore store;
    CHECK(store.getTopology(42) == nullptr);
}

TEST_CASE("MeshStore: topology cleared on removeMesh") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    REQUIRE(store.getTopology(1) != nullptr);

    store.removeMesh(1);
    CHECK(store.getTopology(1) == nullptr);
}

TEST_CASE("MeshStore: topology cleared on clear()") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    REQUIRE(store.getTopology(1) != nullptr);

    store.clear();
    CHECK(store.getTopology(1) == nullptr);
}

TEST_CASE("MeshStore: modifyMesh applies modifier and emits meshModified") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));

    uint32_t modified_id = 0;
    const auto conn = store.meshModified.connect([&](uint32_t id) { modified_id = id; });
    static_cast<void>(conn);

    store.modifyMesh(1,
                     [](MeshEntry& entry) { entry.nodes.push_back(MeshNode{{2.0F, 0.0F, 0.0F}}); });

    CHECK(modified_id == 1);

    const auto* entry = store.find(1);
    REQUIRE(entry != nullptr);
    CHECK(entry->nodes.size() == 4);
}

TEST_CASE("MeshStore: modifyMesh increments version") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));
    const auto v1 = store.find(1)->version;

    store.modifyMesh(1,
                     [](MeshEntry& entry) { entry.nodes.push_back(MeshNode{{2.0F, 0.0F, 0.0F}}); });
    const auto v2 = store.find(1)->version;
    CHECK(v2 > v1);
}

TEST_CASE("MeshStore: modifyMesh rebuilds topology") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));

    const auto* topo_before = store.getTopology(1);
    REQUIRE(topo_before != nullptr);
    CHECK(topo_before->edges.size() == 3);

    store.modifyMesh(1, [](MeshEntry& entry) {
        entry.nodes.push_back(MeshNode{{1.5F, 1.0F, 0.0F}});
        MeshElement tri{};
        tri.type = MeshElementType::Triangle;
        tri.nodeLocalIds = {2, 4, 3, 0, 0, 0, 0, 0};
        entry.elements.push_back(tri);
    });

    const auto* topo_after = store.getTopology(1);
    REQUIRE(topo_after != nullptr);
    CHECK(topo_after->edges.size() == 5);
}

TEST_CASE("MeshStore: modifyMesh on non-existent shapeId is no-op") {
    MeshStore store;
    uint32_t modified_id = 0;
    const auto conn = store.meshModified.connect([&](uint32_t id) { modified_id = id; });
    static_cast<void>(conn);

    store.modifyMesh(99, [](MeshEntry&) {});
    CHECK(modified_id == 0);
}

TEST_CASE("MeshStore: modifyMesh leaves mesh and topology unchanged if modifier throws") {
    MeshStore store;
    store.setMesh(1, makeTriangleMesh(1));

    uint32_t modified_id = 0;
    const auto conn = store.meshModified.connect([&](uint32_t id) { modified_id = id; });
    static_cast<void>(conn);

    const auto version_before = store.find(1)->version;
    REQUIRE(store.getTopology(1) != nullptr);
    const auto edge_count_before = store.getTopology(1)->edges.size();

    CHECK_THROWS_AS(store.modifyMesh(1,
                                     [](MeshEntry& entry) {
                                         entry.nodes.push_back(MeshNode{{2.0F, 0.0F, 0.0F}});
                                         throw std::runtime_error("modifier failed");
                                     }),
                    std::runtime_error);

    CHECK(modified_id == 0);

    const auto* entry = store.find(1);
    REQUIRE(entry != nullptr);
    CHECK(entry->nodes.size() == 3);
    CHECK(entry->version == version_before);

    const auto* topology = store.getTopology(1);
    REQUIRE(topology != nullptr);
    CHECK(topology->edges.size() == edge_count_before);
}
