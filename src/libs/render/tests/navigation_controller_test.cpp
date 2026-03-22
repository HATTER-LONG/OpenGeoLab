#include <doctest/doctest.h>
#include <opengeolab/render/navigation_controller.hpp>

#include <array>
#include <cmath>

using OpenGeoLab::Render::NavigationController;

TEST_CASE("compute_pan translates position along view plane") {
    std::array<float, 4> orientation = {0.f, 0.f, 0.f, 1.f};
    std::array<float, 3> position = {0.f, 0.f, 10.f};

    auto result = NavigationController::compute_pan(
        position, orientation, 0.1f, 0.0f, 5.f);

    CHECK(result.new_position[0] != doctest::Approx(0.f));
    CHECK(result.new_position[2] == doctest::Approx(10.f));
}

TEST_CASE("compute_zoom perspective reduces focal distance on positive delta") {
    std::array<float, 4> orientation = {0.f, 0.f, 0.f, 1.f};
    std::array<float, 3> position = {0.f, 0.f, 10.f};

    auto result = NavigationController::compute_zoom(
        position, orientation, 5.f, 1.f, false, 10.f);

    CHECK(result.new_focal_distance < 5.f);
    CHECK(result.new_position[2] < 10.f);
    CHECK(result.new_height == doctest::Approx(10.f));
}

TEST_CASE("compute_zoom orthographic reduces height on positive delta") {
    std::array<float, 4> orientation = {0.f, 0.f, 0.f, 1.f};
    std::array<float, 3> position = {0.f, 0.f, 10.f};

    auto result = NavigationController::compute_zoom(
        position, orientation, 5.f, 1.f, true, 10.f);

    CHECK(result.new_height < 10.f);
    CHECK(result.new_position[0] == doctest::Approx(0.f));
    CHECK(result.new_position[1] == doctest::Approx(0.f));
    CHECK(result.new_position[2] == doctest::Approx(10.f));
}
