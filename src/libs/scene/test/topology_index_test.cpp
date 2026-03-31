/**
 * @file topology_index_test.cpp
 * @brief Unit tests for TopologyIndex
 */

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

#include <doctest/doctest.h>

#include <cstdint>

namespace OpenGeoLab::Scene::Tests {

namespace {

struct BoxTopologyFixture {
    Geometry::ShapeStore store;
    TopologyIndex index;
    uint32_t shapeId{0};
    const Geometry::ShapeEntry* entry{nullptr};

    BoxTopologyFixture() {
        BRepPrimAPI_MakeBox box_maker(10.0, 20.0, 30.0);
        shapeId = store.add("TestBox", box_maker.Shape());
        store.tessellate(shapeId);
        entry = store.find(shapeId);
        REQUIRE(entry != nullptr);
        index.buildForShape(shapeId, *entry);
    }
};

} // namespace

TEST_CASE("TopologyIndex builds forward lookups for a box") {
    const BoxTopologyFixture fixture;

    for(uint32_t edge_local_id = 1;
        edge_local_id <= static_cast<uint32_t>(fixture.entry->edgeMap.Extent()); ++edge_local_id) {
        CHECK(fixture.index.edgeToWire(fixture.shapeId, edge_local_id).has_value());
    }

    for(uint32_t wire_local_id = 1;
        wire_local_id <= static_cast<uint32_t>(fixture.entry->wireMap.Extent()); ++wire_local_id) {
        CHECK(fixture.index.wireToFace(fixture.shapeId, wire_local_id).has_value());
    }

    for(uint32_t face_local_id = 1;
        face_local_id <= static_cast<uint32_t>(fixture.entry->faceMap.Extent()); ++face_local_id) {
        CHECK(fixture.index.faceToSolid(fixture.shapeId, face_local_id).has_value());
    }
}

TEST_CASE("TopologyIndex exposes non-empty edge lists for each wire") {
    const BoxTopologyFixture fixture;

    for(uint32_t wire_local_id = 1;
        wire_local_id <= static_cast<uint32_t>(fixture.entry->wireMap.Extent()); ++wire_local_id) {
        const auto edge_ids = fixture.index.wireEdges(fixture.shapeId, wire_local_id);
        CHECK_FALSE(edge_ids.empty());
    }
}

TEST_CASE("TopologyIndex returns all six box faces for its solid") {
    const BoxTopologyFixture fixture;

    REQUIRE(fixture.entry->solidMap.Extent() == 1);
    const auto face_ids = fixture.index.solidFaces(fixture.shapeId, 1);

    CHECK(face_ids.size() == 6);
}

TEST_CASE("TopologyIndex removeShape clears all relations") {
    BoxTopologyFixture fixture;

    fixture.index.removeShape(fixture.shapeId);

    CHECK_FALSE(fixture.index.edgeToWire(fixture.shapeId, 1).has_value());
    CHECK_FALSE(fixture.index.wireToFace(fixture.shapeId, 1).has_value());
    CHECK_FALSE(fixture.index.faceToSolid(fixture.shapeId, 1).has_value());
    CHECK(fixture.index.wireEdges(fixture.shapeId, 1).empty());
    CHECK(fixture.index.solidFaces(fixture.shapeId, 1).empty());
}

TEST_CASE("TopologyIndex returns empty results for unknown shapes") {
    TopologyIndex index;

    CHECK_FALSE(index.edgeToWire(999, 1).has_value());
    CHECK_FALSE(index.wireToFace(999, 1).has_value());
    CHECK_FALSE(index.faceToSolid(999, 1).has_value());
    CHECK(index.wireEdges(999, 1).empty());
    CHECK(index.solidFaces(999, 1).empty());
}

} // namespace OpenGeoLab::Scene::Tests
