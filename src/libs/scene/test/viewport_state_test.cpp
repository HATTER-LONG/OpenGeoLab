/**
 * @file viewport_state_test.cpp
 * @brief Unit tests for ViewportState
 */

#include <opengeolab/scene/viewport_state.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::PickAction;
using OpenGeoLab::Scene::BoundingBox3D;
using OpenGeoLab::Scene::CameraState;
using OpenGeoLab::Scene::PendingPickArea;
using OpenGeoLab::Scene::PickAreaCoordType;
using OpenGeoLab::Scene::ViewportState;
using OpenGeoLab::Scene::ViewPreset;

TEST_SUITE("ViewportState") {

    TEST_CASE("default camera matches CameraState reset") {
        ViewportState vps;
        const auto cam = vps.camera();
        CHECK(cam.position.x == doctest::Approx(0.0F));
        CHECK(cam.position.y == doctest::Approx(0.0F));
        CHECK(cam.position.z == doctest::Approx(50.0F));
        CHECK(cam.target == glm::vec3(0.0F));
    }

    TEST_CASE("setCamera updates state and bumps version") {
        ViewportState vps;
        const auto v0 = vps.cameraVersion();

        CameraState state;
        state.position = {10.0F, 20.0F, 30.0F};
        state.target = {1.0F, 2.0F, 3.0F};
        vps.setCamera(state);

        CHECK(vps.cameraVersion() > v0);
        const auto cam = vps.camera();
        CHECK(cam.position.x == doctest::Approx(10.0F));
        CHECK(cam.position.y == doctest::Approx(20.0F));
        CHECK(cam.position.z == doctest::Approx(30.0F));
        CHECK(cam.target.y == doctest::Approx(2.0F));
    }

    TEST_CASE("setViewPreset Front places camera on +Z axis") {
        ViewportState vps;
        vps.setViewPreset(ViewPreset::Front);
        const auto cam = vps.camera();
        CHECK(cam.position.z > cam.target.z);
        CHECK(cam.position.x == doctest::Approx(cam.target.x));
        CHECK(cam.position.y == doctest::Approx(cam.target.y));
    }

    TEST_CASE("setViewPreset Top places camera on +Y axis with -Z up") {
        ViewportState vps;
        vps.setViewPreset(ViewPreset::Top);
        const auto cam = vps.camera();
        CHECK(cam.position.y > cam.target.y);
        CHECK(cam.up.z == doctest::Approx(-1.0F));
    }

    TEST_CASE("setViewPreset Isometric uses diagonal direction") {
        ViewportState vps;
        vps.setViewPreset(ViewPreset::Isometric);
        const auto cam = vps.camera();
        const auto offset = cam.position - cam.target;
        CHECK(offset.x == doctest::Approx(offset.y));
        CHECK(offset.y == doctest::Approx(offset.z));
    }

    TEST_CASE("fitToBounds frames valid bounds") {
        ViewportState vps;
        BoundingBox3D bounds;
        bounds.expand(glm::vec3{-5.0F, -5.0F, -5.0F});
        bounds.expand(glm::vec3{5.0F, 5.0F, 5.0F});

        vps.fitToBounds(bounds);

        const auto cam = vps.camera();
        CHECK(cam.target.x == doctest::Approx(0.0F));
        CHECK(cam.target.y == doctest::Approx(0.0F));
        CHECK(cam.target.z == doctest::Approx(0.0F));

        const float expected_distance = bounds.diagonal() * 1.1F;
        CHECK(cam.position.z == doctest::Approx(expected_distance));
    }

    TEST_CASE("fitToBounds with invalid bounds resets camera") {
        ViewportState vps;
        CameraState custom;
        custom.position = {99.0F, 99.0F, 99.0F};
        vps.setCamera(custom);

        vps.fitToBounds(BoundingBox3D{});

        const auto cam = vps.camera();
        CHECK(cam.position.z == doctest::Approx(50.0F));
    }

    TEST_CASE("requestPickArea and consumePickArea round-trip") {
        ViewportState vps;
        CHECK_FALSE(vps.consumePickArea().has_value());

        const PendingPickArea req{0.1F,           0.2F, 0.8F, 0.9F, PickAreaCoordType::Normalized,
                                  PickAction::Add};
        vps.requestPickArea(req);

        const auto consumed = vps.consumePickArea();
        REQUIRE(consumed.has_value());
        CHECK(consumed->x0 == doctest::Approx(0.1F));
        CHECK(consumed->y0 == doctest::Approx(0.2F));
        CHECK(consumed->x1 == doctest::Approx(0.8F));
        CHECK(consumed->y1 == doctest::Approx(0.9F));
        CHECK(consumed->coordType == PickAreaCoordType::Normalized);
        CHECK(consumed->action == PickAction::Add);

        CHECK_FALSE(vps.consumePickArea().has_value());
    }

    TEST_CASE("cameraChanged signal fires on setCamera") {
        ViewportState vps;
        int count = 0;
        auto conn = vps.cameraChanged.connect([&]() { ++count; });

        CameraState state;
        state.position = {1.0F, 2.0F, 3.0F};
        vps.setCamera(state);
        CHECK(count == 1);

        vps.setViewPreset(ViewPreset::Front);
        CHECK(count == 2);

        BoundingBox3D bounds;
        bounds.expand(glm::vec3{-1.0F, -1.0F, -1.0F});
        bounds.expand(glm::vec3{1.0F, 1.0F, 1.0F});
        vps.fitToBounds(bounds);
        CHECK(count == 3);
    }

    TEST_CASE("pickAreaRequested signal fires on requestPickArea") {
        ViewportState vps;
        int count = 0;
        auto conn = vps.pickAreaRequested.connect([&]() { ++count; });

        vps.requestPickArea(
            {0.0F, 0.0F, 1.0F, 1.0F, PickAreaCoordType::Normalized, PickAction::Add});
        CHECK(count == 1);
    }

} // TEST_SUITE
