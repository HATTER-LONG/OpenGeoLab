/**
 * @file color_map_test.cpp
 * @brief Tests for ColorMap configuration system
 */

#include <opengeolab/core/color_map.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::ColorMapConfig;
using OpenGeoLab::Core::HighlightStyle;
using OpenGeoLab::Core::RenderColor;

namespace ColorMap = OpenGeoLab::Core::ColorMap;

TEST_SUITE("ColorMap") {

    TEST_CASE("kDefault hover edge/vertex color is orange #ff7f00") {
        const auto& style = ColorMap::kDefault.hoverEdgeVertex;
        CHECK(style.color.r == doctest::Approx(1.0f));
        CHECK(style.color.g == doctest::Approx(0.498f).epsilon(0.01));
        CHECK(style.color.b == doctest::Approx(0.0f));
        CHECK(style.lineWidth == doctest::Approx(2.5f));
        CHECK(style.pointScale == doctest::Approx(1.5f));
    }

    TEST_CASE("kDefault selection edge/vertex color is red-pink #ff165d") {
        const auto& style = ColorMap::kDefault.selectionEdgeVertex;
        CHECK(style.color.r == doctest::Approx(1.0f));
        CHECK(style.color.g == doctest::Approx(0.086f).epsilon(0.01));
        CHECK(style.lineWidth == doctest::Approx(2.0f));
        CHECK(style.pointScale == doctest::Approx(1.2f));
    }

    TEST_CASE("kDefault hover face color is blue #4b55e9") {
        const auto& style = ColorMap::kDefault.hoverFace;
        CHECK(style.color.r == doctest::Approx(0.294f).epsilon(0.01));
        CHECK(style.color.g == doctest::Approx(0.333f).epsilon(0.01));
        CHECK(style.color.b == doctest::Approx(0.914f).epsilon(0.01));
    }

    TEST_CASE("kDefault selection face color is deep blue #4116ff") {
        const auto& style = ColorMap::kDefault.selectionFace;
        CHECK(style.color.r == doctest::Approx(0.255f).epsilon(0.01));
        CHECK(style.color.b == doctest::Approx(1.0f));
    }

    TEST_CASE("kDefault sizes") {
        CHECK(ColorMap::kDefault.defaultEdgeWidth == doctest::Approx(1.5f));
        CHECK(ColorMap::kDefault.defaultPointSize == doctest::Approx(6.0f));
    }

    TEST_CASE("active() returns kDefault initially") {
        const auto& cfg = ColorMap::active();
        CHECK(cfg.defaultEdgeWidth == doctest::Approx(ColorMap::kDefault.defaultEdgeWidth));
        CHECK(cfg.defaultPointSize == doctest::Approx(ColorMap::kDefault.defaultPointSize));
    }

    TEST_CASE("setOverride changes active()") {
        ColorMapConfig custom = ColorMap::kDefault;
        custom.defaultEdgeWidth = 5.0f;
        custom.defaultPointSize = 20.0f;
        ColorMap::setOverride(custom);

        CHECK(ColorMap::active().defaultEdgeWidth == doctest::Approx(5.0f));
        CHECK(ColorMap::active().defaultPointSize == doctest::Approx(20.0f));

        // Reset to default
        ColorMap::setOverride(ColorMap::kDefault);
        CHECK(ColorMap::active().defaultEdgeWidth == doctest::Approx(1.5f));
    }
}
