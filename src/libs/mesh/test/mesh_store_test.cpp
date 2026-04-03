/**
 * @file mesh_store_test.cpp
 * @brief MeshStore unit tests
 */

#include <opengeolab/mesh/mesh_store.hpp>

#include <doctest/doctest.h>

#include <algorithm>

using OpenGeoLab::Mesh::MeshElement;
using OpenGeoLab::Mesh::MeshElementType;
using OpenGeoLab::Mesh::MeshEntry;
using OpenGeoLab::Mesh::MeshNode;
using OpenGeoLab::Mesh::MeshStore;

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
