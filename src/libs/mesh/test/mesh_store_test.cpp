/// @file mesh_store_test.cpp
/// @brief Unit tests for MeshStore — add, remove, find, signals.

#include <doctest/doctest.h>

#include <opengeolab/mesh/mesh_store.hpp>

using OpenGeoLab::Mesh::ElementBlock;
using OpenGeoLab::Mesh::ElementType;
using OpenGeoLab::Mesh::MeshEntry;
using OpenGeoLab::Mesh::MeshStore;

/// Helper: create a minimal mesh entry with a single triangle.
static MeshEntry makeSimpleEntry(const std::string& name = "test-mesh",
                                 std::optional<uint32_t> shape_id = std::nullopt) {
    MeshEntry entry;
    entry.name = name;
    entry.sourceShapeId = shape_id;
    entry.nodes.coords = {0, 0, 0, 1, 0, 0, 0, 1, 0};

    ElementBlock block;
    block.type = ElementType::Triangle3;
    block.connectivity = {1, 2, 3};
    entry.surfaceBlocks.push_back(std::move(block));

    entry.elementLocator.build(entry.lineBlocks, entry.surfaceBlocks, entry.volumeBlocks);
    return entry;
}

// ---------------------------------------------------------------------------
// add / find
// ---------------------------------------------------------------------------

TEST_CASE("MeshStore add returns incrementing IDs starting from 1") {
    MeshStore store;
    auto id1 = store.add(makeSimpleEntry("m1"));
    auto id2 = store.add(makeSimpleEntry("m2"));
    CHECK(id1 == 1);
    CHECK(id2 == 2);
}

TEST_CASE("MeshStore find returns entry after add") {
    MeshStore store;
    auto id = store.add(makeSimpleEntry("TestMesh"));
    const auto entry = store.find(id);
    REQUIRE(entry.get() != nullptr);
    CHECK(entry->id == id);
    CHECK(entry->name == "TestMesh");
    CHECK(entry->nodeCount() == 3);
    CHECK(entry->elementCount() == 1);
}

TEST_CASE("MeshStore find returns nullptr for unknown id") {
    MeshStore const store;
    CHECK(store.find(999).get() == nullptr);
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------

TEST_CASE("MeshStore remove makes find return nullptr") {
    MeshStore store;
    auto id = store.add(makeSimpleEntry());
    REQUIRE(store.find(id).get() != nullptr);
    store.remove(id);
    CHECK(store.find(id).get() == nullptr);
}

// ---------------------------------------------------------------------------
// allMeshIds
// ---------------------------------------------------------------------------

TEST_CASE("MeshStore allMeshIds returns valid IDs") {
    MeshStore store;
    auto id1 = store.add(makeSimpleEntry("a"));
    auto id2 = store.add(makeSimpleEntry("b"));
    store.remove(id1);

    auto ids = store.allMeshIds();
    CHECK(ids.size() == 1);
    CHECK(ids[0] == id2);
}

// ---------------------------------------------------------------------------
// findByShapeId
// ---------------------------------------------------------------------------

TEST_CASE("MeshStore findByShapeId") {
    MeshStore store;
    store.add(makeSimpleEntry("m1", 10));
    store.add(makeSimpleEntry("m2", 20));
    store.add(makeSimpleEntry("m3", 10));

    auto result = store.findByShapeId(10);
    CHECK(result.size() == 2);

    auto empty = store.findByShapeId(999);
    CHECK(empty.empty());
}

// ---------------------------------------------------------------------------
// Signals
// ---------------------------------------------------------------------------

TEST_CASE("MeshStore meshAdded signal fires on add") {
    MeshStore store;
    uint32_t signal_id = 0;
    std::string signal_name;

    auto conn = store.meshAdded.connect([&](uint32_t id, const MeshEntry& entry) {
        signal_id = id;
        signal_name = entry.name;
    });

    auto id = store.add(makeSimpleEntry("signal-test"));
    CHECK(signal_id == id);
    CHECK(signal_name == "signal-test");
}

TEST_CASE("MeshStore meshRemoved signal fires on remove") {
    MeshStore store;
    uint32_t removed_id = 0;

    auto conn = store.meshRemoved.connect([&](uint32_t id) { removed_id = id; });

    auto id = store.add(makeSimpleEntry());
    store.remove(id);
    CHECK(removed_id == id);
}
