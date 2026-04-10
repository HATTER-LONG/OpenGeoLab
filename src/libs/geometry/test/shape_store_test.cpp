/**
 * @file shape_store_test.cpp
 * @brief Unit tests for ShapeStore — add, remove, find, sub-shape indexing, signals
 */

#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pnt.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Geometry::ShapeStore;

/** @brief Helper: create a unit box shape. */
static TopoDS_Shape makeBox(double w = 1.0, double h = 1.0, double d = 1.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

TEST_CASE("ShapeStore add returns incrementing ids") {
    ShapeStore store;
    auto id0 = store.add("Box0", makeBox());
    auto id1 = store.add("Box1", makeBox());
    CHECK(id0 == 0);
    CHECK(id1 == 1);
    CHECK(store.size() == 2);
}

TEST_CASE("ShapeStore find returns entry after add") {
    ShapeStore store;
    auto id = store.add("TestBox", makeBox(2, 3, 4));
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->id == id);
    CHECK(entry->name == "TestBox");
    CHECK_FALSE(entry->shape.IsNull());
}

TEST_CASE("ShapeStore find returns nullptr for unknown id") {
    const ShapeStore store;
    CHECK(store.find(999) == nullptr);
}

TEST_CASE("ShapeStore remove makes find return nullptr") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    REQUIRE(store.find(id) != nullptr);
    store.remove(id);
    CHECK(store.find(id) == nullptr);
    CHECK(store.size() == 0);
}

TEST_CASE("ShapeStore remove reuses freed id") {
    ShapeStore store;
    auto id0 = store.add("Box0", makeBox());
    store.add("Box1", makeBox());
    store.remove(id0);

    // Next add should reuse id0
    auto id2 = store.add("Box2", makeBox());
    CHECK(id2 == id0);
    CHECK(store.size() == 2);
}

TEST_CASE("ShapeStore allShapeIds returns active ids") {
    ShapeStore store;
    store.add("A", makeBox());
    auto id1 = store.add("B", makeBox());
    store.add("C", makeBox());
    store.remove(id1);

    auto ids = store.allShapeIds();
    CHECK(ids.size() == 2);
    CHECK(std::find(ids.begin(), ids.end(), 0) != ids.end());
    CHECK(std::find(ids.begin(), ids.end(), 2) != ids.end());
    CHECK(std::find(ids.begin(), ids.end(), 1) == ids.end());
}

TEST_CASE("ShapeStore builds sub-shape index on add") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    // A box has: 8 vertices, 12 edges, 6 wires, 6 faces, 1 solid
    CHECK(entry->vertexMap.Extent() == 8);
    CHECK(entry->edgeMap.Extent() == 12);
    CHECK(entry->wireMap.Extent() == 6);
    CHECK(entry->faceMap.Extent() == 6);
    CHECK(entry->solidMap.Extent() == 1);
}

TEST_CASE("ShapeStore subShape returns valid sub-shapes") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());

    // Retrieve first face
    auto face = store.subShape(id, EntityType::GeoFace, 1);
    CHECK_FALSE(face.IsNull());
    CHECK(face.ShapeType() == TopAbs_FACE);

    // Retrieve first vertex
    auto vertex = store.subShape(id, EntityType::GeoVertex, 1);
    CHECK_FALSE(vertex.IsNull());
    CHECK(vertex.ShapeType() == TopAbs_VERTEX);

    // Out of range returns null
    auto bad = store.subShape(id, EntityType::GeoFace, 999);
    CHECK(bad.IsNull());

    // Unknown shape id returns null
    auto unknown = store.subShape(999, EntityType::GeoFace, 1);
    CHECK(unknown.IsNull());
}

