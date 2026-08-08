/**
 * @file camera_state_test.cpp
 * @brief Unit tests for CameraState
 */

#include <opengeolab/scene/camera_state.hpp>

#include <doctest/doctest.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace OpenGeoLab::Scene::Tests {

namespace {

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

void checkMat4(const glm::mat4& actual, const glm::mat4& expected) {
    for(int column = 0; column < 4; ++column) {
        for(int row = 0; row < 4; ++row) {
            CHECK(actual[column][row] == doctest::Approx(expected[column][row]));
        }
    }
}

} // namespace

TEST_CASE("CameraState view and projection matrices follow cartesian orthographic model") {
    CameraState camera;
    camera.position = {3.0F, 4.0F, 20.0F};
    camera.target = {1.0F, 2.0F, 5.0F};
    camera.up = {0.0F, 1.0F, 0.0F};
    camera.nearPlane = -100.0F;
    camera.farPlane = 100.0F;

    const float aspect = 2.0F;
    const float distance = glm::length(camera.position - camera.target);
    const float half_height = distance * 0.5F;
    const float half_width = half_height * aspect;

    checkMat4(camera.viewMatrix(), glm::lookAt(camera.position, camera.target, camera.up));
    checkMat4(camera.projMatrix(aspect),
              glm::ortho(-half_width, half_width, -half_height, half_height, camera.nearPlane,
                         camera.farPlane));
    CHECK(camera.eyePosition().x == doctest::Approx(camera.position.x));
    CHECK(camera.eyePosition().y == doctest::Approx(camera.position.y));
    CHECK(camera.eyePosition().z == doctest::Approx(camera.position.z));
    CHECK(camera.distance() == doctest::Approx(distance));
}

TEST_CASE("CameraState reset and updateClipping restore symmetric defaults") {
    CameraState camera;
    camera.position = {5.0F, -2.0F, 6.0F};
    camera.target = {1.0F, -2.0F, 1.0F};
    camera.sceneExtent = 0.0F; // Isolate the camera-distance clipping calculation.

    camera.updateClipping();

    const float expected_half_range = glm::length(camera.position - camera.target) * 10.0F;
    CHECK(camera.nearPlane == doctest::Approx(-expected_half_range));
    CHECK(camera.farPlane == doctest::Approx(expected_half_range));

    camera.reset();

    checkVec3(camera.position, glm::vec3{0.0F, 0.0F, 50.0F});
    checkVec3(camera.target, glm::vec3{0.0F, 0.0F, 0.0F});
    checkVec3(camera.up, glm::vec3{0.0F, 1.0F, 0.0F});
    CHECK(camera.nearPlane == doctest::Approx(-500.0F));
    CHECK(camera.farPlane == doctest::Approx(500.0F));
}

TEST_CASE("CameraState fitToBoundingBox frames valid bounds and resets invalid bounds") {
    CameraState camera;
    BoundingBox3D bounds;
    bounds.expand(glm::vec3{-2.0F, -1.0F, 3.0F});
    bounds.expand(glm::vec3{6.0F, 5.0F, 9.0F});

    camera.fitToBoundingBox(bounds);

    const float fit_distance = bounds.diagonal() * 1.1F;
    checkVec3(camera.target, glm::vec3{2.0F, 2.0F, 6.0F});
    checkVec3(camera.position, glm::vec3{2.0F, 2.0F, 6.0F + fit_distance});
    CHECK(camera.nearPlane == doctest::Approx(-fit_distance * 10.0F));
    CHECK(camera.farPlane == doctest::Approx(fit_distance * 10.0F));

    camera.position = {1.0F, 1.0F, 1.0F};
    camera.target = {2.0F, 2.0F, 2.0F};
    camera.up = {1.0F, 0.0F, 0.0F};
    camera.nearPlane = -1.0F;
    camera.farPlane = 1.0F;

    camera.fitToBoundingBox(BoundingBox3D{});

    checkVec3(camera.position, glm::vec3{0.0F, 0.0F, 50.0F});
    checkVec3(camera.target, glm::vec3{0.0F, 0.0F, 0.0F});
    checkVec3(camera.up, glm::vec3{0.0F, 1.0F, 0.0F});
    CHECK(camera.nearPlane == doctest::Approx(-500.0F));
    CHECK(camera.farPlane == doctest::Approx(500.0F));
}

} // namespace OpenGeoLab::Scene::Tests
