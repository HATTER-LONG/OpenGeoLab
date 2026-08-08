/**
 * @file trackball_controller_test.cpp
 * @brief Unit tests for TrackballController
 */

#include <opengeolab/app/trackball_controller.hpp>

#include <doctest/doctest.h>

#include <cmath>

namespace OpenGeoLab::App::Tests {

namespace {

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

} // namespace

TEST_CASE("TrackballController begins and ends drag operations") {
    TrackballController controller;
    CameraState camera;

    CHECK_FALSE(controller.isActive());

    controller.begin(10.0F, 20.0F, TrackballController::Mode::Orbit, camera);

    CHECK(controller.isActive());
    CHECK(controller.mode() == TrackballController::Mode::Orbit);

    controller.end();

    CHECK_FALSE(controller.isActive());
    CHECK(controller.mode() == TrackballController::Mode::None);
}

TEST_CASE("TrackballController drag zoom scales camera distance exponentially") {
    TrackballController controller;
    CameraState camera;

    controller.begin(10.0F, 60.0F, TrackballController::Mode::Zoom, camera);
    controller.update(10.0F, 0.0F, camera);

    const float expected_distance = 50.0F * std::pow(0.90F, 1.5F);
    CHECK(camera.distance() == doctest::Approx(expected_distance));
    CHECK(camera.position.z == doctest::Approx(expected_distance));
    CHECK(camera.nearPlane == doctest::Approx(-expected_distance * 10.0F));
    CHECK(camera.farPlane == doctest::Approx(expected_distance * 10.0F));
}

TEST_CASE("TrackballController wheel zoom clamps to minimum distance") {
    TrackballController controller;
    CameraState camera;

    camera.sceneExtent = 0.0F; // Isolate clipping derived from the minimum camera distance.
    camera.position = camera.target + glm::vec3{0.0F, 0.0F, 0.2F};
    camera.updateClipping();

    controller.wheelZoom(10.0F, camera);
    CHECK(camera.distance() < doctest::Approx(0.2F));

    controller.wheelZoom(100.0F, camera);

    CHECK(camera.distance() == doctest::Approx(0.1F));
    CHECK(camera.position.z == doctest::Approx(0.1F));
    CHECK(camera.nearPlane == doctest::Approx(-1.0F));
    CHECK(camera.farPlane == doctest::Approx(1.0F));
}

TEST_CASE("TrackballController orbit preserves target distance and updates position") {
    TrackballController controller;
    CameraState camera;

    controller.setViewportSize(200.0F, 200.0F);
    controller.begin(100.0F, 100.0F, TrackballController::Mode::Orbit, camera);
    controller.update(140.0F, 100.0F, camera);

    CHECK(camera.distance() == doctest::Approx(50.0F));
    CHECK(camera.target.x == doctest::Approx(0.0F));
    CHECK(camera.target.y == doctest::Approx(0.0F));
    CHECK(camera.target.z == doctest::Approx(0.0F));
    CHECK(camera.position.x < 0.0F);
    CHECK(camera.position.z < 50.0F);
    CHECK(camera.up.y == doctest::Approx(1.0F));
}

} // namespace OpenGeoLab::App::Tests
