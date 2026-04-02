# MSDF Label Rendering Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Render MSDF billboard labels (V:1, E:3, F:6, S:1) on selected entities in the 3D viewport, with a `describe_labels` action for LLM observability.

**Architecture:** New `FontAtlas` loads a pre-generated MSDF texture + JSON metrics. New `LabelPass` renders billboard quads using MSDF fragment shading with depth-aware occlusion. `GLViewportRenderer::synchronize()` resolves labels from `LabelManager` to 3D anchor positions via `GpuBufferManager::lookupEntity()`. `SelectionService` auto-creates labels on entity selection. `describe_labels` scene action exposes visual encoding to LLMs.

**Tech Stack:** C++20, OpenGL 3.3, MSDF font rendering, stb_image (PNG decoding), nlohmann::json, doctest, Qt 6 / QML

**Spec:** `docs/superpowers/specs/2026-04-03-msdf-label-rendering-design.md`

---

## File Map

### New Files (render layer)
| File | Responsibility |
|------|---------------|
| `src/libs/render/src/font/font_atlas.hpp` | MSDF atlas loader: texture + glyph metrics |
| `src/libs/render/src/font/font_atlas.cpp` | JSON parsing, GL texture creation, glyph lookup |
| `src/libs/render/src/pass/label_pass.hpp` | LabelPass class (RenderPassBase) |
| `src/libs/render/src/pass/label_pass.cpp` | MSDF shader, billboard quads, anchor computation, stacking |
| `src/libs/render/src/label_anchor.hpp` | Free functions for anchor computation from DrawRanges + VBO |
| `src/libs/render/src/label_anchor.cpp` | Anchor computation implementation |
| `src/libs/render/resource/fonts/label_atlas.png` | Pre-generated 512×512 MSDF texture |
| `src/libs/render/resource/fonts/label_atlas.json` | Glyph metrics JSON |
| `src/libs/render/test/font_atlas_test.cpp` | FontAtlas JSON parsing tests |
| `src/libs/render/test/label_anchor_test.cpp` | Anchor computation unit tests |

### New Files (scene layer)
| File | Responsibility |
|------|---------------|
| `src/libs/scene/include/opengeolab/scene/describe_labels_action.hpp` | Action header |
| `src/libs/scene/src/describe_labels_action.cpp` | describe_labels implementation |
| `src/libs/scene/test/describe_labels_action_test.cpp` | Action unit tests |

### New Files (core layer)
| File | Responsibility |
|------|---------------|
| `src/libs/core/include/opengeolab/core/label_colors.hpp` | Shared label color constants |

### Modified Files
| File | Change |
|------|--------|
| `CMakeLists.txt` | Add stb_image CPM dependency |
| `src/libs/render/CMakeLists.txt` | Add new sources, stb_image link, resource copy, test files |
| `src/libs/render/include/opengeolab/render/frame_state.hpp` | Add `ResolvedLabel` struct + label fields |
| `src/libs/render/src/render_pipeline.cpp` | Add FontAtlas + LabelPass to Impl, init/render/cleanup |
| `src/libs/scene/CMakeLists.txt` | Add describe_labels source + header + test |
| `src/libs/scene/src/scene_module.cpp` | Register DescribeLabelsAction |
| `src/app/src/gl_viewport_renderer.cpp` | Add label resolution in synchronize() |
| `src/app/include/opengeolab/app/selection_service.hpp` | Add label management Q_INVOKABLE methods |
| `src/app/src/selection_service.cpp` | Implement label methods + auto-label logic |
| `src/app/resource/qml/components/pages/GeoQueryPage.qml` | Auto-label toggle, clear on close |

---

### Task 1: Add stb_image dependency

**Files:**
- Modify: `CMakeLists.txt:74` (after GLM version line)
- Modify: `src/libs/render/CMakeLists.txt`

- [ ] **Step 1: Add stb version variable to root CMakeLists.txt**

In `CMakeLists.txt`, after the `OPENGEOLAB_GLAD_VERSION` line (line 73), add:

```cmake
set(OPENGEOLAB_STB_VERSION master)
```

- [ ] **Step 2: Add stb CPM resolution to root CMakeLists.txt**

In `CMakeLists.txt`, after the glad `opengeolab_resolve_package` block, add:

```cmake
CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG ${OPENGEOLAB_STB_VERSION}
    DOWNLOAD_ONLY YES)

if (stb_ADDED)
    add_library(stb_image INTERFACE)
    target_include_directories(stb_image INTERFACE "${stb_SOURCE_DIR}")
endif ()
```

- [ ] **Step 3: Link stb_image to render library**

In `src/libs/render/CMakeLists.txt`, add `stb_image` to the `PUBLIC_LINKS` section after `glm::glm`:

```cmake
opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME
    Render
    SOURCES
    ${render_sources}
    PUBLIC_HEADERS
    ${render_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::Scene
    glad::glad
    glm::glm
    stb_image)
```

- [ ] **Step 4: Build to verify dependency resolution**

