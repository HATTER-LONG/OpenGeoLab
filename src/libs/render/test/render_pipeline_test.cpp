/**
 * @file render_pipeline_test.cpp
 * @brief Unit tests for render pipeline internals
 */

#include "render_pipeline_detail.hpp"

#include <opengeolab/render/pick_mask.hpp>

#include <doctest/doctest.h>

namespace OpenGeoLab::Render::Detail {

TEST_CASE("RenderPipeline maps part pick masks to part mode") {
    CHECK(pickModeFromMask(PickMask::Part) == PickMode::Part);
    CHECK(pickModeFromMask(PickMask::All) == PickMode::Part);
}

TEST_CASE("RenderPipeline maps solid and wire masks before VEF masks") {
    CHECK(pickModeFromMask(PickMask::Solid) == PickMode::Solid);
    CHECK(pickModeFromMask(PickMask::Wire) == PickMode::Wire);
    CHECK(pickModeFromMask(PickMask::Solid | PickMask::Vertex) == PickMode::Solid);
    CHECK(pickModeFromMask(PickMask::Wire | PickMask::Face) == PickMode::Wire);
}

TEST_CASE("RenderPipeline defaults vertex edge face masks to VEF mode") {
    CHECK(pickModeFromMask(PickMask::None) == PickMode::VEF);
    CHECK(pickModeFromMask(PickMask::Vertex) == PickMode::VEF);
    CHECK(pickModeFromMask(PickMask::Edge) == PickMode::VEF);
    CHECK(pickModeFromMask(PickMask::Face) == PickMode::VEF);
    CHECK(pickModeFromMask(PickMask::Vertex | PickMask::Edge | PickMask::Face) == PickMode::VEF);
}

} // namespace OpenGeoLab::Render::Detail
