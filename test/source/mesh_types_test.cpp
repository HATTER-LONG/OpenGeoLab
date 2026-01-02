/**
 * @file mesh_types_test.cpp
 * @brief Unit tests for mesh data types
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "geometry/mesh_types.hpp"

using namespace OpenGeoLab::Mesh;
using Point3D = OpenGeoLab::Geometry::Point3D;
using Catch::Matchers::WithinRel;

TEST_CASE("MeshData - basic operations", "[mesh][data]") {
    MeshData mesh;

    SECTION("isEmpty returns true for empty mesh") { CHECK(mesh.isEmpty()); }

    SECTION("Adding nodes and elements") {
        MeshNode n1, n2, n3;
        n1.m_id = 1;
        n1.m_position = Point3D(0, 0, 0);
        n2.m_id = 2;
        n2.m_position = Point3D(1, 0, 0);
        n3.m_id = 3;
        n3.m_position = Point3D(0.5, 1, 0);

        mesh.m_nodes.push_back(n1);
        mesh.m_nodes.push_back(n2);
        mesh.m_nodes.push_back(n3);

        MeshElement elem;
        elem.m_id = 1;
        elem.m_type = ElementType::Triangle;
        elem.m_nodeIds = {1, 2, 3};

        mesh.m_elements.push_back(elem);

        CHECK_FALSE(mesh.isEmpty());
        CHECK(mesh.m_nodes.size() == 3);
        CHECK(mesh.m_elements.size() == 1);
    }

    SECTION("getNodeById") {
        MeshNode n1;
        n1.m_id = 42;
        n1.m_position = Point3D(1, 2, 3);
        mesh.m_nodes.push_back(n1);

        const MeshNode* found = mesh.getNodeById(42);
        REQUIRE(found != nullptr);
        CHECK(found->m_position.m_x == 1);
        CHECK(found->m_position.m_y == 2);
        CHECK(found->m_position.m_z == 3);

        CHECK(mesh.getNodeById(999) == nullptr);
    }

    SECTION("getElementById") {
        MeshElement elem;
        elem.m_id = 100;
        elem.m_type = ElementType::Tetrahedron;
        mesh.m_elements.push_back(elem);

        const MeshElement* found = mesh.getElementById(100);
        REQUIRE(found != nullptr);
        CHECK(found->m_type == ElementType::Tetrahedron);

        CHECK(mesh.getElementById(999) == nullptr);
    }

    SECTION("Clear operation") {
        MeshNode n;
        n.m_id = 1;
        mesh.m_nodes.push_back(n);

        MeshElement e;
        e.m_id = 1;
        mesh.m_elements.push_back(e);

        CHECK_FALSE(mesh.isEmpty());

        mesh.clear();

        CHECK(mesh.isEmpty());
        CHECK(mesh.m_nodes.empty());
        CHECK(mesh.m_elements.empty());
    }
}

TEST_CASE("MeshData - quality computation", "[mesh][quality]") {
    MeshData mesh;

    SECTION("Equilateral triangle quality") {
        // Create an equilateral triangle
        double h = std::sqrt(3.0) / 2.0;

        MeshNode n1, n2, n3;
        n1.m_id = 1;
        n1.m_position = Point3D(0, 0, 0);
        n2.m_id = 2;
        n2.m_position = Point3D(1, 0, 0);
        n3.m_id = 3;
        n3.m_position = Point3D(0.5, h, 0);

        mesh.m_nodes = {n1, n2, n3};

        MeshElement elem;
        elem.m_id = 1;
        elem.m_type = ElementType::Triangle;
        elem.m_nodeIds = {1, 2, 3};
        mesh.m_elements = {elem};

        mesh.computeQuality();

        REQUIRE(mesh.m_elementQualities.size() == 1);
        const auto& eq = mesh.m_elementQualities[0];

        CHECK(eq.m_isValid);
        CHECK_THAT(eq.m_aspectRatio, WithinRel(1.0, 0.01));
        CHECK_THAT(eq.m_minAngle, WithinRel(60.0, 0.1));
        CHECK_THAT(eq.m_maxAngle, WithinRel(60.0, 0.1));
    }

    SECTION("Right triangle quality") {
        MeshNode n1, n2, n3;
        n1.m_id = 1;
        n1.m_position = Point3D(0, 0, 0);
        n2.m_id = 2;
        n2.m_position = Point3D(1, 0, 0);
        n3.m_id = 3;
        n3.m_position = Point3D(0, 1, 0);

        mesh.m_nodes = {n1, n2, n3};

        MeshElement elem;
        elem.m_id = 1;
        elem.m_type = ElementType::Triangle;
        elem.m_nodeIds = {1, 2, 3};
        mesh.m_elements = {elem};

        mesh.computeQuality();

        REQUIRE(mesh.m_elementQualities.size() == 1);
        const auto& eq = mesh.m_elementQualities[0];

        CHECK(eq.m_isValid);
        CHECK_THAT(eq.m_minAngle, WithinRel(45.0, 0.1));
        CHECK_THAT(eq.m_maxAngle, WithinRel(90.0, 0.1));
    }

    SECTION("Poor quality elements detection") {
        // Create a very elongated triangle
        MeshNode n1, n2, n3;
        n1.m_id = 1;
        n1.m_position = Point3D(0, 0, 0);
        n2.m_id = 2;
        n2.m_position = Point3D(10, 0, 0); // Very long edge
        n3.m_id = 3;
        n3.m_position = Point3D(5, 0.1, 0); // Very short height

        mesh.m_nodes = {n1, n2, n3};

        MeshElement elem;
        elem.m_id = 1;
        elem.m_type = ElementType::Triangle;
        elem.m_nodeIds = {1, 2, 3};
        mesh.m_elements = {elem};

        mesh.computeQuality();

        QualityThresholds thresholds;
        thresholds.maxAspectRatio = 5.0;

        auto poorElements = mesh.getPoorQualityElements(thresholds);
        CHECK_FALSE(poorElements.empty());
    }
}

TEST_CASE("MeshQualitySummary", "[mesh][quality][summary]") {
    MeshData mesh;

    // Create a simple mesh with multiple triangles
    MeshNode n1, n2, n3, n4;
    n1.m_id = 1;
    n1.m_position = Point3D(0, 0, 0);
    n2.m_id = 2;
    n2.m_position = Point3D(1, 0, 0);
    n3.m_id = 3;
    n3.m_position = Point3D(0.5, 0.866, 0);
    n4.m_id = 4;
    n4.m_position = Point3D(1.5, 0.866, 0);

    mesh.m_nodes = {n1, n2, n3, n4};

    MeshElement e1, e2;
    e1.m_id = 1;
    e1.m_type = ElementType::Triangle;
    e1.m_nodeIds = {1, 2, 3};
    e2.m_id = 2;
    e2.m_type = ElementType::Triangle;
    e2.m_nodeIds = {2, 4, 3};

    mesh.m_elements = {e1, e2};

    mesh.computeQuality();

    CHECK(mesh.m_qualitySummary.totalElements == 2);
    CHECK(mesh.m_qualitySummary.validElements == 2);
    CHECK(mesh.m_qualitySummary.invalidElements == 0);
    CHECK(mesh.m_qualitySummary.avgQuality > 0);
}

TEST_CASE("ElementQuality - default values", "[mesh][quality]") {
    ElementQuality eq;

    CHECK(eq.m_elementId == 0);
    CHECK_THAT(eq.m_aspectRatio, WithinRel(1.0, 0.001));
    CHECK_THAT(eq.m_skewness, WithinRel(0.0, 0.001));
    CHECK_THAT(eq.m_quality, WithinRel(1.0, 0.001));
    CHECK(eq.m_isValid);
}