Run:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4
```
Expected: Build succeeds with stb_image INTERFACE target resolved.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/libs/render/CMakeLists.txt
git commit -m "build(render): add stb_image dependency for MSDF atlas loading

stb_image is a single-header PNG decoder used by FontAtlas to load
the pre-generated MSDF atlas texture. Added as CPM download-only
dependency with an INTERFACE target.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: Label color constants

**Files:**
- Create: `src/libs/core/include/opengeolab/core/label_colors.hpp`

- [ ] **Step 1: Create label_colors.hpp**

```cpp
/**
 * @file label_colors.hpp
 * @brief Shared label color constants for entity-type-based label coloring.
 *
 * Used by LabelPass (render), describe_labels action (scene), and
 * SelectionService (app) to maintain consistent visual encoding.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <glm/vec4.hpp>

#include <string_view>

namespace OpenGeoLab::Core {

/// Label text color per entity type.
inline constexpr glm::vec4 labelColor(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return {0.91F, 0.33F, 0.33F, 1.0F}; // #E85454 Red
    case EntityType::GeoEdge:
        return {0.29F, 0.56F, 0.85F, 1.0F}; // #4A90D9 Blue
    case EntityType::GeoFace:
        return {0.36F, 0.72F, 0.36F, 1.0F}; // #5CB85C Green
    case EntityType::GeoSolid:
        return {0.91F, 0.65F, 0.33F, 1.0F}; // #E8A654 Orange
    default:
        return {0.8F, 0.8F, 0.8F, 1.0F}; // Gray fallback
    }
}

/// Hex color string per entity type (for JSON describe output).
inline constexpr std::string_view labelColorHex(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return "#E85454";
    case EntityType::GeoEdge:
        return "#4A90D9";
    case EntityType::GeoFace:
        return "#5CB85C";
    case EntityType::GeoSolid:
        return "#E8A654";
    default:
        return "#CCCCCC";
    }
}

/// Label text prefix per entity type.
inline constexpr std::string_view labelPrefix(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return "V";
    case EntityType::GeoEdge:
        return "E";
    case EntityType::GeoWire:
        return "W";
    case EntityType::GeoFace:
        return "F";
    case EntityType::GeoSolid:
        return "S";
    default:
        return "?";
    }
}

/// Default label background color (dark semi-transparent).
inline constexpr glm::vec4 K_LABEL_BG_COLOR{0.1F, 0.1F, 0.12F, 0.85F};

/// Alpha multiplier for occluded labels.
inline constexpr float K_LABEL_OCCLUDED_ALPHA = 0.3F;

} // namespace OpenGeoLab::Core
```

- [ ] **Step 2: Build to verify compilation**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_core --parallel 4
```
Expected: Build succeeds (header-only, no source changes needed in core CMake).

- [ ] **Step 3: Commit**

```bash
git add src/libs/core/include/opengeolab/core/label_colors.hpp
git commit -m "feat(core): add shared label color constants for entity-type coloring

V=Red #E85454, E=Blue #4A90D9, F=Green #5CB85C, S=Orange #E8A654.
Includes hex strings for JSON output and prefix strings for label text.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: Generate MSDF font atlas

**Files:**
- Create: `src/libs/render/resource/fonts/label_atlas.png`
- Create: `src/libs/render/resource/fonts/label_atlas.json`
- Create: `tools/generate_atlas.sh` (reference script)

This task requires the `msdf-atlas-gen` CLI tool. If not available, a fallback
test atlas can be created from any monospace bitmap font tool.

- [ ] **Step 1: Create the resource directory**

```bash
mkdir -p src/libs/render/resource/fonts
```

- [ ] **Step 2: Generate the atlas (requires msdf-atlas-gen)**

```bash
msdf-atlas-gen \
  -font /path/to/monospace-font.ttf \
  -charset ascii \
  -type msdf \
  -dimensions 512 512 \
  -pxrange 4 \
  -json src/libs/render/resource/fonts/label_atlas.json \
  -imageout src/libs/render/resource/fonts/label_atlas.png
```

If msdf-atlas-gen is not installed, use any tool that produces:
- A 512×512 MSDF PNG (3-channel)
- A JSON file with `atlas.width`, `atlas.height`, `metrics.lineHeight`,
  `metrics.ascender`, `metrics.descender`, and `glyphs[]` array where each
  glyph has `unicode`, `advance`, `planeBounds{left,bottom,right,top}`,
  `atlasBounds{left,bottom,right,top}`.

- [ ] **Step 3: Create reference generation script**

Create `tools/generate_atlas.sh`:
```bash
#!/bin/bash
# Generate MSDF font atlas for label rendering.
# Requires: msdf-atlas-gen (https://github.com/Chlumsky/msdf-atlas-gen)
# Usage: ./tools/generate_atlas.sh /path/to/font.ttf

FONT="${1:?Usage: $0 <font.ttf>}"
OUT_DIR="src/libs/render/resource/fonts"

msdf-atlas-gen \
  -font "$FONT" \
  -charset ascii \
  -type msdf \
  -dimensions 512 512 \
  -pxrange 4 \
  -json "${OUT_DIR}/label_atlas.json" \
  -imageout "${OUT_DIR}/label_atlas.png"

echo "Atlas generated in ${OUT_DIR}/"
```

- [ ] **Step 4: Add CMake resource copy rule**

In `src/libs/render/CMakeLists.txt`, after the `target_include_directories` line, add:

```cmake
# Copy MSDF atlas resources to build output for runtime loading.
set(RENDER_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(RENDER_RESOURCE_OUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/fonts")

file(GLOB FONT_RESOURCES "${RENDER_RESOURCE_DIR}/fonts/*")
if (FONT_RESOURCES)
    file(MAKE_DIRECTORY "${RENDER_RESOURCE_OUT}")
    foreach (res IN LISTS FONT_RESOURCES)
        get_filename_component(fname "${res}" NAME)
        configure_file("${res}" "${RENDER_RESOURCE_OUT}/${fname}" COPYONLY)
    endforeach ()
endif ()
```

- [ ] **Step 5: Build to verify resource copy**

Run:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4
```
Expected: Configure copies atlas files to `build/bin/resources/fonts/` (or equivalent output dir), build succeeds.

- [ ] **Step 6: Commit**

```bash
git add src/libs/render/resource/fonts/ tools/generate_atlas.sh src/libs/render/CMakeLists.txt
git commit -m "build(render): add MSDF font atlas resources and generation script

Pre-generated 512×512 MSDF atlas with ASCII glyphs for label rendering.
CMake copies atlas files to build output for runtime loading.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: FrameState extension with ResolvedLabel

**Files:**
- Modify: `src/libs/render/include/opengeolab/render/frame_state.hpp`

- [ ] **Step 1: Add ResolvedLabel struct and label fields to FrameState**

In `frame_state.hpp`, add `#include <opengeolab/core/label_colors.hpp>` and `#include <string>` to the includes, then add after the `HighlightEntry` struct:

```cpp
/// A label resolved to a 3D world-space anchor, ready for LabelPass rendering.
struct ResolvedLabel {
    glm::vec3 anchorWorld{};     ///< 3D world-space anchor point
    std::string text;            ///< Display text ("F:3", "V:1")
    glm::vec4 textColor{};      ///< Entity-type color
    glm::vec4 bgColor{};        ///< Background color (with alpha)
    Core::EntityType entityType{Core::EntityType::GeoFace};
    uint32_t stackIndex{0};     ///< Vertical offset for overlapping anchors
    bool occluded{false};        ///< True if anchor is behind geometry
};
```

Add these fields to the `FrameState` struct after `activePickMask`:

```cpp
    /// Phase 2: Resolved labels for LabelPass rendering.
    std::vector<ResolvedLabel> resolvedLabels;

    /// Whether labels should be rendered.
    bool labelsVisible{true};
```

- [ ] **Step 2: Build to verify compilation**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Full build succeeds (FrameState is used across all layers).

- [ ] **Step 3: Commit**

```bash
git add src/libs/render/include/opengeolab/render/frame_state.hpp
git commit -m "feat(render): add ResolvedLabel struct and label fields to FrameState

ResolvedLabel carries anchor position, text, colors, occlusion state,
and stacking index. FrameState gains resolvedLabels vector and
labelsVisible flag for LabelPass consumption.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: FontAtlas — TDD

> **Spec deviation:** The spec shows a single `initialize(imagePath, jsonPath)` method.
> This plan uses a two-phase design (`parseMetrics(jsonString)` + `loadTexture(pngPath)`)
> so that JSON parsing can be unit-tested without a GL context. This is intentional.

**Files:**
- Create: `src/libs/render/src/font/font_atlas.hpp`
- Create: `src/libs/render/src/font/font_atlas.cpp`
- Create: `src/libs/render/test/font_atlas_test.cpp`
- Modify: `src/libs/render/CMakeLists.txt`

- [ ] **Step 1: Write failing tests for FontAtlas JSON parsing**

Create `src/libs/render/test/font_atlas_test.cpp`:

```cpp
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
```

- [ ] **Step 2: Create FontAtlas header**

Create `src/libs/render/src/font/font_atlas.hpp`:

```cpp
/**
 * @file font_atlas.hpp
 * @brief MSDF font atlas loader — texture + glyph metrics.
 */

#pragma once

#include <glad/gl.h>
#include <glm/vec2.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace OpenGeoLab::Render {

/// Per-glyph metrics loaded from the atlas JSON.
struct GlyphMetrics {
    float advance{};            ///< Horizontal advance in EM units
    float planeBounds[4]{};     ///< left, bottom, right, top in EM space
    float atlasBounds[4]{};     ///< left, bottom, right, top in atlas pixels
};

/**
 * @brief Loads a pre-generated MSDF font atlas for label rendering.
 *
 * Two-phase initialization:
 *   1. parseMetrics(jsonString) — parse glyph metrics (no GL required).
 *   2. loadTexture(pngPath) — create GL texture (requires valid GL context).
 *
 * parseMetrics can be called independently for unit testing without GL.
 */
class FontAtlas final {
public:
    /// Parse glyph metrics from a JSON string. No GL context needed.
    /// @return true on success.
    bool parseMetrics(std::string_view json_string);

    /// Load the MSDF atlas PNG and create a GL_RGB8 texture.
    /// Requires a valid GL context. Call after parseMetrics().
    /// @return true on success.
    bool loadTexture(const std::string& png_path);

    /// Release the GL texture. Safe to call if not initialized.
    void cleanup();

    /// Bind the atlas texture to the given texture unit.
    void bind(GLuint texture_unit = 0) const;

