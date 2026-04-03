/**
 * @file font_atlas_test.cpp
 * @brief Unit tests for FontAtlas glyph metric parsing
 */

#include <doctest/doctest.h>

#include "font/font_atlas.hpp"

using OpenGeoLab::Render::FontAtlas;

TEST_CASE("FontAtlas: parseMetrics loads glyph data from JSON string") {
    // Minimal valid JSON with two glyphs (space and 'A')
    constexpr auto json = R"({
        "atlas": {"width": 64, "height": 64, "size": 32, "distanceRange": 4},
        "metrics": {
            "lineHeight": 1.2,
            "ascender": 0.95,
            "descender": -0.25
        },
        "glyphs": [
            {
                "unicode": 32,
                "advance": 0.5
            },
            {
                "unicode": 65,
                "advance": 0.6,
                "planeBounds": {"left": 0.01, "bottom": -0.01, "right": 0.59, "top": 0.95},
                "atlasBounds": {"left": 1, "bottom": 1, "right": 30, "top": 50}
            }
        ]
    })";

    FontAtlas atlas;
    REQUIRE(atlas.parseMetrics(json));

    SUBCASE("atlas dimensions are loaded") {
        CHECK(atlas.atlasSize().x == 64);
        CHECK(atlas.atlasSize().y == 64);
    }

    SUBCASE("font metrics are loaded") {
        CHECK(atlas.lineHeight() == doctest::Approx(1.2));
        CHECK(atlas.ascender() == doctest::Approx(0.95));
        CHECK(atlas.descender() == doctest::Approx(-0.25));
    }

    SUBCASE("glyph lookup returns valid metrics") {
        const auto* glyph_a = atlas.glyph(65); // 'A'
        REQUIRE(glyph_a != nullptr);
        CHECK(glyph_a->advance == doctest::Approx(0.6));
        CHECK(glyph_a->planeBounds[0] == doctest::Approx(0.01)); // left
        CHECK(glyph_a->planeBounds[3] == doctest::Approx(0.95)); // top
        CHECK(glyph_a->atlasBounds[0] == doctest::Approx(1.0));  // left
        CHECK(glyph_a->atlasBounds[3] == doctest::Approx(50.0)); // top
    }

    SUBCASE("space glyph has advance but no bounds") {
        const auto* space = atlas.glyph(32);
        REQUIRE(space != nullptr);
        CHECK(space->advance == doctest::Approx(0.5));
        // planeBounds and atlasBounds should be zeroed for whitespace
        CHECK(space->planeBounds[0] == doctest::Approx(0.0));
    }

    SUBCASE("missing glyph returns nullptr") {
        CHECK(atlas.glyph(9999) == nullptr);
    }
}

TEST_CASE("FontAtlas: parseMetrics rejects invalid JSON") {
    FontAtlas atlas;
    CHECK_FALSE(atlas.parseMetrics("not json"));
    CHECK_FALSE(atlas.parseMetrics(R"({"atlas": {}})"));
}

TEST_CASE("FontAtlas: pxRange returns distance range from atlas metadata") {
    constexpr auto json = R"({
        "atlas": {"width": 512, "height": 512, "size": 48, "distanceRange": 4},
        "metrics": {"lineHeight": 1.0, "ascender": 0.8, "descender": -0.2},
        "glyphs": []
    })";

    FontAtlas atlas;
    REQUIRE(atlas.parseMetrics(json));
    CHECK(atlas.pxRange() == doctest::Approx(4.0));
}
