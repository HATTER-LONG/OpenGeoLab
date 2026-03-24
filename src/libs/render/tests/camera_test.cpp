/**
 * @file camera_test.cpp
 * @brief Unit tests for Camera orbit model.
 */
#include <opengeolab/render/camera.hpp>
#include <opengeolab/scene/bounding_box.hpp>

#include <doctest/doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

#include <array>
#include <cmath>

using OpenGeoLab::Render::Camera;
using OpenGeoLab::Scene::BoundingBox;

namespace {
constexpr float kEps = 1e-4F;

bool vec3Near(const glm::vec3& a, const glm::vec3& b, float eps = kEps) {
    return glm::all(glm::epsilonEqual(a, b, eps));
}

bool mat4Near(const glm::mat4& a, const glm::mat4& b, float eps = kEps) {
    for(int column = 0; column < 4; ++column) {
        if(!glm::all(glm::epsilonEqual(a[column], b[column], eps))) {
            return false;
        }
    }
    return true;
}

std::array<glm::vec3, 8> boundingBoxCorners(const BoundingBox& bbox) {
    return {{
        {bbox.min.x, bbox.min.y, bbox.min.z},
        {bbox.min.x, bbox.min.y, bbox.max.z},
        {bbox.min.x, bbox.max.y, bbox.min.z},
        {bbox.min.x, bbox.max.y, bbox.max.z},
        {bbox.max.x, bbox.min.y, bbox.min.z},
        {bbox.max.x, bbox.min.y, bbox.max.z},
        {bbox.max.x, bbox.max.y, bbox.min.z},
        {bbox.max.x, bbox.max.y, bbox.max.z},
    }};
}
} // namespace

TEST_CASE("Camera") {
    Camera cam;

    SUBCASE("default state produces valid view matrix") {
        const auto v = cam.viewMatrix();
        CHECK_FALSE(mat4Near(v, glm::mat4{1.0F}));
    }

    SUBCASE("default state produces valid projection matrix") {
        cam.setPerspective(45.0F, 1.0F, 0.1F, 1000.0F);
        const auto p = cam.projectionMatrix();
        CHECK_FALSE(mat4Near(p, glm::mat4{1.0F}));
    }

    SUBCASE("setOrthographic updates projection matrix") {
        cam.setOrthographic(10.0F, 1.0F, 0.1F, 100.0F);
        const auto p = cam.projectionMatrix();
        CHECK_FALSE(mat4Near(p, glm::mat4{1.0F}));
        CHECK(glm::epsilonEqual(p[3][3], 1.0F, kEps));
    }

    SUBCASE("orbit changes eye position") {
        const auto pos_before = cam.position();
        cam.orbit(0.5F, 0.0F);
        const auto pos_after = cam.position();
        CHECK_FALSE(vec3Near(pos_before, pos_after));
    }

    SUBCASE("orbit clamps phi to avoid pole singularity") {
        cam.orbit(0.0F, 100.0F);
        const auto pos = cam.position();
        CHECK(std::isfinite(pos.x));
        CHECK(std::isfinite(pos.y));
        CHECK(std::isfinite(pos.z));
    }

    SUBCASE("pan shifts both eye and target") {
        const auto state0 = cam.captureState();
        cam.pan(1.0F, 0.0F);
        const auto state1 = cam.captureState();
        CHECK_FALSE(vec3Near(state0.eye, state1.eye));
        CHECK_FALSE(vec3Near(state0.target, state1.target));
    }

    SUBCASE("zoom changes distance to target") {
        const auto dist_before = glm::length(cam.position() - cam.captureState().target);
        cam.zoom(0.5F);
        const auto dist_after = glm::length(cam.position() - cam.captureState().target);
        CHECK(dist_after > dist_before);
    }

    SUBCASE("zoom clamps to minimum distance") {
        for(int i = 0; i < 100; ++i) {
            cam.zoom(2.0F);
        }
        const auto dist = glm::length(cam.position() - cam.captureState().target);
        CHECK(dist > 0.01F);
    }

    SUBCASE("fitAll centers camera on bounding box") {
        BoundingBox bbox;
        bbox.expand(glm::vec3{-5.0F, 0.0F, -5.0F});
        bbox.expand(glm::vec3{5.0F, 10.0F, 5.0F});
        cam.fitAll(bbox);
        const auto state = cam.captureState();
        const auto center = bbox.center();
        CHECK(vec3Near(state.target, center, 0.1F));
    }

    SUBCASE("captureState restoreState round-trips") {
        cam.orbit(0.3F, 0.2F);
        cam.pan(1.0F, -0.5F);
        const auto saved = cam.captureState();
        Camera cam2;
        cam2.restoreState(saved);
        const auto restored = cam2.captureState();
        CHECK(vec3Near(saved.eye, restored.eye));
        CHECK(vec3Near(saved.target, restored.target));
    }

    SUBCASE("captureState restoreState preserves projection aspect") {
        cam.setPerspective(45.0F, 2.0F, 0.1F, 100.0F);
        const auto saved = cam.captureState();
        Camera cam2;
        cam2.restoreState(saved);
        CHECK(mat4Near(cam.projectionMatrix(), cam2.projectionMatrix()));
    }

    SUBCASE("orthographic screenToWorldRay origin varies by screen position") {
        cam.setOrthographic(10.0F, 1.0F, 0.1F, 100.0F);
        const auto center_ray = cam.screenToWorldRay(400.0F, 400.0F, 800, 800);
        const auto corner_ray = cam.screenToWorldRay(700.0F, 200.0F, 800, 800);
        CHECK_FALSE(vec3Near(center_ray.origin, corner_ray.origin));
        CHECK(vec3Near(center_ray.direction, corner_ray.direction, 1.0e-3F));
    }

    SUBCASE("fitAll frames box on narrow aspect ratio") {
        BoundingBox bbox;
        bbox.expand(glm::vec3{-5.0F, -0.5F, -0.5F});
        bbox.expand(glm::vec3{5.0F, 0.5F, 0.5F});
        cam.setPerspective(45.0F, 0.5F, 0.1F, 100.0F);
        cam.fitAll(bbox);

        const glm::mat4 vp = cam.projectionMatrix() * cam.viewMatrix();
        for(const glm::vec3& corner : boundingBoxCorners(bbox)) {
            const glm::vec4 clip = vp * glm::vec4{corner, 1.0F};
            const glm::vec3 ndc = glm::vec3{clip} / clip.w;
            CHECK(std::abs(ndc.x) <= 1.0F + 1.0e-3F);
            CHECK(std::abs(ndc.y) <= 1.0F + 1.0e-3F);
        }
    }

    SUBCASE("screenToWorldRay center pixel points forward") {
        cam.setPerspective(45.0F, 1.0F, 0.1F, 100.0F);
        const auto ray = cam.screenToWorldRay(400.0F, 400.0F, 800, 800);
        CHECK(glm::length(ray.direction) > 0.9F);
        CHECK(vec3Near(ray.origin, cam.position(), 1.0F));
    }
}