    /// Look up glyph metrics by Unicode code point.
    /// @return nullptr if the glyph is not in the atlas.
    [[nodiscard]] const GlyphMetrics* glyph(uint32_t code_point) const;

    [[nodiscard]] glm::ivec2 atlasSize() const noexcept { return m_atlasSize; }
    [[nodiscard]] float lineHeight() const noexcept { return m_lineHeight; }
    [[nodiscard]] float ascender() const noexcept { return m_ascender; }
    [[nodiscard]] float descender() const noexcept { return m_descender; }
    [[nodiscard]] float pxRange() const noexcept { return m_pxRange; }
    [[nodiscard]] GLuint textureId() const noexcept { return m_texture; }

private:
    GLuint m_texture{0};
    glm::ivec2 m_atlasSize{};
    float m_lineHeight{};
    float m_ascender{};
    float m_descender{};
    float m_pxRange{4.0F};
    std::unordered_map<uint32_t, GlyphMetrics> m_glyphs;
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Create FontAtlas implementation**

Create `src/libs/render/src/font/font_atlas.cpp`:

```cpp
/**
 * @file font_atlas.cpp
 * @brief FontAtlas implementation — JSON parsing + GL texture loading
 */

#include "font/font_atlas.hpp"

#include <nlohmann/json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include <fstream>
#include <sstream>

namespace OpenGeoLab::Render {

bool FontAtlas::parseMetrics(std::string_view json_string) {
    try {
        auto root = nlohmann::json::parse(json_string);

        // Atlas metadata
        const auto& atlas = root.at("atlas");
        m_atlasSize.x = atlas.at("width").get<int>();
        m_atlasSize.y = atlas.at("height").get<int>();
        if(atlas.contains("distanceRange")) {
            m_pxRange = atlas["distanceRange"].get<float>();
        }

        // Font metrics
        const auto& metrics = root.at("metrics");
        m_lineHeight = metrics.at("lineHeight").get<float>();
        m_ascender = metrics.at("ascender").get<float>();
        m_descender = metrics.at("descender").get<float>();

        // Glyphs
        m_glyphs.clear();
        for(const auto& g : root.at("glyphs")) {
            GlyphMetrics gm{};
            auto code_point = g.at("unicode").get<uint32_t>();
            gm.advance = g.at("advance").get<float>();

            if(g.contains("planeBounds")) {
                const auto& pb = g["planeBounds"];
                gm.planeBounds[0] = pb.at("left").get<float>();
                gm.planeBounds[1] = pb.at("bottom").get<float>();
                gm.planeBounds[2] = pb.at("right").get<float>();
                gm.planeBounds[3] = pb.at("top").get<float>();
            }

            if(g.contains("atlasBounds")) {
                const auto& ab = g["atlasBounds"];
                gm.atlasBounds[0] = ab.at("left").get<float>();
                gm.atlasBounds[1] = ab.at("bottom").get<float>();
                gm.atlasBounds[2] = ab.at("right").get<float>();
                gm.atlasBounds[3] = ab.at("top").get<float>();
            }

            m_glyphs[code_point] = gm;
        }

        return true;
    } catch(const nlohmann::json::exception&) {
        return false;
    }
}

bool FontAtlas::loadTexture(const std::string& png_path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    auto* data = stbi_load(png_path.c_str(), &width, &height, &channels, 3);
    if(data == nullptr) {
        return false;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return true;
}

void FontAtlas::cleanup() {
    if(m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
}

void FontAtlas::bind(GLuint texture_unit) const {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

const GlyphMetrics* FontAtlas::glyph(uint32_t code_point) const {
    auto it = m_glyphs.find(code_point);
    return it != m_glyphs.end() ? &it->second : nullptr;
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 4: Add new source files to CMakeLists.txt**

In `src/libs/render/CMakeLists.txt`, add to `render_sources`:
```cmake
    src/font/font_atlas.cpp
```

Add to test `SOURCES`:
```cmake
        test/font_atlas_test.cpp
```

Also add nlohmann_json and stb_image as `PRIVATE` link to the render library (nlohmann_json is an implementation detail):
Add to the `opengeolab_add_module` block, after `PUBLIC_LINKS`:
```cmake
    PRIVATE_LINKS
    nlohmann_json::nlohmann_json
```

- [ ] **Step 5: Run test to verify it fails then passes**

Run:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target opengeolab_render_test --parallel 4
ctest --test-dir build -C RelWithDebInfo -R render_test --output-on-failure
```
Expected: All font_atlas tests PASS.

- [ ] **Step 6: Commit**

```bash
git add src/libs/render/src/font/ src/libs/render/test/font_atlas_test.cpp src/libs/render/CMakeLists.txt
git commit -m "feat(render): add FontAtlas — MSDF atlas JSON parser and texture loader

Two-phase init: parseMetrics(jsonString) for unit-testable parsing,
loadTexture(pngPath) for GL texture creation. Uses stb_image for PNG
decoding and nlohmann::json for glyph metrics parsing.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: Label anchor computation — TDD

**Files:**
- Create: `src/libs/render/src/label_anchor.hpp`
- Create: `src/libs/render/src/label_anchor.cpp`
- Create: `src/libs/render/test/label_anchor_test.cpp`
- Modify: `src/libs/render/CMakeLists.txt`

- [ ] **Step 1: Write failing tests for anchor computation**

Create `src/libs/render/test/label_anchor_test.cpp`:

```cpp
/**
 * @file label_anchor_test.cpp
 * @brief Unit tests for label anchor computation from vertex data
 */

#include <doctest/doctest.h>

#include "label_anchor.hpp"

#include <glm/glm.hpp>

#include <vector>

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
```

- [ ] **Step 2: Run tests to verify they fail**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_render_test --parallel 4
```
Expected: FAIL — `label_anchor.hpp` not found.

- [ ] **Step 3: Create label_anchor.hpp**

```cpp
/**
 * @file label_anchor.hpp
 * @brief Free functions for computing label anchor positions from vertex data.
 */

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace OpenGeoLab::Render {

/// Compute the centroid of a set of vertex positions.
/// Returns origin if positions is empty.
[[nodiscard]] glm::vec3 computeAnchorFromVertices(std::span<const glm::vec3> positions);

/// Assign stack indices for overlapping labels.
/// Labels whose screen positions are within @p tolerance pixels of each other
/// are grouped and assigned sequential stackIndex values (0, 1, 2, ...).
/// @return Vector of stack indices, one per input position.
[[nodiscard]] std::vector<uint32_t>
computeStackIndices(std::span<const glm::vec2> screen_positions, float tolerance);

} // namespace OpenGeoLab::Render
```

- [ ] **Step 4: Create label_anchor.cpp**

```cpp
/**
 * @file label_anchor.cpp
 * @brief Label anchor computation and stacking logic
 */

#include "label_anchor.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <numeric>

namespace OpenGeoLab::Render {

glm::vec3 computeAnchorFromVertices(std::span<const glm::vec3> positions) {
    if(positions.empty()) {
        return glm::vec3{0.0F};
    }
    auto sum =
        std::accumulate(positions.begin(), positions.end(), glm::vec3{0.0F});
    return sum / static_cast<float>(positions.size());
}

std::vector<uint32_t>
computeStackIndices(std::span<const glm::vec2> screen_positions, float tolerance) {
    const auto count = screen_positions.size();
    std::vector<uint32_t> indices(count, 0);
    if(count <= 1) {
        return indices;
    }

    // For each label, count how many earlier labels overlap with it.
    const float tol2 = tolerance * tolerance;
    for(std::size_t i = 1; i < count; ++i) {
        uint32_t stack = 0;
        for(std::size_t j = 0; j < i; ++j) {
            auto diff = screen_positions[i] - screen_positions[j];
            if(glm::dot(diff, diff) <= tol2) {
                stack = std::max(stack, indices[j] + 1);
            }
        }
        indices[i] = stack;
    }
    return indices;
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 5: Add to CMakeLists.txt**

In `src/libs/render/CMakeLists.txt`, add to `render_sources`:
```cmake
    src/label_anchor.cpp
```

Add to test `SOURCES`:
```cmake
        test/label_anchor_test.cpp
```

- [ ] **Step 6: Run tests**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_render_test --parallel 4
ctest --test-dir build -C RelWithDebInfo -R render_test --output-on-failure
```
Expected: All label_anchor tests PASS.

- [ ] **Step 7: Commit**

```bash
git add src/libs/render/src/label_anchor.hpp src/libs/render/src/label_anchor.cpp \
        src/libs/render/test/label_anchor_test.cpp src/libs/render/CMakeLists.txt
git commit -m "feat(render): add label anchor computation and stacking utilities

computeAnchorFromVertices computes centroid from vertex positions.
computeStackIndices groups overlapping screen-space labels and assigns
sequential stack offsets for vertical stacking.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: LabelPass — MSDF rendering

**Files:**
- Create: `src/libs/render/src/pass/label_pass.hpp`
- Create: `src/libs/render/src/pass/label_pass.cpp`
- Modify: `src/libs/render/CMakeLists.txt`

- [ ] **Step 1: Create LabelPass header**

Create `src/libs/render/src/pass/label_pass.hpp`:

```cpp
/**
 * @file label_pass.hpp
 * @brief LabelPass — billboard MSDF entity label rendering
 */

#pragma once

#include "render_pass_base.hpp"
#include "core/shader_program.hpp"

#include <glad/gl.h>

#include <cstdint>

namespace OpenGeoLab::Render {

class FontAtlas;

/**
 * @brief Renders billboard MSDF labels attached to selected entities.
 *
 * Labels are pre-resolved to world-space anchors in FrameState::resolvedLabels.
 * This pass projects anchors to screen space, generates billboard quads,
 * and renders them with MSDF fragment shading.
 *
 * Depth: reads depth buffer (for occlusion feedback), does not write.
 * Blending: standard alpha blending for background transparency.
 */
class LabelPass final : public RenderPassBase {
public:
    /// Set the font atlas (must be called before initialize).
    void setFontAtlas(FontAtlas* atlas) { m_fontAtlas = atlas; }

    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    /// Per-vertex data for a label quad (background or glyph).
    struct LabelVertex {
        float pos[2];            ///< Screen-space position in pixels
        float texCoord[2];       ///< Atlas UV (0,0 for background)
        float color[4];          ///< RGBA color
        float isMsdf;            ///< 1.0 for glyph, 0.0 for background/pointer
        float occlusionAlpha;    ///< 1.0 visible, 0.3 occluded
    };

