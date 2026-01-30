/**
 * @file tessellation_test.cpp
 * @brief Unit tests for TessellationService
 */

#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/shape_builder.hpp"
#include "render/render_data.hpp"
#include "render/tessellation_service.hpp"


#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

#include <catch2/catch_test_macros.hpp>

using namespace OpenGeoLab; // NOLINT

namespace {

TopoDS_Shape createTestBox(double dx = 10.0, double dy = 10.0, double dz = 10.0) {
    BRepPrimAPI_MakeBox maker(dx, dy, dz);
    maker.Build();
    return maker.Shape();
}

TopoDS_Shape createTestSphere(double radius = 5.0) {
    BRepPrimAPI_MakeSphere maker(radius);
    maker.Build();
    return maker.Shape();
}

// =============================================================================
// TessellationService Tests
// =============================================================================

TEST_CASE("TessellationService - Tessellate part", "[render][tessellation]") {
    auto doc = Geometry::GeometryDocument::create();
    auto shape = createTestBox();

    Geometry::ShapeBuilder builder(doc);
    auto result = builder.buildFromShape(shape, "TestBox");
    REQUIRE(result.m_success);

    Render::TessellationService tess_service;
    auto params = Render::TessellationParams::mediumQuality();

    auto part_render_data = tess_service.tessellatePart(result.m_rootPart, 0, params);

    REQUIRE(part_render_data != nullptr);
    CHECK(part_render_data->m_partEntityId == result.m_rootPart->entityId());
    CHECK(part_render_data->m_partName == "TestBox");

    // Box should have 6 faces tessellated
    CHECK(part_render_data->m_faces.size() == 6);

    // Each face should have triangles
    for(const auto& face : part_render_data->m_faces) {
        CHECK(face.triangleCount() > 0);
        CHECK(face.vertexCount() > 0);
    }

    // Should have edges for wireframe
    CHECK(part_render_data->m_edges.size() > 0);
}

TEST_CASE("TessellationService - Tessellate document", "[render][tessellation]") {
    auto doc = Geometry::GeometryDocument::create();

    // Add two parts
    auto box_shape = createTestBox();
    auto sphere_shape = createTestSphere();

    Geometry::ShapeBuilder builder(doc);
    auto box_result = builder.buildFromShape(box_shape, "Box");
    REQUIRE(box_result.m_success);

    auto sphere_result = builder.buildFromShape(sphere_shape, "Sphere");
    REQUIRE(sphere_result.m_success);

    Render::TessellationService tess_service;
    auto doc_render_data = tess_service.tessellateDocument(doc);

    REQUIRE(doc_render_data != nullptr);
    CHECK(doc_render_data->partCount() == 2);
    CHECK(doc_render_data->totalTriangleCount() > 0);
}

TEST_CASE("TessellationService - Tessellation quality levels", "[render][tessellation]") {
    auto doc = Geometry::GeometryDocument::create();
    auto shape = createTestSphere(10.0);

    Geometry::ShapeBuilder builder(doc);
    auto result = builder.buildFromShape(shape, "Sphere");
    REQUIRE(result.m_success);

    Render::TessellationService tess_service;

    // Low quality should produce fewer triangles
    auto low_params = Render::TessellationParams::lowQuality();
    auto low_data = tess_service.tessellatePart(result.m_rootPart, 0, low_params);
    REQUIRE(low_data != nullptr);
    size_t low_triangles = low_data->totalTriangleCount();

    // High quality should produce more triangles
    auto high_params = Render::TessellationParams::highQuality();
    auto high_data = tess_service.tessellatePart(result.m_rootPart, 0, high_params);
    REQUIRE(high_data != nullptr);
    size_t high_triangles = high_data->totalTriangleCount();

    CHECK(high_triangles > low_triangles);
}

// =============================================================================
// RenderData Tests
// =============================================================================

TEST_CASE("RenderColor - Color generation", "[render][color]") {
    SECTION("From HSV") {
        auto red = Render::RenderColor::fromHSV(0.0f, 1.0f, 1.0f);
        CHECK(red.m_r > 0.9f);
        CHECK(red.m_g < 0.1f);
        CHECK(red.m_b < 0.1f);

        auto green = Render::RenderColor::fromHSV(120.0f, 1.0f, 1.0f);
        CHECK(green.m_r < 0.1f);
        CHECK(green.m_g > 0.9f);
        CHECK(green.m_b < 0.1f);
    }

    SECTION("From index - distinct colors") {
        auto color0 = Render::RenderColor::fromIndex(0);
        auto color1 = Render::RenderColor::fromIndex(1);
        auto color2 = Render::RenderColor::fromIndex(2);

        // Colors should be different
        CHECK((color0.m_r != color1.m_r || color0.m_g != color1.m_g || color0.m_b != color1.m_b));
        CHECK((color1.m_r != color2.m_r || color1.m_g != color2.m_g || color1.m_b != color2.m_b));
    }
}

TEST_CASE("PartRenderData - Merge to buffers", "[render][data]") {
    Render::PartRenderData part_data;

    // Create two simple faces with triangles
    Render::RenderFace face1;
    face1.m_vertices = {Render::RenderVertex(0, 0, 0, 0, 0, 1),
                        Render::RenderVertex(1, 0, 0, 0, 0, 1),
                        Render::RenderVertex(0, 1, 0, 0, 0, 1)};
    face1.m_indices = {0, 1, 2};

    Render::RenderFace face2;
    face2.m_vertices = {Render::RenderVertex(0, 0, 1, 0, 0, 1),
                        Render::RenderVertex(1, 0, 1, 0, 0, 1),
                        Render::RenderVertex(0, 1, 1, 0, 0, 1)};
    face2.m_indices = {0, 1, 2};

    part_data.m_faces = {face1, face2};

    std::vector<Render::RenderVertex> vertices;
    std::vector<uint32_t> indices;
    part_data.mergeToBuffers(vertices, indices);

    CHECK(vertices.size() == 6); // 3 + 3
    CHECK(indices.size() == 6);  // 3 + 3

    // Second face indices should be offset
    CHECK(indices[3] == 3);
    CHECK(indices[4] == 4);
    CHECK(indices[5] == 5);
}

} // anonymous namespace