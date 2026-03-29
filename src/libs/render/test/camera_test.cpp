/**
 * @file camera_test.cpp
 * @brief Unit tests for Camera and TrackballController.
 */

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/trackball_controller.hpp>

#include <doctest/doctest.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

#include <cmath>

namespace {

constexpr float kEps = 1e-4f;

bool vec3Near(const glm::vec3& a, const glm::vec3& b, float eps = kEps) {
    return glm::all(glm::epsilonEqual(a, b, eps));
}

bool mat4NonZero(const glm::mat4& m) {
    for(int c = 0; c < 4; ++c) {
        for(int r = 0; r < 4; ++r) {
            if(std::abs(m[c][r]) > 1e-10f) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

// ── Camera ──────────────────────────────────────────────────────────────────

TEST_CASE("Camera: default viewMatrix is non-zero") {
    OpenGeoLab::Render::Camera cam;
    CHECK(mat4NonZero(cam.viewMatrix()));
}

TEST_CASE("Camera: default projectionMatrix is non-zero") {
    OpenGeoLab::Render::Camera cam;
    CHECK(mat4NonZero(cam.projectionMatrix(1.5f)));
}

TEST_CASE("Camera: fitToBoundingBox updates position and target") {
    OpenGeoLab::Render::Camera cam;
    cam.fitToBoundingBox({10.f, 20.f, 30.f}, 5.f);
    CHECK(vec3Near(cam.target(), {10.f, 20.f, 30.f}));
    // Position should have moved from default
    CHECK_FALSE(vec3Near(cam.position(), {0.f, 0.f, 50.f}));
}

TEST_CASE("Camera: updateClipping adjusts near/far for ortho") {
    OpenGeoLab::Render::Camera cam;
    cam.updateClipping(100.f);
    // Ortho uses symmetric range: near = -10*distance, far = +10*distance
    CHECK(cam.nearPlane() < 0.f);
    CHECK(cam.farPlane() > 100.f);
    CHECK(cam.farPlane() == doctest::Approx(-cam.nearPlane()));
}

TEST_CASE("Camera: reset restores defaults") {
    OpenGeoLab::Render::Camera cam;
    cam.setPosition({99.f, 99.f, 99.f});
    cam.setFov(90.f);
    cam.reset();
    CHECK(vec3Near(cam.position(), {0.f, 0.f, 50.f}));
    CHECK(std::abs(cam.fov() - 45.f) < kEps);
}

// ── TrackballController ─────────────────────────────────────────────────────

TEST_CASE("Trackball: orbit changes camera position") {
    OpenGeoLab::Render::Camera cam;
    OpenGeoLab::Render::TrackballController ctrl;
    ctrl.setViewportSize(800.f, 600.f);

    glm::vec3 before = cam.position();
    ctrl.begin(400.f, 300.f, OpenGeoLab::Render::TrackballController::Mode::Orbit, cam);
    ctrl.update(450.f, 350.f, cam);
    ctrl.end();

    CHECK_FALSE(vec3Near(cam.position(), before, 1e-3f));
}

TEST_CASE("Trackball: pan shifts target") {
    OpenGeoLab::Render::Camera cam;
    OpenGeoLab::Render::TrackballController ctrl;
    ctrl.setViewportSize(800.f, 600.f);

    glm::vec3 before = cam.target();
    ctrl.begin(400.f, 300.f, OpenGeoLab::Render::TrackballController::Mode::Pan, cam);
    ctrl.update(500.f, 300.f, cam);
    ctrl.end();

    CHECK_FALSE(vec3Near(cam.target(), before, 1e-3f));
}

TEST_CASE("Trackball: wheelZoom changes distance") {
    OpenGeoLab::Render::Camera cam;
    OpenGeoLab::Render::TrackballController ctrl;
    ctrl.setViewportSize(800.f, 600.f);
    ctrl.syncFromCamera(cam);

    float dist_before = glm::length(cam.position() - cam.target());
    ctrl.wheelZoom(3.f, cam);
    float dist_after = glm::length(cam.position() - cam.target());

    CHECK(dist_after < dist_before);
}

TEST_CASE("Trackball: syncFromCamera round-trip consistency") {
    OpenGeoLab::Render::Camera cam;
    cam.setPosition({10.f, 20.f, 30.f});
    cam.setTarget({0.f, 0.f, 0.f});

    OpenGeoLab::Render::TrackballController ctrl;
    ctrl.setViewportSize(800.f, 600.f);
    ctrl.syncFromCamera(cam);

    // After sync, a no-op orbit should leave position essentially unchanged
    ctrl.begin(400.f, 300.f, OpenGeoLab::Render::TrackballController::Mode::Orbit, cam);
    ctrl.end();

    CHECK(vec3Near(cam.position(), {10.f, 20.f, 30.f}, 0.1f));
}
