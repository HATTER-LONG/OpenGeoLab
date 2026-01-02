/**
 * @file render_data_test.cpp
 * @brief Unit tests for render data structures
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "render/render_data.hpp"

using namespace OpenGeoLab::Render;
using Catch::Matchers::WithinRel;

TEST_CASE("RenderGeometry - basic operations", "[render][geometry]") {
    RenderGeometry geometry;

    SECTION("isEmpty returns true for empty geometry") {
        CHECK(geometry.isEmpty());
        CHECK(geometry.triangleCount() == 0);
    }

    SECTION("Adding vertices and indices") {
        geometry.vertices.push_back(
            RenderVertex(QVector3D(0, 0, 0), QVector3D(0, 0, 1), QVector3D(1, 0, 0)));
        geometry.vertices.push_back(
            RenderVertex(QVector3D(1, 0, 0), QVector3D(0, 0, 1), QVector3D(0, 1, 0)));
        geometry.vertices.push_back(
            RenderVertex(QVector3D(0, 1, 0), QVector3D(0, 0, 1), QVector3D(0, 0, 1)));

        geometry.indices = {0, 1, 2};

        CHECK_FALSE(geometry.isEmpty());
        CHECK(geometry.triangleCount() == 1);
    }

    SECTION("Bounding box calculation") {
        geometry.vertices.push_back(
            RenderVertex(QVector3D(-1, -2, -3), QVector3D(0, 0, 1), QVector3D(1, 0, 0)));
        geometry.vertices.push_back(
            RenderVertex(QVector3D(4, 5, 6), QVector3D(0, 0, 1), QVector3D(0, 1, 0)));
        geometry.vertices.push_back(
            RenderVertex(QVector3D(1, 1, 1), QVector3D(0, 0, 1), QVector3D(0, 0, 1)));

        QVector3D minPt = geometry.boundingBoxMin();
        QVector3D maxPt = geometry.boundingBoxMax();
        QVector3D center = geometry.center();

        CHECK_THAT(minPt.x(), WithinRel(-1.0f, 0.001f));
        CHECK_THAT(minPt.y(), WithinRel(-2.0f, 0.001f));
        CHECK_THAT(minPt.z(), WithinRel(-3.0f, 0.001f));

        CHECK_THAT(maxPt.x(), WithinRel(4.0f, 0.001f));
        CHECK_THAT(maxPt.y(), WithinRel(5.0f, 0.001f));
        CHECK_THAT(maxPt.z(), WithinRel(6.0f, 0.001f));

        CHECK_THAT(center.x(), WithinRel(1.5f, 0.001f));
        CHECK_THAT(center.y(), WithinRel(1.5f, 0.001f));
        CHECK_THAT(center.z(), WithinRel(1.5f, 0.001f));
    }

    SECTION("Clear operation") {
        geometry.vertices.push_back(
            RenderVertex(QVector3D(0, 0, 0), QVector3D(0, 0, 1), QVector3D(1, 0, 0)));
        geometry.indices = {0};

        CHECK_FALSE(geometry.isEmpty());

        geometry.clear();

        CHECK(geometry.isEmpty());
        CHECK(geometry.vertices.empty());
        CHECK(geometry.indices.empty());
    }
}

TEST_CASE("RenderVertex - construction", "[render][vertex]") {
    SECTION("Default construction") {
        RenderVertex v;
        CHECK(v.position == QVector3D(0, 0, 0));
        CHECK(v.normal == QVector3D(0, 0, 0));
        CHECK(v.color == QVector3D(0, 0, 0));
    }

    SECTION("Parameterized construction") {
        RenderVertex v(QVector3D(1, 2, 3), QVector3D(0, 1, 0), QVector3D(0.5f, 0.5f, 0.5f));
        CHECK(v.position == QVector3D(1, 2, 3));
        CHECK(v.normal == QVector3D(0, 1, 0));
        CHECK(v.color == QVector3D(0.5f, 0.5f, 0.5f));
    }
}