    void buildLabelGeometry(const FrameState& state);

    ShaderProgram m_shader;
    FontAtlas* m_fontAtlas{nullptr};
    GLuint m_vao{0};
    GLuint m_vbo{0};
    std::vector<LabelVertex> m_vertices;
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Create LabelPass implementation**

Create `src/libs/render/src/pass/label_pass.cpp`:

```cpp
/**
 * @file label_pass.cpp
 * @brief LabelPass implementation — MSDF billboard label rendering
 */

#include "pass/label_pass.hpp"

#include "font/font_atlas.hpp"

#include <opengeolab/core/label_colors.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view LABEL_VS = R"glsl(
#version 330 core

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec4 a_color;
layout(location = 3) in float a_isMsdf;
layout(location = 4) in float a_occlusionAlpha;

uniform vec2 u_viewportSize;

out vec2 v_texCoord;
out vec4 v_color;
out float v_isMsdf;

void main() {
    vec2 ndc = (a_pos / u_viewportSize) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_texCoord = a_texCoord;
    v_color = vec4(a_color.rgb, a_color.a * a_occlusionAlpha);
    v_isMsdf = a_isMsdf;
}
)glsl";

constexpr std::string_view LABEL_FS = R"glsl(
#version 330 core

uniform sampler2D u_atlas;
uniform float u_pxRange;

in vec2 v_texCoord;
in vec4 v_color;
in float v_isMsdf;

out vec4 fragColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    if (v_isMsdf > 0.5) {
        vec3 msd = texture(u_atlas, v_texCoord).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        vec2 unitRange = u_pxRange / vec2(textureSize(u_atlas, 0));
        vec2 screenTexSize = 1.0 / fwidth(v_texCoord);
        float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);
        float opacity = smoothstep(0.5 - 1.0 / screenPxRange,
                                   0.5 + 1.0 / screenPxRange, sd);
        fragColor = vec4(v_color.rgb, v_color.a * opacity);
    } else {
        fragColor = v_color;
    }
}
)glsl";

// Billboard sizing constants
constexpr float K_FONT_SCALE = 24.0F;   ///< Base font size in pixels
constexpr float K_PAD_H = 4.0F;         ///< Horizontal padding
constexpr float K_PAD_V = 2.0F;         ///< Vertical padding
constexpr float K_POINTER_HEIGHT = 6.0F; ///< Pointer triangle height
constexpr float K_POINTER_HALF_W = 4.0F; ///< Pointer triangle half-width
constexpr float K_STACK_GAP = 4.0F;     ///< Gap between stacked labels
constexpr float K_MIN_PX = 12.0F;       ///< Minimum label pixel size
constexpr float K_MAX_PX = 48.0F;       ///< Maximum label pixel size

} // namespace

bool LabelPass::onInitialize() {
    if(!m_shader.create(LABEL_VS, LABEL_FS)) {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Layout: pos(2f) + texCoord(2f) + color(4f) + isMsdf(1f) + occlusionAlpha(1f) = 10 floats
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(LabelVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(4 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(9 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void LabelPass::onCleanup() {
    m_shader.destroy();
    if(m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if(m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}

void LabelPass::buildLabelGeometry(const FrameState& state) {
    m_vertices.clear();
    if(m_fontAtlas == nullptr) {
        return;
    }

    const auto vp_w = static_cast<float>(state.viewportWidth);
    const auto vp_h = static_cast<float>(state.viewportHeight);
    const auto mvp = state.projMatrix * state.viewMatrix;

    for(const auto& label : state.resolvedLabels) {
        // Project anchor to screen space
        auto clip = mvp * glm::vec4(label.anchorWorld, 1.0F);
        if(clip.w <= 0.0F) {
            continue; // Behind camera
        }
        auto ndc = glm::vec3(clip) / clip.w;
        float screen_x = (ndc.x * 0.5F + 0.5F) * vp_w;
        float screen_y = (ndc.y * 0.5F + 0.5F) * vp_h;

        // Compute text width from glyph advances
        float text_width = 0.0F;
        for(char ch : label.text) {
            const auto* gm = m_fontAtlas->glyph(static_cast<uint32_t>(ch));
            if(gm != nullptr) {
                text_width += gm->advance * K_FONT_SCALE;
            }
        }

        float text_height = m_fontAtlas->lineHeight() * K_FONT_SCALE;
        float bg_w = text_width + 2.0F * K_PAD_H;
        float bg_h = text_height + 2.0F * K_PAD_V;

        // Stack offset (upward)
        float stack_offset = static_cast<float>(label.stackIndex) *
                             (bg_h + K_POINTER_HEIGHT + K_STACK_GAP);

        // Label center (above anchor)
        float label_cx = screen_x;
        float label_cy = screen_y + K_POINTER_HEIGHT + bg_h * 0.5F + stack_offset;

        float occlusion_alpha = label.occluded ? Core::K_LABEL_OCCLUDED_ALPHA : 1.0F;

        // -- Background rectangle (two triangles) --
        float bg_left = label_cx - bg_w * 0.5F;
        float bg_right = label_cx + bg_w * 0.5F;
        float bg_bottom = label_cy - bg_h * 0.5F;
        float bg_top = label_cy + bg_h * 0.5F;

        auto bg = label.bgColor;
        auto push_bg = [&](float x, float y) {
            m_vertices.push_back(
                {{x, y}, {0.0F, 0.0F}, {bg.r, bg.g, bg.b, bg.a}, 0.0F, occlusion_alpha});
        };
        // Triangle 1
        push_bg(bg_left, bg_bottom);
        push_bg(bg_right, bg_bottom);
        push_bg(bg_right, bg_top);
        // Triangle 2
        push_bg(bg_left, bg_bottom);
        push_bg(bg_right, bg_top);
        push_bg(bg_left, bg_top);

        // -- Pointer triangle --
        float ptr_top = bg_bottom;
        float ptr_bottom = screen_y + stack_offset;
        push_bg(label_cx - K_POINTER_HALF_W, ptr_top);
        push_bg(label_cx + K_POINTER_HALF_W, ptr_top);
        push_bg(label_cx, ptr_bottom);

        // -- Glyph quads --
        float cursor_x = bg_left + K_PAD_H;
        float baseline_y = label_cy - text_height * 0.5F +
                           m_fontAtlas->ascender() * K_FONT_SCALE;

        auto atlas_w = static_cast<float>(m_fontAtlas->atlasSize().x);
        auto atlas_h = static_cast<float>(m_fontAtlas->atlasSize().y);
        auto tc = label.textColor;

        for(char ch : label.text) {
            const auto* gm = m_fontAtlas->glyph(static_cast<uint32_t>(ch));
            if(gm == nullptr) {
                continue;
            }

            // Skip whitespace glyphs (no atlas bounds)
            if(gm->atlasBounds[0] == 0.0F && gm->atlasBounds[2] == 0.0F) {
                cursor_x += gm->advance * K_FONT_SCALE;
                continue;
            }

            // Glyph screen-space quad
            float gx0 = cursor_x + gm->planeBounds[0] * K_FONT_SCALE;
            float gy0 = baseline_y - gm->planeBounds[3] * K_FONT_SCALE; // top
            float gx1 = cursor_x + gm->planeBounds[2] * K_FONT_SCALE;
            float gy1 = baseline_y - gm->planeBounds[1] * K_FONT_SCALE; // bottom

            // Atlas UVs (normalized)
            float u0 = gm->atlasBounds[0] / atlas_w;
            float v0 = gm->atlasBounds[1] / atlas_h;
            float u1 = gm->atlasBounds[2] / atlas_w;
            float v1 = gm->atlasBounds[3] / atlas_h;

            auto push_glyph = [&](float x, float y, float u, float v) {
                m_vertices.push_back(
                    {{x, y}, {u, v}, {tc.r, tc.g, tc.b, tc.a}, 1.0F, occlusion_alpha});
            };
            // Triangle 1
            push_glyph(gx0, gy0, u0, v1);
            push_glyph(gx1, gy0, u1, v1);
            push_glyph(gx1, gy1, u1, v0);
            // Triangle 2
            push_glyph(gx0, gy0, u0, v1);
            push_glyph(gx1, gy1, u1, v0);
            push_glyph(gx0, gy1, u0, v0);

            cursor_x += gm->advance * K_FONT_SCALE;
        }
    }
}

void LabelPass::render(const FrameState& state, const GpuBufferManager& /*buffers*/) {
    if(!isInitialized() || m_fontAtlas == nullptr) {
        return;
    }
    if(!state.labelsVisible || state.resolvedLabels.empty()) {
        return;
    }

    buildLabelGeometry(state);
    if(m_vertices.empty()) {
        return;
    }

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(LabelVertex)),
                 m_vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set GL state: depth read, no write, alpha blending
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.bind();
    m_shader.setVec2("u_viewportSize",
                     glm::vec2(static_cast<float>(state.viewportWidth),
                               static_cast<float>(state.viewportHeight)));
    m_shader.setFloat("u_pxRange", m_fontAtlas->pxRange());

    m_fontAtlas->bind(0);
    m_shader.setInt("u_atlas", 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);

    m_shader.unbind();

    // Restore GL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Add label_pass.cpp to CMakeLists.txt**

In `src/libs/render/CMakeLists.txt`, add to `render_sources`:
```cmake
    src/pass/label_pass.cpp
```

- [ ] **Step 4: Build to verify compilation**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4
```
Expected: Build succeeds.

- [ ] **Step 5: Commit**

```bash
git add src/libs/render/src/pass/label_pass.hpp src/libs/render/src/pass/label_pass.cpp \
        src/libs/render/CMakeLists.txt
git commit -m "feat(render): add LabelPass — MSDF billboard label rendering

Renders entity labels as screen-aligned billboards with MSDF fragment
shading. Features: background rectangles, pointer triangles, glyph quads,
per-label occlusion alpha, vertical label stacking.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: RenderPipeline integration

**Files:**
- Modify: `src/libs/render/src/render_pipeline.cpp`

- [ ] **Step 1: Add LabelPass and FontAtlas includes and Impl members**

In `render_pipeline.cpp`, add includes:
```cpp
#include "font/font_atlas.hpp"
#include "pass/label_pass.hpp"
```

Add to `struct RenderPipeline::Impl`:
```cpp
    FontAtlas fontAtlas;
    LabelPass labelPass;
    std::string fontAtlasDir; ///< Directory containing atlas resources
```

- [ ] **Step 2: Update initialize() to init FontAtlas and LabelPass**

After `m_impl->highlightPass.setThickLineRenderer(...)` (line 73), add:

```cpp
    // Phase 2: Label rendering
    {
        auto json_path = m_impl->fontAtlasDir + "/label_atlas.json";
        auto png_path = m_impl->fontAtlasDir + "/label_atlas.png";

        std::ifstream json_file(json_path);
        if(json_file.good()) {
            std::string json_str((std::istreambuf_iterator<char>(json_file)),
                                 std::istreambuf_iterator<char>());
            if(m_impl->fontAtlas.parseMetrics(json_str)) {
                m_impl->fontAtlas.loadTexture(png_path);
                m_impl->labelPass.setFontAtlas(&m_impl->fontAtlas);
                m_impl->labelPass.initialize();
            }
        }
    }
```

Add `#include <fstream>` to the includes.

- [ ] **Step 3: Add setFontAtlasDir() and resolveEntityAnchor() to RenderPipeline**

In `render_pipeline.hpp`, add public methods:
```cpp
    /// Set the directory containing MSDF atlas resources.
    /// Must be called before initialize().
    void setFontAtlasDir(const std::string& dir);

    /// Compute the world-space anchor (centroid) of an entity from CPU-side vertex data.
    /// Uses GpuBufferManager::lookupEntity() to find DrawRanges, then reads from
    /// the CPU-side vertex cache to compute the centroid.
    /// @return The centroid position, or origin if entity is not found.
    [[nodiscard]] glm::vec3 resolveEntityAnchor(uint32_t shape_id,
                                                 Core::EntityType entity_type,
                                                 uint32_t local_id) const;
```

Add includes in `render_pipeline.hpp`:
```cpp
#include <opengeolab/core/entity_tag.hpp>
#include <glm/vec3.hpp>
```

In `render_pipeline.cpp`, add implementations:
```cpp
void RenderPipeline::setFontAtlasDir(const std::string& dir) {
    m_impl->fontAtlasDir = dir;
}

glm::vec3 RenderPipeline::resolveEntityAnchor(uint32_t shape_id,
                                               Core::EntityType entity_type,
                                               uint32_t local_id) const {
    auto ranges = m_impl->bufferManager.lookupEntity(shape_id, entity_type, local_id);
    if(ranges.empty()) {
        return glm::vec3{0.0F};
    }

    // Collect vertex positions from CPU-side cache for all DrawRanges
    std::vector<glm::vec3> positions;
    for(const auto& range : ranges) {
        auto vertices = m_impl->bufferManager.readVertexPositions(
            shape_id, range.vertexOffset, range.vertexCount);
        positions.insert(positions.end(), vertices.begin(), vertices.end());
    }

    return Render::computeAnchorFromVertices(positions);
}
```

Note: This requires `GpuBufferManager` to have a `readVertexPositions()` method that
returns CPU-side cached vertex data. If this method does not exist yet, add a minimal
implementation that reads from the `RenderMeshData` cache (the CPU copy stored before
GPU upload). Include `"label_anchor.hpp"` for `computeAnchorFromVertices()`.

- [ ] **Step 4: Add LabelPass to render() pass order**

In `render()`, after the wireframePass line (line 100), add:
```cpp
    m_impl->labelPass.render(state, m_impl->bufferManager);
```

- [ ] **Step 5: Add cleanup for LabelPass and FontAtlas**

In `cleanup()`, before `m_impl->opaquePass.cleanup()` (line 155), add:
```cpp
    m_impl->labelPass.cleanup();
    m_impl->fontAtlas.cleanup();
```

- [ ] **Step 6: Build to verify**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4
```
Expected: Build succeeds.

- [ ] **Step 7: Commit**

```bash
git add src/libs/render/src/render_pipeline.cpp src/libs/render/include/opengeolab/render/render_pipeline.hpp
git commit -m "feat(render): integrate LabelPass and FontAtlas into RenderPipeline

LabelPass renders between WireframePass and SelectionPass.
FontAtlas loaded from configurable resource directory via setFontAtlasDir().

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 9: describe_labels action — TDD

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/describe_labels_action.hpp`
- Create: `src/libs/scene/src/describe_labels_action.cpp`
- Create: `src/libs/scene/test/describe_labels_action_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`
- Modify: `src/libs/scene/src/scene_module.cpp`

- [ ] **Step 1: Write failing tests**

Create `src/libs/scene/test/describe_labels_action_test.cpp`:

```cpp
/**
 * @file describe_labels_action_test.cpp
 * @brief Tests for the describe_labels scene action
 */

#include <doctest/doctest.h>

#include <opengeolab/scene/describe_labels_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/label_colors.hpp>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Scene::DescribeLabelsAction;
using OpenGeoLab::Scene::Label3D;
using OpenGeoLab::Scene::LabelManager;

TEST_CASE("describe_labels: empty label manager returns legend and empty list") {
    LabelManager mgr;
    DescribeLabelsAction action(mgr);

    auto result = action.execute({}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "describe_labels");
    CHECK(result["totalLabels"] == 0);
    CHECK(result["labels"].is_array());
    CHECK(result["labels"].empty());

    // Color legend is always present
    CHECK(result["colorLegend"].is_object());
    CHECK(result["colorLegend"].contains("GeoVertex"));
    CHECK(result["colorLegend"].contains("GeoEdge"));
    CHECK(result["colorLegend"].contains("GeoFace"));
    CHECK(result["colorLegend"].contains("GeoSolid"));
}

TEST_CASE("describe_labels: returns active labels with entity details") {
    LabelManager mgr;
    Label3D label;
    label.entity = {1, EntityType::GeoFace, 3};
    label.text = "F:3";
    mgr.addLabel(label);

    DescribeLabelsAction action(mgr);
    auto result = action.execute({}, nullptr);

    CHECK(result["totalLabels"] == 1);
    REQUIRE(result["labels"].size() == 1);
    CHECK(result["labels"][0]["text"] == "F:3");
    CHECK(result["labels"][0]["shapeId"] == 1);
    CHECK(result["labels"][0]["entityType"] == "GeoFace");
    CHECK(result["labels"][0]["localId"] == 3);
    CHECK(result["labels"][0]["color"] == "#5CB85C");
}

TEST_CASE("describe_labels: color legend matches label_colors.hpp") {
    LabelManager mgr;
    DescribeLabelsAction action(mgr);
    auto result = action.execute({}, nullptr);

    auto vertex_entry = result["colorLegend"]["GeoVertex"];
    CHECK(vertex_entry["prefix"] == "V");
    CHECK(vertex_entry["color"] == "#E85454");

    auto edge_entry = result["colorLegend"]["GeoEdge"];
    CHECK(edge_entry["prefix"] == "E");
    CHECK(edge_entry["color"] == "#4A90D9");

    auto face_entry = result["colorLegend"]["GeoFace"];
    CHECK(face_entry["prefix"] == "F");
    CHECK(face_entry["color"] == "#5CB85C");

    auto solid_entry = result["colorLegend"]["GeoSolid"];
    CHECK(solid_entry["prefix"] == "S");
    CHECK(solid_entry["color"] == "#E8A654");
}

TEST_CASE("describe_labels: describe() returns valid schema") {
    LabelManager mgr;
    DescribeLabelsAction action(mgr);
    auto desc = action.describe();

    CHECK(desc["name"] == "describe_labels");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc.contains("returns"));
}
```

- [ ] **Step 2: Create action header**

Create `src/libs/scene/include/opengeolab/scene/describe_labels_action.hpp`:

```cpp
/**
 * @file describe_labels_action.hpp
 * @brief DescribeLabelsAction — return label state and visual encoding for LLM
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Return active labels and visual encoding scheme for LLM observability.
 *
 * Always includes the color legend (entity type → color + prefix) even when
 * there are no active labels, so an LLM can learn the encoding before any
 * selections are made.
 */
class OPENGEOLAB_SCENE_EXPORT DescribeLabelsAction final : public Core::IAction {
public:
    explicit DescribeLabelsAction(const LabelManager& label_manager);
    ~DescribeLabelsAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "describe_labels";

private:
    const LabelManager& m_labelManager;

