/**
 * @file label_anchor_test.cpp
 * @brief Unit tests for label anchor computation from vertex data
 */

#include <doctest/doctest.h>

#include <opengeolab/render/label_anchor.hpp>

using OpenGeoLab::Render::computeAnchorFromVertices;
using OpenGeoLab::Render::computeStackIndices;

TEST_CASE("computeAnchorFromVertices: single vertex returns that vertex") {
    std::vector<glm::vec3> positions = {{1.0F, 2.0F, 3.0F}};
    auto anchor = computeAnchorFromVertices(positions);
    CHECK(anchor.x == doctest::Approx(1.0F));
    CHECK(anchor.y == doctest::Approx(2.0F));
    CHECK(anchor.z == doctest::Approx(3.0F));
}

TEST_CASE("computeAnchorFromVertices: two vertices returns midpoint") {
    std::vector<glm::vec3> positions = {{0.0F, 0.0F, 0.0F}, {2.0F, 4.0F, 6.0F}};
    auto anchor = computeAnchorFromVertices(positions);
    CHECK(anchor.x == doctest::Approx(1.0F));
    CHECK(anchor.y == doctest::Approx(2.0F));
    CHECK(anchor.z == doctest::Approx(3.0F));
}

TEST_CASE("computeAnchorFromVertices: many vertices returns centroid") {
    std::vector<glm::vec3> positions = {
        {0.0F, 0.0F, 0.0F},
        {4.0F, 0.0F, 0.0F},
        {4.0F, 4.0F, 0.0F},
        {0.0F, 4.0F, 0.0F},
    };
    auto anchor = computeAnchorFromVertices(positions);
    CHECK(anchor.x == doctest::Approx(2.0F));
    CHECK(anchor.y == doctest::Approx(2.0F));
    CHECK(anchor.z == doctest::Approx(0.0F));
}

TEST_CASE("computeAnchorFromVertices: empty input returns origin") {
    std::vector<glm::vec3> positions;
    auto anchor = computeAnchorFromVertices(positions);
    CHECK(anchor.x == doctest::Approx(0.0F));
    CHECK(anchor.y == doctest::Approx(0.0F));
    CHECK(anchor.z == doctest::Approx(0.0F));
}

TEST_CASE("computeStackIndices: non-overlapping labels get stackIndex 0") {
    std::vector<glm::vec2> screen_positions = {
        {100.0F, 100.0F},
        {300.0F, 300.0F},
    };
    auto indices = computeStackIndices(screen_positions, 4.0F);
    REQUIRE(indices.size() == 2);
    CHECK(indices[0] == 0);
    CHECK(indices[1] == 0);
}

TEST_CASE("computeStackIndices: overlapping labels get sequential indices") {
    std::vector<glm::vec2> screen_positions = {
        {100.0F, 100.0F},
        {102.0F, 101.0F}, // within 4px tolerance
        {101.0F, 99.0F},  // within 4px tolerance
    };
    auto indices = computeStackIndices(screen_positions, 4.0F);
    REQUIRE(indices.size() == 3);
    CHECK(indices[0] == 0);
    CHECK(indices[1] == 1);
    CHECK(indices[2] == 2);
}

TEST_CASE("computeStackIndices: separate groups get independent stacking") {
    std::vector<glm::vec2> screen_positions = {
        {100.0F, 100.0F}, // Group A
        {102.0F, 101.0F}, // Group A
        {500.0F, 500.0F}, // Group B (far away)
    };
    auto indices = computeStackIndices(screen_positions, 4.0F);
    REQUIRE(indices.size() == 3);
    CHECK(indices[0] == 0);
    CHECK(indices[1] == 1);
    CHECK(indices[2] == 0); // Group B starts at 0
}
