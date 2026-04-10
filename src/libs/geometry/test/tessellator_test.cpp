/**
 * @file tessellator_test.cpp
 * @brief Unit tests for the tessellator free function and calculateDeflection
 */

#include <opengeolab/geometry/tessellator.hpp>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <gp_Pnt.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Geometry::calculateDeflection;
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

// ── calculateDeflection tests ───────────────────────────────────

TEST_CASE("calculateDeflection returns positive value for unit box") {
    const auto shape = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape();
    const double defl = calculateDeflection(shape);

    CHECK(defl > 0.0);
    CHECK(defl >= Precision::Confusion() * 1.5); // above safety floor
}

TEST_CASE("calculateDeflection scales with bounding-box size") {
    const auto small_box = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape();
    const auto large_box = BRepPrimAPI_MakeBox(100.0, 100.0, 100.0).Shape();

    const double defl_small = calculateDeflection(small_box);
    const double defl_large = calculateDeflection(large_box);

    // Larger shape should yield a larger (or equal via clamping) deflection
    CHECK(defl_large >= defl_small);
}

TEST_CASE("calculateDeflection respects tessRatio") {
    const auto shape = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();

    const double defl_default = calculateDeflection(shape, 1.0);
    const double defl_fine = calculateDeflection(shape, 0.5);

    // Smaller ratio should produce equal or smaller deflection (finer mesh)
    CHECK(defl_fine <= defl_default);
}

TEST_CASE("calculateDeflection for wire shape is finer than solid") {
    // Build a simple wire
    const auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(10, 0, 0)).Edge();
    const auto wire = BRepBuilderAPI_MakeWire(edge).Wire();

    const auto box = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();

    const double defl_wire = calculateDeflection(wire);
    const double defl_box = calculateDeflection(box);

    // Wire deflection should be capped smaller for finer edge display
    CHECK(defl_wire <= defl_box);
}

TEST_CASE("calculateDeflection never returns below safety floor") {
    // Small but valid shape (above OCCT precision threshold)
    const auto small = BRepPrimAPI_MakeBox(1e-4, 1e-4, 1e-4).Shape();
    const double defl = calculateDeflection(small);

    CHECK(defl >= std::max(1.0e-7, Precision::Confusion() * 1.5));
}

TEST_CASE("Tessellate with auto deflection (linearDeflection=0) produces valid data") {
    auto entry = makeEntry(BRepPrimAPI_MakeBox(5.0, 5.0, 5.0).Shape());

    // Default TessellationParams has linearDeflection = 0 (auto)
    auto result = tessellate(entry);

    CHECK(result.visualData.surfaces.size() == 6);
    CHECK_FALSE(result.triangleTags.empty());
    CHECK(result.visualData.edges.size() == 12);
}
