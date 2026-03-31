/**
 * @file point_vector3d_test.cpp
 * @brief Unit tests for Vector3D, Point3D, and almostEqual utilities
 */

#include <opengeolab/core/point_vector3d.hpp>

#include <doctest/doctest.h>

#include <type_traits>

namespace OpenGeoLab::Core::Tests {

// ============================================================
// almostEqual
// ============================================================

TEST_CASE("almostEqual - identical values") {
    CHECK(almostEqual(0.0, 0.0));
    CHECK(almostEqual(1.0, 1.0));
    CHECK(almostEqual(-3.14, -3.14));
}

TEST_CASE("almostEqual - within absolute tolerance") {
    CHECK(almostEqual(0.0, 1e-13));
    CHECK(almostEqual(1e-13, 0.0));
}

TEST_CASE("almostEqual - within relative tolerance") {
    CHECK(almostEqual(1e6, 1e6 + 0.5));
    CHECK_FALSE(almostEqual(1e6, 1e6 + 100.0));
}

TEST_CASE("almostEqual - clearly different values") {
    CHECK_FALSE(almostEqual(1.0, 2.0));
    CHECK_FALSE(almostEqual(0.0, 1.0));
}

TEST_CASE("almostEqual - integer types always use exact equality") {
    CHECK(almostEqual(42, 42));
    CHECK_FALSE(almostEqual(42, 43));
}

TEST_CASE("almostEqual - float type") {
    CHECK(almostEqual(1.0f, 1.0f));
    CHECK_FALSE(almostEqual(1.0f, 2.0f));
}

// ============================================================
// Vector3D - construction and element access
// ============================================================

TEST_CASE("Vector3D - default construction is zero") {
    constexpr Vec3d v;
    CHECK(v.x == 0.0);
    CHECK(v.y == 0.0);
    CHECK(v.z == 0.0);
}

TEST_CASE("Vector3D - value construction") {
    constexpr Vec3d v{1.0, 2.0, 3.0};
    CHECK(v.x == 1.0);
    CHECK(v.y == 2.0);
    CHECK(v.z == 3.0);
}

TEST_CASE("Vector3D - operator[] read") {
    constexpr Vec3d v{4.0, 5.0, 6.0};
    CHECK(v[0] == 4.0);
    CHECK(v[1] == 5.0);
    CHECK(v[2] == 6.0);
}

TEST_CASE("Vector3D - operator[] write") {
    Vec3d v;
    v[0] = 7.0;
    v[1] = 8.0;
    v[2] = 9.0;
    CHECK(v.x == 7.0);
    CHECK(v.y == 8.0);
    CHECK(v.z == 9.0);
}

// ============================================================
// Vector3D - unary operators
// ============================================================

TEST_CASE("Vector3D - unary plus") {
    constexpr Vec3d v{1.0, -2.0, 3.0};
    constexpr Vec3d pos = +v;
    CHECK(pos.x == 1.0);
    CHECK(pos.y == -2.0);
    CHECK(pos.z == 3.0);
}

TEST_CASE("Vector3D - unary minus") {
    constexpr Vec3d v{1.0, -2.0, 3.0};
    constexpr Vec3d neg = -v;
    CHECK(neg.x == -1.0);
    CHECK(neg.y == 2.0);
    CHECK(neg.z == -3.0);
}

// ============================================================
// Vector3D - compound assignment
// ============================================================

TEST_CASE("Vector3D - operator+=") {
    Vec3d a{1.0, 2.0, 3.0};
    a += Vec3d{10.0, 20.0, 30.0};
    CHECK(a.almostEquals({11.0, 22.0, 33.0}));
}

TEST_CASE("Vector3D - operator-=") {
    Vec3d a{10.0, 20.0, 30.0};
    a -= Vec3d{1.0, 2.0, 3.0};
    CHECK(a.almostEquals({9.0, 18.0, 27.0}));
}

TEST_CASE("Vector3D - operator*=") {
    Vec3d a{1.0, 2.0, 3.0};
    a *= 2.0;
    CHECK(a.almostEquals({2.0, 4.0, 6.0}));
}

TEST_CASE("Vector3D - operator/=") {
    Vec3d a{4.0, 6.0, 8.0};
    a /= 2.0;
    CHECK(a.almostEquals({2.0, 3.0, 4.0}));
}

// ============================================================
// Vector3D - binary arithmetic
// ============================================================

TEST_CASE("Vector3D - operator+") {
    constexpr Vec3d r = Vec3d{1.0, 2.0, 3.0} + Vec3d{4.0, 5.0, 6.0};
    CHECK(r.almostEquals({5.0, 7.0, 9.0}));
}

TEST_CASE("Vector3D - operator-") {
    constexpr Vec3d r = Vec3d{4.0, 5.0, 6.0} - Vec3d{1.0, 2.0, 3.0};
    CHECK(r.almostEquals({3.0, 3.0, 3.0}));
}

TEST_CASE("Vector3D - operator* (vector * scalar)") {
    constexpr Vec3d r = Vec3d{1.0, 2.0, 3.0} * 3.0;
    CHECK(r.almostEquals({3.0, 6.0, 9.0}));
}

TEST_CASE("Vector3D - operator* (scalar * vector)") {
    constexpr Vec3d r = 3.0 * Vec3d{1.0, 2.0, 3.0};
    CHECK(r.almostEquals({3.0, 6.0, 9.0}));
}

TEST_CASE("Vector3D - operator/") {
    constexpr Vec3d r = Vec3d{6.0, 9.0, 12.0} / 3.0;
    CHECK(r.almostEquals({2.0, 3.0, 4.0}));
}

// ============================================================
// Vector3D - geometric operations
// ============================================================

TEST_CASE("Vector3D - dot product") {
    constexpr Vec3d a{1.0, 2.0, 3.0};
    constexpr Vec3d b{4.0, 5.0, 6.0};
    CHECK(a.dot(b) == doctest::Approx(32.0));
}

TEST_CASE("Vector3D - dot product of orthogonal vectors is zero") {
    constexpr Vec3d x{1.0, 0.0, 0.0};
    constexpr Vec3d y{0.0, 1.0, 0.0};
    CHECK(x.dot(y) == doctest::Approx(0.0));
}

TEST_CASE("Vector3D - cross product") {
    constexpr Vec3d x{1.0, 0.0, 0.0};
    constexpr Vec3d y{0.0, 1.0, 0.0};
    constexpr Vec3d result = x.cross(y);
    CHECK(result.almostEquals({0.0, 0.0, 1.0}));
}

TEST_CASE("Vector3D - cross product anti-commutativity") {
    constexpr Vec3d a{1.0, 2.0, 3.0};
    constexpr Vec3d b{4.0, 5.0, 6.0};
    CHECK(a.cross(b).almostEquals(-b.cross(a)));
}

TEST_CASE("Vector3D - squaredLength") {
    constexpr Vec3d v{3.0, 4.0, 0.0};
    CHECK(v.squaredLength() == doctest::Approx(25.0));
}

TEST_CASE("Vector3D - length") {
    const Vec3d v{3.0, 4.0, 0.0};
    CHECK(v.length() == doctest::Approx(5.0));
}

TEST_CASE("Vector3D - length of zero vector") {
    const Vec3d v;
    CHECK(v.length() == doctest::Approx(0.0));
}

// ============================================================
// Vector3D - normalization
// ============================================================

TEST_CASE("Vector3D - normalized returns unit vector") {
    const Vec3d v{0.0, 3.0, 4.0};
    const Vec3d n = v.normalized();
    CHECK(n.length() == doctest::Approx(1.0));
    CHECK(n.almostEquals({0.0, 0.6, 0.8}));
}

TEST_CASE("Vector3D - normalized of zero vector returns zero") {
    const Vec3d v;
    const Vec3d n = v.normalized();
    CHECK(n.almostEquals({0.0, 0.0, 0.0}));
}

TEST_CASE("Vector3D - normalizeInplace succeeds for nonzero") {
    Vec3d v{0.0, 3.0, 4.0};
    CHECK(v.normalizeInplace());
    CHECK(v.length() == doctest::Approx(1.0));
}

TEST_CASE("Vector3D - normalizeInplace fails for zero vector") {
    Vec3d v;
    CHECK_FALSE(v.normalizeInplace());
}

// ============================================================
// Vector3D - almostEquals
// ============================================================

TEST_CASE("Vector3D - almostEquals for identical vectors") {
    constexpr Vec3d a{1.0, 2.0, 3.0};
    CHECK(a.almostEquals(a));
}

TEST_CASE("Vector3D - almostEquals for slightly different vectors") {
    constexpr Vec3d a{1.0, 2.0, 3.0};
    constexpr Vec3d b{1.0 + 1e-13, 2.0, 3.0};
    CHECK(a.almostEquals(b));
}

TEST_CASE("Vector3D - almostEquals rejects clearly different vectors") {
    constexpr Vec3d a{1.0, 2.0, 3.0};
    constexpr Vec3d b{1.0, 2.0, 4.0};
    CHECK_FALSE(a.almostEquals(b));
}

// ============================================================
// Vector3D - integer specialization
// ============================================================

TEST_CASE("Vector3D<int> - basic operations") {
    constexpr Vector3D<int> a{1, 2, 3};
    constexpr Vector3D<int> b{4, 5, 6};
    constexpr auto sum = a + b;
    CHECK(sum.x == 5);
    CHECK(sum.y == 7);
    CHECK(sum.z == 9);
}

TEST_CASE("Vector3D<int> - dot product") {
    constexpr Vector3D<int> a{1, 2, 3};
    constexpr Vector3D<int> b{4, 5, 6};
    CHECK(a.dot(b) == 32);
}

TEST_CASE("Vector3D<int> - normalizeInplace returns false") {
    Vector3D<int> v{1, 0, 0};
    CHECK_FALSE(v.normalizeInplace());
}

// ============================================================
// Point3D - construction and element access
// ============================================================

TEST_CASE("Point3D - default construction is origin") {
    constexpr Pt3d p;
    CHECK(p.x == 0.0);
    CHECK(p.y == 0.0);
    CHECK(p.z == 0.0);
}

TEST_CASE("Point3D - value construction") {
    constexpr Pt3d p{1.0, 2.0, 3.0};
    CHECK(p.x == 1.0);
    CHECK(p.y == 2.0);
    CHECK(p.z == 3.0);
}

TEST_CASE("Point3D - operator[] read") {
    constexpr Pt3d p{4.0, 5.0, 6.0};
    CHECK(p[0] == 4.0);
    CHECK(p[1] == 5.0);
    CHECK(p[2] == 6.0);
}

TEST_CASE("Point3D - operator[] write") {
    Pt3d p;
    p[0] = 7.0;
    p[1] = 8.0;
    p[2] = 9.0;
    CHECK(p.x == 7.0);
    CHECK(p.y == 8.0);
    CHECK(p.z == 9.0);
}

// ============================================================
// Point3D - distance
// ============================================================

TEST_CASE("Point3D - distanceTo") {
    const Pt3d a{0.0, 0.0, 0.0};
    const Pt3d b{3.0, 4.0, 0.0};
    CHECK(a.distanceTo(b) == doctest::Approx(5.0));
}

TEST_CASE("Point3D - squaredDistanceTo") {
    constexpr Pt3d a{0.0, 0.0, 0.0};
    constexpr Pt3d b{3.0, 4.0, 0.0};
    CHECK(a.squaredDistanceTo(b) == doctest::Approx(25.0));
}

TEST_CASE("Point3D - distance to self is zero") {
    const Pt3d p{1.0, 2.0, 3.0};
    CHECK(p.distanceTo(p) == doctest::Approx(0.0));
}

// ============================================================
// Point3D - almostEquals
// ============================================================

TEST_CASE("Point3D - almostEquals for identical points") {
    constexpr Pt3d a{1.0, 2.0, 3.0};
    CHECK(a.almostEquals(a));
}

TEST_CASE("Point3D - almostEquals rejects distant points") {
    constexpr Pt3d a{1.0, 2.0, 3.0};
    constexpr Pt3d b{10.0, 20.0, 30.0};
    CHECK_FALSE(a.almostEquals(b));
}

// ============================================================
// Point3D / Vector3D interaction
// ============================================================

TEST_CASE("Point3D + Vector3D") {
    constexpr Pt3d p{1.0, 2.0, 3.0};
    constexpr Vec3d v{10.0, 20.0, 30.0};
    constexpr Pt3d result = p + v;
    CHECK(result.almostEquals({11.0, 22.0, 33.0}));
}

TEST_CASE("Vector3D + Point3D (commutative)") {
    constexpr Pt3d p{1.0, 2.0, 3.0};
    constexpr Vec3d v{10.0, 20.0, 30.0};
    constexpr Pt3d result = v + p;
    CHECK(result.almostEquals({11.0, 22.0, 33.0}));
}

TEST_CASE("Point3D - Vector3D") {
    constexpr Pt3d p{10.0, 20.0, 30.0};
    constexpr Vec3d v{1.0, 2.0, 3.0};
    constexpr Pt3d result = p - v;
    CHECK(result.almostEquals({9.0, 18.0, 27.0}));
}

TEST_CASE("Point3D - Point3D yields Vector3D") {
    constexpr Pt3d a{10.0, 20.0, 30.0};
    constexpr Pt3d b{1.0, 2.0, 3.0};
    constexpr Vec3d result = a - b;
    CHECK(result.almostEquals({9.0, 18.0, 27.0}));
}

TEST_CASE("Point3D += Vector3D") {
    Pt3d p{1.0, 2.0, 3.0};
    p += Vec3d{10.0, 20.0, 30.0};
    CHECK(p.almostEquals({11.0, 22.0, 33.0}));
}

TEST_CASE("Point3D -= Vector3D") {
    Pt3d p{10.0, 20.0, 30.0};
    p -= Vec3d{1.0, 2.0, 3.0};
    CHECK(p.almostEquals({9.0, 18.0, 27.0}));
}

// ============================================================
// Type aliases
// ============================================================

TEST_CASE("Type aliases are correct") {
    static_assert(std::is_same_v<Vec3f, Vector3D<float>>);
    static_assert(std::is_same_v<Vec3d, Vector3D<double>>);
    static_assert(std::is_same_v<Pt3f, Point3D<float>>);
    static_assert(std::is_same_v<Pt3d, Point3D<double>>);
    CHECK(true);
}

// ============================================================
// Float specialization
// ============================================================

TEST_CASE("Vec3f - basic arithmetic") {
    constexpr Vec3f a{1.0f, 2.0f, 3.0f};
    constexpr Vec3f b{4.0f, 5.0f, 6.0f};
    constexpr auto sum = a + b;
    CHECK(sum.almostEquals({5.0f, 7.0f, 9.0f}));
}

TEST_CASE("Pt3f - distanceTo") {
    const Pt3f a{0.0f, 0.0f, 0.0f};
    const Pt3f b{3.0f, 4.0f, 0.0f};
    CHECK(a.distanceTo(b) == doctest::Approx(5.0f));
}

// ============================================================
// defaultEpsilon
// ============================================================

TEST_CASE("Vector3D<double>::defaultEpsilon is positive") {
    CHECK(Vec3d::defaultEpsilon() > 0.0);
    CHECK(Vec3d::defaultEpsilon() == DEFAULT_EPSILON);
}

TEST_CASE("Vector3D<int>::defaultEpsilon is zero") { CHECK(Vector3D<int>::defaultEpsilon() == 0); }

} // namespace OpenGeoLab::Core::Tests
