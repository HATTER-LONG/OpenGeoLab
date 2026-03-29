/**
 * @file tessellator_test.cpp
 * @brief Unit tests for the tessellator free function
 */

#include <opengeolab/geometry/tessellator.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TopExp.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Geometry::ShapeEntry;
using OpenGeoLab::Geometry::tessellate;

/** @brief Build a ShapeEntry with sub-shape maps from an OCC shape. */
static ShapeEntry makeEntry(const TopoDS_Shape& shape, const std::string& name = "test") {
    ShapeEntry entry;
    entry.id = 0;
    entry.name = name;
    entry.shape = shape;
    TopExp::MapShapes(shape, TopAbs_VERTEX, entry.vertexMap);
    TopExp::MapShapes(shape, TopAbs_EDGE, entry.edgeMap);
    TopExp::MapShapes(shape, TopAbs_WIRE, entry.wireMap);
    TopExp::MapShapes(shape, TopAbs_FACE, entry.faceMap);
    TopExp::MapShapes(shape, TopAbs_SOLID, entry.solidMap);
    return entry;
}

TEST_CASE("Tessellate box produces surfaces, edges, and points") {
    auto entry = makeEntry(BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape());
    auto result = tessellate(entry);

    // Surfaces: one per face; box has 6 faces
    CHECK(result.visualData.surfaces.size() == 6);
    for(const auto& surf : result.visualData.surfaces) {
        CHECK_FALSE(surf.positions.empty());
        CHECK_FALSE(surf.indices.empty());
        // Each position component is xyz
        CHECK(surf.positions.size() % 3 == 0);
        // Each index triple forms a triangle
        CHECK(surf.indices.size() % 3 == 0);
    }

    // Triangle tags: one per triangle, all GeoFace
    CHECK_FALSE(result.triangleTags.empty());
    for(const auto& tag : result.triangleTags) {
        CHECK(tag.type == EntityType::GeoFace);
        CHECK(tag.localId >= 1);
        CHECK(tag.localId <= 6);
    }

    // Edges: box has 12 edges
    CHECK(result.visualData.edges.size() == 12);
    for(const auto& edge : result.visualData.edges) {
        CHECK_FALSE(edge.positions.empty());
        CHECK_FALSE(edge.indices.empty());
    }

    // Edge tags: one per line segment
    CHECK_FALSE(result.edgeTags.empty());
    for(const auto& tag : result.edgeTags) {
        CHECK(tag.type == EntityType::GeoEdge);
        CHECK(tag.localId >= 1);
        CHECK(tag.localId <= 12);
    }

    // Points: 8 vertices
    REQUIRE(result.visualData.points.size() == 1);
    CHECK(result.visualData.points[0].positions.size() == 8 * 3);

    // Vertex tags: one per vertex
    CHECK(result.vertexTags.size() == 8);
    for(const auto& tag : result.vertexTags) {
        CHECK(tag.type == EntityType::GeoVertex);
        CHECK(tag.localId >= 1);
        CHECK(tag.localId <= 8);
    }
}

TEST_CASE("Tessellate sphere produces valid data") {
    auto entry = makeEntry(BRepPrimAPI_MakeSphere(1.0).Shape());
    auto result = tessellate(entry);

    // Sphere has at least 1 face
    CHECK(result.visualData.surfaces.size() >= 1);
    CHECK_FALSE(result.triangleTags.empty());

    // Total triangle count should be reasonable for a sphere
    std::size_t total_tri = 0;
    for(const auto& surf : result.visualData.surfaces) {
        total_tri += surf.indices.size() / 3;
    }
    CHECK(total_tri > 10); // At least some triangles

    // Edges should be present (sphere has edges at seams)
    CHECK(result.visualData.edges.size() >= 1);

    // Vertices should be present
    CHECK(result.visualData.points.size() >= 1);
}
