/**
 * @file trackball_test.cpp
 * @brief Unit tests for trackball rotation
 */

#include <catch2/catch_test_macros.hpp>

#include "render/trackball.hpp"

using namespace OpenGeoLab::Render;

TEST_CASE("Trackball - initialization", "[render][trackball]") {
    Trackball trackball;

    SECTION("Default rotation is identity") {
        QQuaternion rot = trackball.rotation();
        CHECK(rot.isIdentity());
    }

    SECTION("Default trackball size") { CHECK(trackball.trackballSize() == 0.8f); }
}

TEST_CASE("Trackball - rotation operations", "[render][trackball]") {
    Trackball trackball;
    trackball.setViewportSize(QSize(800, 600));

    SECTION("Begin sets initial position") {
        trackball.begin(400, 300);
        // No exception should be thrown
    }

    SECTION("Rotate returns quaternion") {
        trackball.begin(400, 300);
        QQuaternion rot = trackball.rotate(500, 300);

        // Should return a non-identity quaternion for horizontal movement
        CHECK_FALSE(rot.isIdentity());
    }

    SECTION("Small movement returns near-identity") {
        trackball.begin(400, 300);
        QQuaternion rot = trackball.rotate(401, 300);

        // Very small rotation
        float angle;
        QVector3D axis;
        rot.getAxisAndAngle(&axis, &angle);
        CHECK(std::abs(angle) < 5.0f);
    }

    SECTION("Reset clears rotation") {
        trackball.begin(100, 100);
        trackball.rotate(500, 300);
        trackball.rotate(600, 400);

        trackball.reset();

        CHECK(trackball.rotation().isIdentity());
    }
}

TEST_CASE("Trackball - viewport size", "[render][trackball]") {
    Trackball trackball;

    SECTION("Set viewport size") {
        trackball.setViewportSize(QSize(1920, 1080));
        // No exception should be thrown
    }
}

TEST_CASE("Trackball - trackball size parameter", "[render][trackball]") {
    Trackball trackball;

    SECTION("Set trackball size") {
        trackball.setTrackballSize(1.0f);
        CHECK(trackball.trackballSize() == 1.0f);
    }
}

TEST_CASE("Trackball - rotation accumulation", "[render][trackball]") {
    Trackball trackball;
    trackball.setViewportSize(QSize(800, 600));

    trackball.begin(400, 300);
    QQuaternion rot1 = trackball.rotate(450, 300);

    // Continue rotation
    QQuaternion rot2 = trackball.rotate(500, 300);

    // Accumulated rotation should not be identity
    QQuaternion total = trackball.rotation();
    CHECK_FALSE(total.isIdentity());
}

TEST_CASE("Trackball - set rotation directly", "[render][trackball]") {
    Trackball trackball;

    QQuaternion customRot = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 45.0f);
    trackball.setRotation(customRot);

    QQuaternion retrieved = trackball.rotation();
    CHECK(retrieved == customRot);
}
