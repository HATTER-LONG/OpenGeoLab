#include <doctest/doctest.h>

#include <opengeolab/geometry/bounding_box_calculator.hpp>

#include <array>
#include <stdexcept>

namespace OpenGeoLab::Geometry {
namespace {

TEST_CASE("BoundingBoxCalculator compute returns extrema for input points") {
    const std::array<Point3D, 3> points{
        Point3D{1.5, -2.0, 8.0},
        Point3D{-4.0, 5.0, 3.25},
        Point3D{2.25, -6.5, 10.0},
    };

    const BoundingBox bounding_box = BoundingBoxCalculator::compute(points);

    CHECK(bounding_box.min.x == doctest::Approx(-4.0));
    CHECK(bounding_box.min.y == doctest::Approx(-6.5));
    CHECK(bounding_box.min.z == doctest::Approx(3.25));
    CHECK(bounding_box.max.x == doctest::Approx(2.25));
    CHECK(bounding_box.max.y == doctest::Approx(5.0));
    CHECK(bounding_box.max.z == doctest::Approx(10.0));
}

TEST_CASE("BoundingBoxCalculator compute rejects empty input") {
    const std::array<Point3D, 0> points{};
    const auto compute_for_empty_input = [&points]() {
        static_cast<void>(BoundingBoxCalculator::compute(points));
    };

    CHECK_THROWS_AS(compute_for_empty_input(), std::invalid_argument);
}

TEST_CASE("BoundingBoxCalculator generates deterministic random points") {
    const auto first_run = BoundingBoxCalculator::generateRandomPoints(3, 7U);
    const auto second_run = BoundingBoxCalculator::generateRandomPoints(3, 7U);

    REQUIRE(first_run.size() == 3);
    REQUIRE(second_run.size() == 3);

    for(std::size_t index = 0; index < first_run.size(); ++index) {
        CHECK(first_run[index].x == doctest::Approx(second_run[index].x));
        CHECK(first_run[index].y == doctest::Approx(second_run[index].y));
        CHECK(first_run[index].z == doctest::Approx(second_run[index].z));
    }
}

} // namespace
} // namespace OpenGeoLab::Geometry
