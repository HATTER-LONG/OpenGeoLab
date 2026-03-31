/**
 * @file trackball_controller_test.cpp
 * @brief Unit tests for TrackballController
 */

#include <opengeolab/app/camera_state.hpp>
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

TEST_CASE("TrackballController pans along the view plane") {
    TrackballController controller;
    CameraState camera;

    controller.setViewportSize(200.0F, 100.0F);
    controller.begin(10.0F, 20.0F, TrackballController::Mode::Pan, camera);

    CHECK(controller.isActive());
    CHECK(controller.mode() == TrackballController::Mode::Pan);

    controller.update(20.0F, 0.0F, camera);

    checkVec3(camera.position, glm::vec3{-0.75F, -1.5F, 50.0F});
    checkVec3(camera.target, glm::vec3{-0.75F, -1.5F, 0.0F});
    CHECK(camera.distance() == doctest::Approx(50.0F));

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

TEST_CASE("TrackballController fit and view presets delegate to CameraState conventions") {
    TrackballController controller;
    CameraState camera;
    Scene::BoundingBox3D bounds;
    bounds.expand(glm::vec3{-2.0F, -1.0F, 3.0F});
    bounds.expand(glm::vec3{6.0F, 5.0F, 9.0F});

    controller.fitToScene(bounds, camera);
    const float fit_distance = bounds.diagonal() * 1.5F;
    checkVec3(camera.target, glm::vec3{2.0F, 2.0F, 6.0F});
    checkVec3(camera.position, glm::vec3{2.0F, 2.0F, 6.0F + fit_distance});

    controller.setViewPreset(TrackballController::ViewPreset::Top, camera);
    checkVec3(camera.position, glm::vec3{2.0F, 2.0F + fit_distance, 6.0F});
    checkVec3(camera.up, glm::vec3{0.0F, 0.0F, -1.0F});

    controller.setViewPreset(TrackballController::ViewPreset::Isometric, camera);
    const float axis_offset = fit_distance / std::sqrt(3.0F);
    checkVec3(camera.position, camera.target + glm::vec3{axis_offset, axis_offset, axis_offset});
    checkVec3(camera.up, glm::vec3{0.0F, 1.0F, 0.0F});
}

} // namespace OpenGeoLab::App::Tests