    [[nodiscard]] static nlohmann::json buildColorLegend();
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Create action implementation**

Create `src/libs/scene/src/describe_labels_action.cpp`:

```cpp
/**
 * @file describe_labels_action.cpp
 * @brief DescribeLabelsAction implementation
 */

#include <opengeolab/scene/describe_labels_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/label_colors.hpp>
#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

namespace {

std::string_view entityTypeName(Core::EntityType entity_type) {
    switch(entity_type) {
    case Core::EntityType::GeoVertex:
        return "GeoVertex";
    case Core::EntityType::GeoEdge:
        return "GeoEdge";
    case Core::EntityType::GeoWire:
        return "GeoWire";
    case Core::EntityType::GeoFace:
        return "GeoFace";
    case Core::EntityType::GeoSolid:
        return "GeoSolid";
    case Core::EntityType::MeshNode:
        return "MeshNode";
    case Core::EntityType::MeshEdge:
        return "MeshEdge";
    case Core::EntityType::MeshElement:
        return "MeshElement";
    case Core::EntityType::SceneNode:
        return "SceneNode";
    }
    return "Unknown";
}

} // namespace

DescribeLabelsAction::DescribeLabelsAction(const LabelManager& label_manager)
    : m_labelManager(label_manager) {}

DescribeLabelsAction::~DescribeLabelsAction() = default;

nlohmann::json DescribeLabelsAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Return active viewport labels and their visual encoding scheme. "
         "Designed for LLM consumption alongside viewport screenshots."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success."}}},
          {"action", {{"type", "string"}, {"description", "Echo of action name."}}},
          {"colorLegend",
           {{"type", "object"},
            {"description", "Entity type → {prefix, color, description} mapping."}}},
          {"labels",
           {{"type", "array"},
            {"description",
             "Array of {text, shapeId, entityType, localId, color} for each active label."}}},
          {"totalLabels",
           {{"type", "integer"}, {"description", "Number of active labels."}}}}}};
}

nlohmann::json DescribeLabelsAction::execute(const nlohmann::json& /*param*/,
                                              const Core::ProgressCallback& progress) {
    auto labels = m_labelManager.labels();

    nlohmann::json labels_json = nlohmann::json::array();
    for(const auto& lbl : labels) {
        labels_json.push_back(
            {{"text", lbl.text},
             {"shapeId", lbl.entity.shapeId},
             {"entityType", std::string(entityTypeName(lbl.entity.entityType))},
             {"localId", lbl.entity.localId},
             {"color", std::string(Core::labelColorHex(lbl.entity.entityType))}});
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"colorLegend", buildColorLegend()},
            {"textFormat", "<prefix>:<localId>  (e.g. F:3 = Face #3)"},
            {"occlusionBehavior",
             "Labels behind geometry appear semi-transparent (30% opacity)"},
            {"labels", std::move(labels_json)},
            {"totalLabels", labels.size()}};
}

nlohmann::json DescribeLabelsAction::buildColorLegend() {
    return {{"GeoVertex",
             {{"prefix", "V"},
              {"color", Core::labelColorHex(Core::EntityType::GeoVertex)},
              {"description", "Red label — topological vertex"}}},
            {"GeoEdge",
             {{"prefix", "E"},
              {"color", Core::labelColorHex(Core::EntityType::GeoEdge)},
              {"description", "Blue label — topological edge"}}},
            {"GeoFace",
             {{"prefix", "F"},
              {"color", Core::labelColorHex(Core::EntityType::GeoFace)},
              {"description", "Green label — topological face"}}},
            {"GeoSolid",
             {{"prefix", "S"},
              {"color", Core::labelColorHex(Core::EntityType::GeoSolid)},
              {"description", "Orange label — topological solid"}}}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Add to scene CMakeLists.txt**

In `src/libs/scene/CMakeLists.txt`:
- Add `include/opengeolab/scene/describe_labels_action.hpp` to `scene_public_headers`
- Add `src/describe_labels_action.cpp` to `scene_sources`
- Add `test/describe_labels_action_test.cpp` to test `SOURCES`

- [ ] **Step 5: Register in SceneModule**

In `src/libs/scene/src/scene_module.cpp`, add include:
```cpp
#include <opengeolab/scene/describe_labels_action.hpp>
```

After the last `registerAction<>` call (line 43), add:
```cpp
    registerAction<DescribeLabelsAction>(std::cref(m_sceneGraph.labelManager()));
```

- [ ] **Step 6: Build and run tests**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo -R scene_test --output-on-failure
```
Expected: All tests pass including new describe_labels tests.

- [ ] **Step 7: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/describe_labels_action.hpp \
        src/libs/scene/src/describe_labels_action.cpp \
        src/libs/scene/test/describe_labels_action_test.cpp \
        src/libs/scene/CMakeLists.txt \
        src/libs/scene/src/scene_module.cpp
git commit -m "feat(scene): add describe_labels action for LLM observability

Returns color legend (V=Red, E=Blue, F=Green, S=Orange) and active label
list with entity references. Always includes legend even with no labels,
so LLMs can learn the visual encoding before selections are made.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 10: GLViewportRenderer label synchronization

**Files:**
- Modify: `src/app/src/gl_viewport_renderer.cpp`

- [ ] **Step 1: Add label version member and includes**

In `gl_viewport_renderer.cpp`, add includes:
```cpp
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/core/label_colors.hpp>
#include "label_anchor.hpp"  // for computeStackIndices
```

Note: `label_anchor.hpp` is a render-layer internal header in `src/libs/render/src/`.
The app target already has a `target_include_directories` that covers render's private
sources (via the library link). If not, add the private include path in the app's
CMakeLists.txt: `target_include_directories(opengeolab_app PRIVATE ${render_src_dir})`.

Add private member (or local state — depends on where GLViewportRenderer stores state):
```cpp
uint64_t m_cachedLabelVersion{0};
```

- [ ] **Step 2: Add label resolution logic in synchronize()**

After the hover version tracking block (where `m_frameState.hoveredEntries` is assigned), add:

```cpp
    // Phase 2: Resolve labels from LabelManager → FrameState
    if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
        const auto& lbl_mgr = scene->labelManager();
        const uint64_t lbl_ver = lbl_mgr.version();

        if(lbl_ver != m_cachedLabelVersion) {
            m_frameState.resolvedLabels.clear();
            auto labels = lbl_mgr.labels();

            const auto mvp = m_frameState.projMatrix * m_frameState.viewMatrix;
            const auto vp_w = static_cast<float>(m_frameState.viewportWidth);
            const auto vp_h = static_cast<float>(m_frameState.viewportHeight);

            // Collect screen positions for stacking, and resolved labels
            std::vector<glm::vec2> screen_positions;
            std::vector<Render::ResolvedLabel> resolved;
            resolved.reserve(labels.size());
            screen_positions.reserve(labels.size());

            for(const auto& lbl : labels) {
                // Compute world-space anchor from CPU-side vertex data
                glm::vec3 anchor = m_pipeline.resolveEntityAnchor(
                    lbl.entity.shapeId, lbl.entity.entityType, lbl.entity.localId);

                // Project anchor to clip space for visibility test
                auto clip = mvp * glm::vec4(anchor, 1.0F);
                if(clip.w <= 0.0F) {
                    continue; // Behind camera — skip
                }
                auto ndc = glm::vec3(clip) / clip.w;

                // Screen-space position
                float sx = (ndc.x * 0.5F + 0.5F) * vp_w;
                float sy = (ndc.y * 0.5F + 0.5F) * vp_h;
                screen_positions.emplace_back(sx, sy);

                // Depth-based occlusion: read depth buffer from previous frame
                // Clamp to valid pixel coordinates
                int px = std::clamp(static_cast<int>(sx), 0, m_frameState.viewportWidth - 1);
                int py = std::clamp(static_cast<int>(sy), 0, m_frameState.viewportHeight - 1);
                float stored_depth = 1.0F;
                glReadPixels(px, py, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &stored_depth);
                float anchor_depth = ndc.z * 0.5F + 0.5F; // NDC [-1,1] → depth [0,1]
                bool occluded = (anchor_depth > stored_depth + 0.001F);

                Render::ResolvedLabel rl;
                rl.anchorWorld = anchor;
                rl.text = lbl.text;
                rl.textColor = lbl.textColor;
                rl.bgColor = lbl.bgColor;
                rl.entityType = lbl.entity.entityType;
                rl.occluded = occluded;
                rl.stackIndex = 0; // Will be filled after all labels are collected

                resolved.push_back(std::move(rl));
            }

            // Compute stack indices for overlapping labels
            auto stack_indices = Render::computeStackIndices(screen_positions, 4.0F);
            for(std::size_t i = 0; i < resolved.size(); ++i) {
                resolved[i].stackIndex = stack_indices[i];
            }

            m_frameState.resolvedLabels = std::move(resolved);
            m_cachedLabelVersion = lbl_ver;
        }
    }
```

Add includes at the top of the file:
```cpp
#include "label_anchor.hpp"  // for computeStackIndices
```

Note: `synchronize()` runs on the render thread while the GUI thread is blocked,
so it has a valid GL context and access to the previous frame's depth buffer via
`glReadPixels()`. The occlusion depth comparison uses a 0.001 tolerance to avoid
self-occlusion from co-planar geometry.

- [ ] **Step 3: Build to verify compilation**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Full build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/app/src/gl_viewport_renderer.cpp
git commit -m "feat(app): add label version tracking in GLViewportRenderer synchronize

Resolves LabelManager labels to ResolvedLabel entries in FrameState
using version-based dirty tracking. Label anchor computation is
delegated to LabelPass on the render thread.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 11: SelectionService label methods

**Files:**
- Modify: `src/app/include/opengeolab/app/selection_service.hpp`
- Modify: `src/app/src/selection_service.cpp`

- [ ] **Step 1: Add label method declarations to SelectionService header**

Add includes:
```cpp
#include <opengeolab/core/label_colors.hpp>
```

Add forward declaration and member:
```cpp
namespace OpenGeoLab::Scene { class LabelManager; }
```

Add public methods and property:
```cpp
    void setLabelManager(Scene::LabelManager* manager);

    Q_INVOKABLE void addLabelForSelection(int shapeId, int entityType, int localId);
    Q_INVOKABLE void removeLabelForSelection(int shapeId, int entityType, int localId);
    Q_INVOKABLE void setLabelsVisible(bool visible);
    [[nodiscard]] bool labelsVisible() const;

    Q_PROPERTY(bool labelsVisible READ labelsVisible NOTIFY labelsVisibleChanged)
    Q_PROPERTY(bool autoLabel READ autoLabel WRITE setAutoLabel NOTIFY autoLabelChanged)
    [[nodiscard]] bool autoLabel() const;
    void setAutoLabel(bool enabled);

Q_SIGNALS:
    // ... add to existing signals:
    void labelsVisibleChanged();
    void autoLabelChanged();
```

Add private members:
```cpp
    Scene::LabelManager* m_labelManager{nullptr};
    bool m_labelsVisible{true};
    bool m_autoLabel{true};
```

- [ ] **Step 2: Implement label methods**

In `selection_service.cpp`, add include:
```cpp
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/label_colors.hpp>

#include <fmt/format.h>
```

Add implementations:
```cpp
void SelectionService::setLabelManager(Scene::LabelManager* manager) {
    m_labelManager = manager;
}

void SelectionService::addLabelForSelection(int shapeId, int entityType, int localId) {
    if(m_labelManager == nullptr) {
        return;
    }
    auto type = static_cast<Core::EntityType>(entityType);
    Scene::Label3D label;
    label.entity = {static_cast<uint32_t>(shapeId), type, static_cast<uint32_t>(localId)};
    label.text = fmt::format("{}:{}", Core::labelPrefix(type), localId);
    label.textColor = Core::labelColor(type);
    label.bgColor = Core::K_LABEL_BG_COLOR;
    m_labelManager->addLabel(std::move(label));
}

void SelectionService::removeLabelForSelection(int shapeId, int entityType, int localId) {
    if(m_labelManager == nullptr) {
        return;
    }
    Core::EntityRef ref{static_cast<uint32_t>(shapeId),
                        static_cast<Core::EntityType>(entityType),
                        static_cast<uint32_t>(localId)};
    m_labelManager->removeByEntity(ref);
}

void SelectionService::setLabelsVisible(bool visible) {
    if(m_labelsVisible != visible) {
        m_labelsVisible = visible;
        emit labelsVisibleChanged();
    }
}

bool SelectionService::labelsVisible() const {
    return m_labelsVisible;
}

bool SelectionService::autoLabel() const {
    return m_autoLabel;
}

void SelectionService::setAutoLabel(bool enabled) {
    if(m_autoLabel != enabled) {
        m_autoLabel = enabled;
        emit autoLabelChanged();
    }
}
```

- [ ] **Step 3: Wire auto-label on entity selection**

In the existing signal connection where `entitySelected` is handled, add auto-label logic:

```cpp
// In the existing entitySelected signal handler:
if(m_autoLabel && m_labelManager != nullptr) {
    addLabelForSelection(static_cast<int>(entity.shapeId),
                         static_cast<int>(entity.entityType),
                         static_cast<int>(entity.localId));
}
```

Similarly, on `entityDeselected`:
```cpp
if(m_autoLabel && m_labelManager != nullptr) {
    removeLabelForSelection(static_cast<int>(entity.shapeId),
                            static_cast<int>(entity.entityType),
                            static_cast<int>(entity.localId));
}
```

On `selectionCleared`:
```cpp
if(m_labelManager != nullptr) {
    m_labelManager->clearLabels();
}
```

- [ ] **Step 4: Wire setLabelManager in main.cpp**

In `src/app/src/main.cpp`, after `selectionService.setSelectionState(...)`, add:
```cpp
selectionService.setLabelManager(&scene_module->sceneGraph().labelManager());
```

- [ ] **Step 5: Build to verify**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Full build succeeds.

- [ ] **Step 6: Commit**

```bash
git add src/app/include/opengeolab/app/selection_service.hpp \
        src/app/src/selection_service.cpp \
        src/app/src/main.cpp
git commit -m "feat(app): add label management to SelectionService

Auto-label on entity select/deselect using entity-type colors and prefix
text (V:1, E:3, F:6). Exposes addLabelForSelection, removeLabelForSelection,
setLabelsVisible, and autoLabel property to QML.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 12: GeoQueryPage QML label wiring

**Files:**
- Modify: `src/app/resource/qml/components/pages/GeoQueryPage.qml`

- [ ] **Step 1: Add auto-label toggle and cleanup**

In `GeoQueryPage.qml`, add a label toggle in the header area (near the EntityTypeSelector):

```qml
// After the EntityTypeSelector section, add:
RowLayout {
    Layout.fillWidth: true
    spacing: 8

    Text {
        text: qsTr("Show Labels")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    Switch {
        id: labelToggle
        checked: SelectionService.autoLabel
        onCheckedChanged: SelectionService.autoLabel = checked
    }
}
```

- [ ] **Step 2: Clear labels on panel close**

Update the existing `Component.onDestruction` or close handler:

```qml
function close() {
    SelectionService.deactivatePickMode()
    SelectionService.clearSelection()
    // Phase 2: also clear labels
    if (typeof SelectionService.setLabelsVisible === "function") {
        SelectionService.setLabelsVisible(false)
    }
}
```

Note: The conditional check ensures backward compatibility if the method isn't available yet.

- [ ] **Step 3: Build to verify QML loads**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Build succeeds. Manual test: launch app, open GeoQueryPage, verify toggle appears.

- [ ] **Step 4: Commit**

```bash
git add src/app/resource/qml/components/pages/GeoQueryPage.qml
git commit -m "feat(app): wire label toggle and cleanup in GeoQueryPage

Adds auto-label toggle switch and clears labels on panel close.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 13: Full integration build and test

- [ ] **Step 1: Full rebuild**

Run:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Full build succeeds with zero errors.

- [ ] **Step 2: Run all tests**

Run:
```bash
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: All tests pass (existing + new font_atlas, label_anchor, describe_labels tests).

- [ ] **Step 3: clang-format**

Run:
```bash
clang-format -i src/libs/core/include/opengeolab/core/label_colors.hpp \
                src/libs/render/src/font/font_atlas.hpp \
                src/libs/render/src/font/font_atlas.cpp \
                src/libs/render/src/label_anchor.hpp \
                src/libs/render/src/label_anchor.cpp \
                src/libs/render/src/pass/label_pass.hpp \
                src/libs/render/src/pass/label_pass.cpp \
                src/libs/scene/include/opengeolab/scene/describe_labels_action.hpp \
                src/libs/scene/src/describe_labels_action.cpp
```

- [ ] **Step 4: clang-tidy check**

Run clang-tidy on new files and fix any warnings.

- [ ] **Step 5: Manual visual test**

1. Launch the application
2. Create a box (or load a model)
3. Open the Geometry Query page
4. Select some faces/edges/vertices
5. Verify labels appear at entity locations
6. Rotate camera — labels should billboard
7. Toggle label switch off/on
8. Close panel — labels should disappear
9. Call `describe_labels` via Python — verify JSON output

- [ ] **Step 6: Final commit if any formatting fixes**

```bash
git add -A
git commit -m "style(render): apply clang-format to Phase 2 label rendering files

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
