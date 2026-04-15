/**
 * @file camera_state_test.cpp
 * @brief Unit tests for Scene::CameraState
 */

#include <opengeolab/scene/camera_state.hpp>

#include <doctest/doctest.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace OpenGeoLab::Scene::Tests {

namespace {

static void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

static void checkMat4(const glm::mat4& actual, const glm::mat4& expected) {
    for(int column = 0; column < 4; ++column) {
        for(int row = 0; row < 4; ++row) {
            CHECK(actual[column][row] == doctest::Approx(expected[column][row]));
        }
    }
}

} // namespace

TEST_CASE("Scene::CameraState view and projection matrices") {
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
    CHECK(camera.distance() == doctest::Approx(distance));
}

TEST_CASE("Scene::CameraState reset and updateClipping") {
    CameraState camera;
    camera.position = {5.0F, -2.0F, 6.0F};
    camera.target = {1.0F, -2.0F, 1.0F};
    camera.sceneExtent = 0.0F; // Disable scene floor to test camera-based range

    camera.updateClipping();

    const float expected_half_range = glm::length(camera.position - camera.target) * 10.0F;
    CHECK(camera.nearPlane == doctest::Approx(-expected_half_range));
    CHECK(camera.farPlane == doctest::Approx(expected_half_range));

    camera.reset();
    checkVec3(camera.position, glm::vec3{0.0F, 0.0F, 50.0F});
    checkVec3(camera.target, glm::vec3{0.0F, 0.0F, 0.0F});
}

TEST_CASE("Scene::CameraState clipping floored by sceneExtent") {
    CameraState camera;
    camera.position = {0.0F, 0.0F, 1.0F};
    camera.target = {0.0F, 0.0F, 0.0F};
    camera.sceneExtent = 500.0F;

    camera.updateClipping();

    // camera_distance=1 → camera-based=10, scene-based=1000 → floor wins
    CHECK(camera.nearPlane == doctest::Approx(-1000.0F));
    CHECK(camera.farPlane == doctest::Approx(1000.0F));
}

TEST_CASE("Scene::CameraState fitToBoundingBox") {
    CameraState camera;
    BoundingBox3D bounds;
    bounds.expand(glm::vec3{-2.0F, -1.0F, 3.0F});
    bounds.expand(glm::vec3{6.0F, 5.0F, 9.0F});

    camera.fitToBoundingBox(bounds);

    const float fit_distance = bounds.diagonal() * 1.1F;
    checkVec3(camera.target, glm::vec3{2.0F, 2.0F, 6.0F});
    checkVec3(camera.position, glm::vec3{2.0F, 2.0F, 6.0F + fit_distance});

    camera.fitToBoundingBox(BoundingBox3D{});
    checkVec3(camera.position, glm::vec3{0.0F, 0.0F, 50.0F});
}

} // namespace OpenGeoLab::Scene::Tests
