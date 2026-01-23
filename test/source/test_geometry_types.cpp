/**
 * @file test_geometry_types.cpp
 * @brief Unit tests for geometry types (Point3D, Vector3D, BoundingBox3D)
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <geometry/geometry_types.hpp>

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using Catch::Approx;

// =============================================================================
// Point3D Tests
// =============================================================================
namespace {

using namespace OpenGeoLab::Geometry; // NOLINT
TEST_CASE("Point3D construction and access", "[geometry][point3d]") {
    SECTION("Default construction creates origin") {
        Point3D p;
        CHECK(p.m_x == 0.0);
        CHECK(p.m_y == 0.0);
        CHECK(p.m_z == 0.0);
    }

    SECTION("Parameterized construction") {
        Point3D p(1.0, 2.0, 3.0);
        CHECK(p.m_x == 1.0);
        CHECK(p.m_y == 2.0);
        CHECK(p.m_z == 3.0);
    }

    SECTION("Static origin() method") {
        Point3D origin = Point3D::origin();
        CHECK(origin.m_x == 0.0);
        CHECK(origin.m_y == 0.0);
        CHECK(origin.m_z == 0.0);
    }
}

TEST_CASE("Point3D arithmetic operations", "[geometry][point3d]") { // NOLINT
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(4.0, 5.0, 6.0);

    SECTION("Addition") {
        Point3D result = p1 + p2;
        CHECK(result.m_x == 5.0);
        CHECK(result.m_y == 7.0);
        CHECK(result.m_z == 9.0);
    }

    SECTION("Subtraction") {
        Point3D result = p2 - p1;
        CHECK(result.m_x == 3.0);
        CHECK(result.m_y == 3.0);
        CHECK(result.m_z == 3.0);
    }

    SECTION("Scalar multiplication") {
        Point3D result = p1 * 2.0;
        CHECK(result.m_x == 2.0);
        CHECK(result.m_y == 4.0);
        CHECK(result.m_z == 6.0);
    }

    SECTION("Scalar division") {
        Point3D result = p2 / 2.0;
        CHECK(result.m_x == 2.0);
        CHECK(result.m_y == 2.5);
        CHECK(result.m_z == 3.0);
    }
}

TEST_CASE("Point3D comparison operations", "[geometry][point3d]") {
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(1.0, 2.0, 3.0);
    Point3D p3(1.0, 2.0, 3.1);

    SECTION("Equality") {
        CHECK(p1 == p2);
        CHECK_FALSE(p1 == p3);
    }

    SECTION("Inequality") {
        CHECK_FALSE(p1 != p2);
        CHECK(p1 != p3);
    }

    SECTION("Approximate equality") {
        Point3D p4(1.0 + 1e-10, 2.0, 3.0);
        CHECK(p1.isApprox(p4));
        CHECK_FALSE(p1.isApprox(p3));
    }
}

TEST_CASE("Point3D distance calculations", "[geometry][point3d]") {
    Point3D p1(0.0, 0.0, 0.0);
    Point3D p2(3.0, 4.0, 0.0);
    Point3D p3(1.0, 1.0, 1.0);

    SECTION("Distance to another point") {
        CHECK(p1.distanceTo(p2) == Approx(5.0));
        CHECK(p1.distanceTo(p3) == Approx(std::sqrt(3.0)));
    }

    SECTION("Squared distance") {
        CHECK(p1.squaredDistanceTo(p2) == Approx(25.0));
        CHECK(p1.squaredDistanceTo(p3) == Approx(3.0));
    }
}

TEST_CASE("Point3D linear interpolation", "[geometry][point3d]") {
    Point3D p1(0.0, 0.0, 0.0);
    Point3D p2(10.0, 10.0, 10.0);

    SECTION("Lerp at t=0 returns start point") {
        Point3D result = p1.lerp(p2, 0.0);
        CHECK(result.isApprox(p1));
    }

    SECTION("Lerp at t=1 returns end point") {
        Point3D result = p1.lerp(p2, 1.0);
        CHECK(result.isApprox(p2));
    }

    SECTION("Lerp at t=0.5 returns midpoint") {
        Point3D result = p1.lerp(p2, 0.5);
        CHECK(result.m_x == Approx(5.0));
        CHECK(result.m_y == Approx(5.0));
        CHECK(result.m_z == Approx(5.0));
    }
}

// =============================================================================
// Vector3D Tests
// =============================================================================

TEST_CASE("Vector3D construction", "[geometry][vector3d]") { // NOLINT
    SECTION("Default construction creates zero vector") {
        Vector3D v;
        CHECK(v.m_x == 0.0);
        CHECK(v.m_y == 0.0);
        CHECK(v.m_z == 0.0);
    }

    SECTION("Parameterized construction") {
        Vector3D v(1.0, 2.0, 3.0);
        CHECK(v.m_x == 1.0);
        CHECK(v.m_y == 2.0);
        CHECK(v.m_z == 3.0);
    }

    SECTION("Construction from Point3D") {
        Point3D p(1.0, 2.0, 3.0);
        Vector3D v(p);
        CHECK(v.m_x == 1.0);
        CHECK(v.m_y == 2.0);
        CHECK(v.m_z == 3.0);
    }

    SECTION("Static unit vectors") {
        Vector3D x = Vector3D::unitX();
        Vector3D y = Vector3D::unitY();
        Vector3D z = Vector3D::unitZ();

        CHECK(x == Vector3D(1.0, 0.0, 0.0));
        CHECK(y == Vector3D(0.0, 1.0, 0.0));
        CHECK(z == Vector3D(0.0, 0.0, 1.0));
    }
}

TEST_CASE("Vector3D arithmetic operations", "[geometry][vector3d]") { // NOLINT
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(4.0, 5.0, 6.0);

    SECTION("Addition") {
        Vector3D result = v1 + v2;
        CHECK(result.m_x == 5.0);
        CHECK(result.m_y == 7.0);
        CHECK(result.m_z == 9.0);
    }

    SECTION("Subtraction") {
        Vector3D result = v2 - v1;
        CHECK(result.m_x == 3.0);
        CHECK(result.m_y == 3.0);
        CHECK(result.m_z == 3.0);
    }

    SECTION("Negation") {
        Vector3D result = -v1;
        CHECK(result.m_x == -1.0);
        CHECK(result.m_y == -2.0);
        CHECK(result.m_z == -3.0);
    }

    SECTION("Scalar multiplication (right)") {
        Vector3D result = v1 * 2.0;
        CHECK(result.m_x == 2.0);
        CHECK(result.m_y == 4.0);
        CHECK(result.m_z == 6.0);
    }

    SECTION("Scalar multiplication (left)") {
        Vector3D result = 2.0 * v1;
        CHECK(result.m_x == 2.0);
        CHECK(result.m_y == 4.0);
        CHECK(result.m_z == 6.0);
    }

    SECTION("In-place operations") {
        Vector3D v = v1;
        v += v2;
        CHECK(v == Vector3D(5.0, 7.0, 9.0));

        v = v1;
        v -= v2;
        CHECK(v == Vector3D(-3.0, -3.0, -3.0));

        v = v1;
        v *= 2.0;
        CHECK(v == Vector3D(2.0, 4.0, 6.0));

        v = v2;
        v /= 2.0;
        CHECK(v == Vector3D(2.0, 2.5, 3.0));
    }
}

TEST_CASE("Vector3D dot product", "[geometry][vector3d]") {
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(4.0, 5.0, 6.0);

    SECTION("Basic dot product") {
        double dot = v1.dot(v2);
        CHECK(dot == Approx(32.0)); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18
    }

    SECTION("Perpendicular vectors have zero dot product") {
        Vector3D x = Vector3D::unitX();
        Vector3D y = Vector3D::unitY();
        CHECK(x.dot(y) == Approx(0.0));
    }
}

TEST_CASE("Vector3D cross product", "[geometry][vector3d]") {
    SECTION("Unit vector cross products") {
        Vector3D x = Vector3D::unitX();
        Vector3D y = Vector3D::unitY();
        Vector3D z = Vector3D::unitZ();

        CHECK(x.cross(y).isApprox(z));
        CHECK(y.cross(z).isApprox(x));
        CHECK(z.cross(x).isApprox(y));
    }

    SECTION("Anti-commutativity") {
        Vector3D v1(1.0, 2.0, 3.0);
        Vector3D v2(4.0, 5.0, 6.0);

        Vector3D cross1 = v1.cross(v2);
        Vector3D cross2 = v2.cross(v1);

        CHECK(cross1.isApprox(-cross2));
    }
}

TEST_CASE("Vector3D length and normalization", "[geometry][vector3d]") {
    SECTION("Length calculation") {
        Vector3D v(3.0, 4.0, 0.0);
        CHECK(v.length() == Approx(5.0));
        CHECK(v.squaredLength() == Approx(25.0));
    }

    SECTION("Normalization") {
        Vector3D v(3.0, 4.0, 0.0);
        Vector3D normalized = v.normalized();

        CHECK(normalized.length() == Approx(1.0));
        CHECK(normalized.m_x == Approx(0.6));
        CHECK(normalized.m_y == Approx(0.8));
        CHECK(normalized.m_z == Approx(0.0));
    }

    SECTION("Zero vector normalization returns zero") {
        Vector3D zero = Vector3D::zero();
        Vector3D normalized = zero.normalized();
        CHECK(normalized.isZero());
    }

    SECTION("In-place normalization") {
        Vector3D v(3.0, 4.0, 0.0);
        v.normalize();
        CHECK(v.isUnit());
    }
}

TEST_CASE("Vector3D geometric properties", "[geometry][vector3d]") { // NOLINT
    SECTION("isZero check") {
        Vector3D zero = Vector3D::zero();
        Vector3D nonzero(0.001, 0.0, 0.0);

        CHECK(zero.isZero());
        CHECK_FALSE(nonzero.isZero());
    }

    SECTION("isUnit check") {
        Vector3D unit = Vector3D::unitX();
        Vector3D nonunit(2.0, 0.0, 0.0);

        CHECK(unit.isUnit());
        CHECK_FALSE(nonunit.isUnit());
    }

    SECTION("Angle between vectors") {
        Vector3D x = Vector3D::unitX();
        Vector3D y = Vector3D::unitY();
        Vector3D neg_x(-1.0, 0.0, 0.0);

        CHECK(x.angleTo(y) == Approx(M_PI / 2.0));
        CHECK(x.angleTo(neg_x) == Approx(M_PI));
        CHECK(x.angleTo(x) == Approx(0.0));
    }

    SECTION("Parallel check") {
        Vector3D v1(1.0, 0.0, 0.0);
        Vector3D v2(2.0, 0.0, 0.0);
        Vector3D v3(-1.0, 0.0, 0.0);
        Vector3D v4(0.0, 1.0, 0.0);

        CHECK(v1.isParallelTo(v2));
        CHECK(v1.isParallelTo(v3));
        CHECK_FALSE(v1.isParallelTo(v4));
    }

    SECTION("Perpendicular check") {
        Vector3D x = Vector3D::unitX();
        Vector3D y = Vector3D::unitY();
        Vector3D v(1.0, 1.0, 0.0);

        CHECK(x.isPerpendicularTo(y));
        CHECK_FALSE(x.isPerpendicularTo(v));
    }
}

TEST_CASE("Vector3D projection and reflection", "[geometry][vector3d]") {
    SECTION("Projection onto another vector") {
        Vector3D v(3.0, 4.0, 0.0);
        Vector3D onto(1.0, 0.0, 0.0);

        Vector3D proj = v.projectOnto(onto);
        CHECK(proj.m_x == Approx(3.0));
        CHECK(proj.m_y == Approx(0.0));
        CHECK(proj.m_z == Approx(0.0));
    }

    SECTION("Reflection about a normal") {
        Vector3D incident(1.0, -1.0, 0.0);
        Vector3D normal(0.0, 1.0, 0.0);

        Vector3D reflected = incident.reflect(normal);
        CHECK(reflected.m_x == Approx(1.0));
        CHECK(reflected.m_y == Approx(1.0));
        CHECK(reflected.m_z == Approx(0.0));
    }
}

// =============================================================================
// BoundingBox3D Tests
// =============================================================================

TEST_CASE("BoundingBox3D construction and validity", "[geometry][bbox]") {
    SECTION("Default construction creates invalid box") {
        BoundingBox3D box;
        CHECK_FALSE(box.isValid());
    }

    SECTION("Construction from min/max points") {
        Point3D min(0.0, 0.0, 0.0);
        Point3D max(1.0, 1.0, 1.0);
        BoundingBox3D box(min, max);

        CHECK(box.isValid());
        CHECK(box.m_min == min);
        CHECK(box.m_max == max);
    }
}

TEST_CASE("BoundingBox3D expand operations", "[geometry][bbox]") {
    BoundingBox3D box;

    SECTION("Expand by single point") {
        box.expand(Point3D(1.0, 2.0, 3.0));
        CHECK(box.isValid());
        CHECK(box.m_min == Point3D(1.0, 2.0, 3.0));
        CHECK(box.m_max == Point3D(1.0, 2.0, 3.0));
    }

    SECTION("Expand by multiple points") {
        box.expand(Point3D(0.0, 0.0, 0.0));
        box.expand(Point3D(1.0, 2.0, 3.0));
        box.expand(Point3D(-1.0, -1.0, -1.0));

        CHECK(box.m_min == Point3D(-1.0, -1.0, -1.0));
        CHECK(box.m_max == Point3D(1.0, 2.0, 3.0));
    }

    SECTION("Expand by another box") {
        BoundingBox3D box1(Point3D(0.0, 0.0, 0.0), Point3D(1.0, 1.0, 1.0));
        BoundingBox3D box2(Point3D(-1.0, 2.0, 0.5), Point3D(0.5, 3.0, 2.0));

        box1.expand(box2);

        CHECK(box1.m_min == Point3D(-1.0, 0.0, 0.0));
        CHECK(box1.m_max == Point3D(1.0, 3.0, 2.0));
    }
}

TEST_CASE("BoundingBox3D geometric properties", "[geometry][bbox]") {
    BoundingBox3D box(Point3D(0.0, 0.0, 0.0), Point3D(2.0, 4.0, 6.0));

    SECTION("Center calculation") {
        Point3D center = box.center();
        CHECK(center.m_x == Approx(1.0));
        CHECK(center.m_y == Approx(2.0));
        CHECK(center.m_z == Approx(3.0));
    }

    SECTION("Size calculation") {
        Vector3D size = box.size();
        CHECK(size.m_x == Approx(2.0));
        CHECK(size.m_y == Approx(4.0));
        CHECK(size.m_z == Approx(6.0));
    }

    SECTION("Diagonal length") {
        double diag = box.diagonal();
        // sqrt(2² + 4² + 6²) = sqrt(4 + 16 + 36) = sqrt(56)
        CHECK(diag == Approx(std::sqrt(56.0)));
    }
}

TEST_CASE("BoundingBox3D containment and intersection", "[geometry][bbox]") {
    BoundingBox3D box(Point3D(0.0, 0.0, 0.0), Point3D(10.0, 10.0, 10.0));

    SECTION("Point containment") {
        CHECK(box.contains(Point3D(5.0, 5.0, 5.0)));
        CHECK(box.contains(Point3D(0.0, 0.0, 0.0)));    // on boundary
        CHECK(box.contains(Point3D(10.0, 10.0, 10.0))); // on boundary
        CHECK_FALSE(box.contains(Point3D(-1.0, 5.0, 5.0)));
        CHECK_FALSE(box.contains(Point3D(11.0, 5.0, 5.0)));
    }

    SECTION("Box intersection") {
        BoundingBox3D overlapping(Point3D(5.0, 5.0, 5.0), Point3D(15.0, 15.0, 15.0));
        BoundingBox3D adjacent(Point3D(10.0, 0.0, 0.0), Point3D(20.0, 10.0, 10.0));
        BoundingBox3D separate(Point3D(20.0, 20.0, 20.0), Point3D(30.0, 30.0, 30.0));

        CHECK(box.intersects(overlapping));
        CHECK(box.intersects(adjacent)); // touching boxes intersect
        CHECK_FALSE(box.intersects(separate));
    }
}

// =============================================================================
// ID System Tests
// =============================================================================

TEST_CASE("EntityId generation", "[geometry][id]") {
    resetEntityIdGenerator();

    SECTION("Generated IDs are unique and sequential") {
        EntityId id1 = generateEntityId();
        EntityId id2 = generateEntityId();
        EntityId id3 = generateEntityId();

        CHECK(id1 != INVALID_ENTITY_ID);
        CHECK(id2 != INVALID_ENTITY_ID);
        CHECK(id3 != INVALID_ENTITY_ID);
        CHECK(id1 != id2);
        CHECK(id2 != id3);
        CHECK(id1 < id2);
        CHECK(id2 < id3);
    }
}

TEST_CASE("EntityUID generation", "[geometry][id]") {
    SECTION("UIDs are unique within same type") {
        resetEntityUIDGenerator(EntityType::Vertex);

        EntityUID uid1 = generateEntityUID(EntityType::Vertex);
        EntityUID uid2 = generateEntityUID(EntityType::Vertex);

        CHECK(uid1 != INVALID_ENTITY_UID);
        CHECK(uid2 != INVALID_ENTITY_UID);
        CHECK(uid1 != uid2);
    }

    SECTION("UIDs are independent across types") {
        resetEntityUIDGenerator(EntityType::Vertex);
        resetEntityUIDGenerator(EntityType::Edge);

        EntityUID vertex_uid = generateEntityUID(EntityType::Vertex);
        EntityUID edge_uid = generateEntityUID(EntityType::Edge);

        // Both should be 1 after reset (UIDs are type-scoped)
        CHECK(vertex_uid == edge_uid);
    }
}

TEST_CASE("isApproxEqual function", "[geometry][util]") {
    CHECK(isApproxEqual(1.0, 1.0));
    CHECK(isApproxEqual(1.0, 1.0 + 1e-10));
    CHECK_FALSE(isApproxEqual(1.0, 1.1));
    CHECK(isApproxEqual(1.0, 1.05, 0.1));
}

} // namespace