TEST_CASE("ShapeStore emits shapeAdded signal on add") {
    ShapeStore store;

    uint32_t signal_id = UINT32_MAX;
    std::string signal_name;
    auto conn =
        store.shapeAdded.connect([&](uint32_t id, const OpenGeoLab::Geometry::ShapeEntry& entry) {
            signal_id = id;
            signal_name = entry.name;
        });

    auto id = store.add("SignalBox", makeBox());
    CHECK(signal_id == id);
    CHECK(signal_name == "SignalBox");
}

TEST_CASE("ShapeStore emits shapeRemoved signal on remove") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());

    uint32_t removed_id = UINT32_MAX;
    auto conn = store.shapeRemoved.connect([&](uint32_t rid) { removed_id = rid; });

    store.remove(id);
    CHECK(removed_id == id);
}

TEST_CASE("ShapeStore tessellate populates visualData") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());

    // Before tessellation: no visual data
    CHECK(store.find(id)->visualData == nullptr);

    store.tessellate(id);

    const auto* entry = store.find(id);
    REQUIRE(entry->visualData != nullptr);
    CHECK_FALSE(entry->visualData->surfaces.empty());
    CHECK_FALSE(entry->visualData->edges.empty());
    CHECK_FALSE(entry->visualData->points.empty());
    CHECK_FALSE(entry->triangleTags.empty());
    CHECK_FALSE(entry->edgeTags.empty());
    CHECK_FALSE(entry->vertexTags.empty());
}

TEST_CASE("ShapeStore tessellate emits shapeUpdated signal") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());

    uint32_t updated_id = UINT32_MAX;
    auto conn = store.shapeUpdated.connect(
        [&](uint32_t uid, const OpenGeoLab::Geometry::ShapeEntry&) { updated_id = uid; });

    store.tessellate(id);
    CHECK(updated_id == id);
}

TEST_CASE("ShapeStore tessellate throws for unknown shapeId") {
    ShapeStore store;
    CHECK_THROWS_AS(store.tessellate(999), std::invalid_argument);
}

TEST_CASE("ShapeStore replaceShape updates shape and rebuilds sub-shape index") {
    ShapeStore store;
    auto id = store.add("Box6", makeBox(1, 1, 1));

    // Original box: 6 faces, 1 solid
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->faceMap.Extent() == 6);
    CHECK(entry->solidMap.Extent() == 1);

    // Replace with a compound of two boxes
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    builder.Add(compound, BRepPrimAPI_MakeBox(1, 1, 1).Shape());
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(5, 0, 0), 2, 2, 2).Shape());
    store.replaceShape(id, compound);

    entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->name == "Box6");
    CHECK(entry->solidMap.Extent() == 2);
    CHECK(entry->faceMap.Extent() == 12);
}

TEST_CASE("ShapeStore replaceShape clears tessellation cache") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    store.tessellate(id);
    REQUIRE(store.find(id)->visualData != nullptr);

    store.replaceShape(id, makeBox(2, 2, 2));
    CHECK(store.find(id)->visualData == nullptr);
    CHECK(store.find(id)->triangleTags.empty());
    CHECK(store.find(id)->edgeTags.empty());
    CHECK(store.find(id)->vertexTags.empty());
}

TEST_CASE("ShapeStore replaceShape emits shapeUpdated signal") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());

    uint32_t updated_id = UINT32_MAX;
    int solid_count_at_emit = -1;
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    builder.Add(compound, BRepPrimAPI_MakeBox(1, 1, 1).Shape());
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(5, 0, 0), 2, 2, 2).Shape());
    auto conn = store.shapeUpdated.connect(
        [&](uint32_t uid, const OpenGeoLab::Geometry::ShapeEntry& entry) {
            updated_id = uid;
            solid_count_at_emit = entry.solidMap.Extent();
        });

    store.replaceShape(id, compound);
    CHECK(updated_id == id);
    CHECK(solid_count_at_emit == 2);
}

TEST_CASE("ShapeStore replaceShape throws for unknown shapeId") {
    ShapeStore store;
    CHECK_THROWS_AS(store.replaceShape(999, makeBox()), std::invalid_argument);
}
