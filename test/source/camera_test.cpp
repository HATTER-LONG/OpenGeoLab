/**
 * @file camera_test.cpp
 * @brief Unit tests for camera system
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "render/camera.hpp"

using namespace OpenGeoLab::Render;
using Catch::Matchers::WithinRel;

TEST_CASE("Camera - initialization", "[render][camera]") {
    Camera camera;

    SECTION("Default values") {
        CHECK_THAT(camera.yaw(), WithinRel(0.0f, 0.001f));
        CHECK_THAT(camera.pitch(), WithinRel(30.0f, 0.001f));
        CHECK_THAT(camera.distance(), WithinRel(5.0f, 0.001f));
        CHECK_THAT(camera.fieldOfView(), WithinRel(45.0f, 0.001f));
    }

    SECTION("Target defaults to origin") {
        QVector3D target = camera.target();
        CHECK_THAT(target.x(), WithinRel(0.0f, 0.001f));
        CHECK_THAT(target.y(), WithinRel(0.0f, 0.001f));
        CHECK_THAT(target.z(), WithinRel(0.0f, 0.001f));
    }

    SECTION("Up vector defaults to Y-up") {
        QVector3D up = camera.upVector();
        CHECK_THAT(up.x(), WithinRel(0.0f, 0.001f));
        CHECK_THAT(up.y(), WithinRel(1.0f, 0.001f));
        CHECK_THAT(up.z(), WithinRel(0.0f, 0.001f));
    }
}

TEST_CASE("Camera - orbit operations", "[render][camera]") {
    Camera camera;

    SECTION("Orbit changes yaw and pitch") {
        camera.setOrbitAngles(0.0f, 0.0f);
        camera.orbit(45.0f, 15.0f);

        CHECK_THAT(camera.yaw(), WithinRel(45.0f, 0.001f));
        CHECK_THAT(camera.pitch(), WithinRel(15.0f, 0.001f));
    }

    SECTION("Pitch is clamped") {
        camera.setOrbitAngles(0.0f, 0.0f);
        camera.orbit(0.0f, 100.0f);

        CHECK(camera.pitch() < 90.0f);
        CHECK(camera.pitch() > -90.0f);
    }

    SECTION("Yaw wraps around") {
        camera.setOrbitAngles(350.0f, 0.0f);
        camera.orbit(20.0f, 0.0f);

        CHECK_THAT(camera.yaw(), WithinRel(10.0f, 0.001f));
    }
}

TEST_CASE("Camera - zoom operations", "[render][camera]") {
    Camera camera;

    SECTION("Zoom in reduces distance") {
        float initialDistance = camera.distance();
        camera.zoom(2.0f);

        CHECK(camera.distance() < initialDistance);
    }

    SECTION("Zoom out increases distance") {
        float initialDistance = camera.distance();
        camera.zoom(0.5f);

        CHECK(camera.distance() > initialDistance);
    }

    SECTION("Distance is clamped") {
        // Try to zoom in too much
        for(int i = 0; i < 100; ++i) {
            camera.zoom(10.0f);
        }
        CHECK(camera.distance() > 0.0f);

        // Try to zoom out too much
        camera.reset();
        for(int i = 0; i < 100; ++i) {
            camera.zoom(0.01f);
        }
        CHECK(camera.distance() < 1000000.0f);
    }
}

TEST_CASE("Camera - pan operations", "[render][camera]") {
    Camera camera;

    SECTION("Pan changes target") {
        QVector3D initialTarget = camera.target();
        camera.pan(100.0f, 0.0f);

        QVector3D newTarget = camera.target();
        CHECK(newTarget != initialTarget);
    }
}

TEST_CASE("Camera - fit to bounds", "[render][camera]") {
    Camera camera;

    SECTION("Fit adjusts distance and target") {
        QVector3D minPt(-10, -10, -10);
        QVector3D maxPt(10, 10, 10);

        camera.fitToBounds(minPt, maxPt);

        QVector3D target = camera.target();
        CHECK_THAT(target.x(), WithinRel(0.0f, 0.001f));
        CHECK_THAT(target.y(), WithinRel(0.0f, 0.001f));
        CHECK_THAT(target.z(), WithinRel(0.0f, 0.001f));

        // Distance should be large enough to see the box
        CHECK(camera.distance() > 10.0f);
    }
}

TEST_CASE("Camera - reset", "[render][camera]") {
    Camera camera;

    // Change values
    camera.orbit(45.0f, 20.0f);
    camera.zoom(2.0f);
    camera.setTarget(QVector3D(10, 10, 10));

    // Reset
    camera.reset();

    CHECK_THAT(camera.yaw(), WithinRel(45.0f, 0.001f));
    CHECK_THAT(camera.pitch(), WithinRel(30.0f, 0.001f));
    CHECK_THAT(camera.distance(), WithinRel(5.0f, 0.001f));
}

TEST_CASE("Camera - matrices", "[render][camera]") {
    Camera camera;

    SECTION("View matrix is not identity") {
        QMatrix4x4 view = camera.viewMatrix();
        QMatrix4x4 identity;

        CHECK(view != identity);
    }

    SECTION("Projection matrix is valid") {
        QMatrix4x4 proj = camera.projectionMatrix(16.0f / 9.0f);

        // Check that it's a valid perspective matrix
        CHECK(proj(3, 3) == 0.0f); // Perspective division indicator
    }

    SECTION("VP matrix combines view and projection") {
        QMatrix4x4 vp = camera.viewProjectionMatrix(1.0f);
        QMatrix4x4 view = camera.viewMatrix();
        QMatrix4x4 proj = camera.projectionMatrix(1.0f);

        QMatrix4x4 expected = proj * view;

        // Check a few elements
        CHECK_THAT(vp(0, 0), WithinRel(expected(0, 0), 0.001f));
        CHECK_THAT(vp(1, 1), WithinRel(expected(1, 1), 0.001f));
    }
}
