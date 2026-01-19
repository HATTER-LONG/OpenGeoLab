/**
 * @file geometry_types_test.cpp
 * @brief Unit tests for geometry type definitions
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "geometry/geometry_types.hpp"

using namespace OpenGeoLab::Geometry;
using Catch::Matchers::WithinRel;

TEST_CASE("Point3D operations", "[geometry][types]") {
    SECTION("Default construction") {
        Point3D point;
        CHECK(point.x == 0.0);
        CHECK(point.y == 0.0);
        CHECK(point.z == 0.0);
    }

    SECTION("Parameterized construction") {
        Point3D point(1.0, 2.0, 3.0);
        CHECK(point.x == 1.0);
        CHECK(point.y == 2.0);
        CHECK(point.z == 3.0);
    }

    SECTION("Equality comparison") {
        Point3D p1(1.0, 2.0, 3.0);
        Point3D p2(1.0, 2.0, 3.0);
        Point3D p3(1.0, 2.0, 4.0);

        CHECK(p1 == p2);
        CHECK_FALSE(p1 == p3);
    }
}

TEST_CASE("Vector3D operations", "[geometry][types]") {
    SECTION("Default construction") {
        Vector3D vec;
        CHECK(vec.x == 0.0);
        CHECK(vec.y == 0.0);
        CHECK(vec.z == 0.0);
    }

    SECTION("Length calculation") {
        Vector3D vec(3.0, 4.0, 0.0);
        CHECK_THAT(vec.length(), WithinRel(5.0, 1e-10));
    }

    SECTION("Normalization") {
        Vector3D vec(3.0, 4.0, 0.0);
        Vector3D normalized = vec.normalized();
        CHECK_THAT(normalized.x, WithinRel(0.6, 1e-10));
        CHECK_THAT(normalized.y, WithinRel(0.8, 1e-10));
        CHECK_THAT(normalized.z, WithinRel(0.0, 1e-10));
    }

    SECTION("Zero vector normalization") {
        Vector3D zero;
        Vector3D normalized = zero.normalized();
        CHECK(normalized.x == 0.0);
        CHECK(normalized.y == 0.0);
        CHECK(normalized.z == 0.0);
    }
}

TEST_CASE("Color operations", "[geometry][types]") {
    SECTION("Default construction") {
        Color color;
        CHECK_THAT(color.r, WithinRel(0.8f, 1e-5f));
        CHECK_THAT(color.g, WithinRel(0.8f, 1e-5f));
        CHECK_THAT(color.b, WithinRel(0.8f, 1e-5f));
        CHECK_THAT(color.a, WithinRel(1.0f, 1e-5f));
    }

    SECTION("From RGB integers") {
        Color color = Color::fromRGB(255, 128, 0);
        CHECK_THAT(color.r, WithinRel(1.0f, 1e-5f));
        CHECK_THAT(color.g, WithinRel(0.502f, 1e-2f));
        CHECK_THAT(color.b, WithinRel(0.0f, 1e-5f));
        CHECK_THAT(color.a, WithinRel(1.0f, 1e-5f));
    }
}

TEST_CASE("BoundingBox operations", "[geometry][types]") {
    SECTION("Default construction") {
        BoundingBox bbox;
        // Default values - may not be valid
    }

    SECTION("Parameterized construction") {
        BoundingBox bbox(Point3D(0, 0, 0), Point3D(10, 10, 10));
        CHECK(bbox.isValid());
    }

    SECTION("Center calculation") {
        BoundingBox bbox(Point3D(0, 0, 0), Point3D(10, 20, 30));
        Point3D center = bbox.center();
        CHECK(center.x == 5.0);
        CHECK(center.y == 10.0);
        CHECK(center.z == 15.0);
    }

    SECTION("Diagonal length") {
        BoundingBox bbox(Point3D(0, 0, 0), Point3D(3, 4, 0));
        CHECK_THAT(bbox.diagonalLength(), WithinRel(5.0, 1e-10));
    }

    SECTION("Expand by point") {
        BoundingBox bbox(Point3D(0, 0, 0), Point3D(10, 10, 10));
        bbox.expand(Point3D(20, 5, 5));
        CHECK(bbox.max.x == 20.0);
        CHECK(bbox.min.x == 0.0);
    }

    SECTION("Expand by another bounding box") {
        BoundingBox bbox1(Point3D(0, 0, 0), Point3D(10, 10, 10));
        BoundingBox bbox2(Point3D(-5, -5, -5), Point3D(5, 5, 5));
        bbox1.expand(bbox2);
        CHECK(bbox1.min.x == -5.0);
        CHECK(bbox1.max.x == 10.0);
    }
}

TEST_CASE("Entity ID generation", "[geometry][types]") {
    SECTION("IDs are unique") {
        EntityId id1 = generateEntityId();
        EntityId id2 = generateEntityId();
        EntityId id3 = generateEntityId();

        CHECK(id1 != id2);
        CHECK(id2 != id3);
        CHECK(id1 != id3);
    }

    SECTION("IDs are never INVALID_ENTITY_ID") {
        for(int i = 0; i < 100; ++i) {
            EntityId id = generateEntityId();
            CHECK(id != INVALID_ENTITY_ID);
        }
    }
}
