/**
 * @file tessellator_test.cpp
 * @brief Unit tests for Tessellator
 */

#include "geometry/tessellator.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <catch2/catch_test_macros.hpp>

namespace {

using namespace OpenGeoLab::Geometry;

TEST_CASE("Tessellator - tessellate box shape") {
    BRepPrimAPI_MakeBox box_maker(10.0, 10.0, 10.0);
    box_maker.Build();
    REQUIRE(box_maker.IsDone());

    auto render_data = Tessellator::tessellateShape(box_maker.Shape());

    // Should have triangles
    REQUIRE_FALSE(render_data.m_triangleMesh.isEmpty());
    REQUIRE(render_data.m_triangleMesh.triangleCount() > 0);

    // A box has 6 faces, each with at least 2 triangles
    REQUIRE(render_data.m_triangleMesh.triangleCount() >= 12);

    // Should have edges
    REQUIRE_FALSE(render_data.m_edgeMesh.isEmpty());
    REQUIRE(render_data.m_edgeMesh.lineCount() > 0);

    // Vertex count should match normals
    REQUIRE(render_data.m_triangleMesh.m_vertices.size() ==
            render_data.m_triangleMesh.m_normals.size());
}

TEST_CASE("Tessellator - tessellate sphere shape") {
    BRepPrimAPI_MakeSphere sphere_maker(5.0);
    sphere_maker.Build();
    REQUIRE(sphere_maker.IsDone());

    auto params = TessellationParams::defaultQuality();
    auto render_data = Tessellator::tessellateShape(sphere_maker.Shape(), params);

    REQUIRE_FALSE(render_data.m_triangleMesh.isEmpty());
    REQUIRE(render_data.m_triangleMesh.triangleCount() > 0);

    // Sphere should have many triangles due to curvature
    REQUIRE(render_data.m_triangleMesh.triangleCount() > 100);
}

TEST_CASE("Tessellator - high quality produces more triangles") {
    BRepPrimAPI_MakeSphere sphere_maker(5.0);
    sphere_maker.Build();

    auto low_quality =
        Tessellator::tessellateShape(sphere_maker.Shape(), TessellationParams::lowQuality());
    auto high_quality =
        Tessellator::tessellateShape(sphere_maker.Shape(), TessellationParams::highQuality());

    // High quality should produce more triangles
    REQUIRE(high_quality.m_triangleMesh.triangleCount() >
            low_quality.m_triangleMesh.triangleCount());
}

TEST_CASE("Tessellator - null shape returns empty") {
    TopoDS_Shape null_shape;
    auto render_data = Tessellator::tessellateShape(null_shape);

    REQUIRE(render_data.isEmpty());
}

TEST_CASE("Tessellator - extract edges only") {
    BRepPrimAPI_MakeBox box_maker(10.0, 10.0, 10.0);
    box_maker.Build();

    auto edge_mesh = Tessellator::extractEdges(box_maker.Shape());

    // A box has 12 edges
    REQUIRE_FALSE(edge_mesh.isEmpty());
    REQUIRE(edge_mesh.vertexCount() > 0);
    REQUIRE(edge_mesh.lineCount() >= 12);
}

TEST_CASE("Tessellator - generate part color") {
    // Different IDs should generate different colors
    auto color1 = Tessellator::generatePartColor(1);
    auto color2 = Tessellator::generatePartColor(2);
    auto color3 = Tessellator::generatePartColor(100);

    // Colors should be different
    bool all_same =
        (color1.m_r == color2.m_r && color1.m_g == color2.m_g && color1.m_b == color2.m_b);
    REQUIRE_FALSE(all_same);

    // Same ID should generate same color
    auto color1_again = Tessellator::generatePartColor(1);
    REQUIRE(color1.m_r == color1_again.m_r);
    REQUIRE(color1.m_g == color1_again.m_g);
    REQUIRE(color1.m_b == color1_again.m_b);

    // Colors should be valid (in range)
    REQUIRE(color3.m_r >= 0.0f);
    REQUIRE(color3.m_r <= 1.0f);
    REQUIRE(color3.m_g >= 0.0f);
    REQUIRE(color3.m_g <= 1.0f);
    REQUIRE(color3.m_b >= 0.0f);
    REQUIRE(color3.m_b <= 1.0f);
}

TEST_CASE("TriangleMesh - merge operation") {
    TriangleMesh mesh1;
    mesh1.m_vertices = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    mesh1.m_normals = {0, 0, 1, 0, 0, 1, 0, 0, 1};
    mesh1.m_indices = {0, 1, 2};

    TriangleMesh mesh2;
    mesh2.m_vertices = {2, 0, 0, 3, 0, 0, 2, 1, 0};
    mesh2.m_normals = {0, 0, 1, 0, 0, 1, 0, 0, 1};
    mesh2.m_indices = {0, 1, 2};

    mesh1.merge(mesh2);

    REQUIRE(mesh1.vertexCount() == 6);
    REQUIRE(mesh1.triangleCount() == 2);

    // Second triangle indices should be offset
    REQUIRE(mesh1.m_indices[3] == 3);
    REQUIRE(mesh1.m_indices[4] == 4);
    REQUIRE(mesh1.m_indices[5] == 5);
}

TEST_CASE("EdgeMesh - merge operation") {
    EdgeMesh mesh1;
    mesh1.m_vertices = {0, 0, 0, 1, 0, 0};
    mesh1.m_indices = {0, 1};

    EdgeMesh mesh2;
    mesh2.m_vertices = {2, 0, 0, 3, 0, 0};
    mesh2.m_indices = {0, 1};

    mesh1.merge(mesh2);

    REQUIRE(mesh1.vertexCount() == 4);
    REQUIRE(mesh1.lineCount() == 2);
    REQUIRE(mesh1.m_indices[2] == 2);
    REQUIRE(mesh1.m_indices[3] == 3);
}

TEST_CASE("Color4f - predefined colors") {
    auto red = Color4f::red();
    REQUIRE(red.m_r == 1.0f);
    REQUIRE(red.m_g == 0.0f);
    REQUIRE(red.m_b == 0.0f);
    REQUIRE(red.m_a == 1.0f);

    auto green = Color4f::green();
    REQUIRE(green.m_r == 0.0f);
    REQUIRE(green.m_g == 1.0f);
    REQUIRE(green.m_b == 0.0f);

    auto blue = Color4f::blue();
    REQUIRE(blue.m_r == 0.0f);
    REQUIRE(blue.m_g == 0.0f);
    REQUIRE(blue.m_b == 1.0f);
}

} // namespace
