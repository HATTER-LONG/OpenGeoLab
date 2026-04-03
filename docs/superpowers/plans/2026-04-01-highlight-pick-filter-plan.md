# Selection Highlight, Pick Priority & GPU Filter — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver interactive pick-and-highlight with entity-type-aware colors/sizes, GPU-level pick mask filtering, and neighborhood-based pick priority (Vertex > Edge > Face).

**Architecture:** Four modules — ColorMap (core, constexpr defaults + runtime override), SelectionPass GPU mask filtering (render), pickAt() neighborhood priority (render), HighlightPass entity-type-aware rendering (render). Changes propagate through FrameState which gains a `HighlightEntry` type and `activePickMask` field.

**Tech Stack:** C++20, OpenGL 3.3 core, glad, glm, doctest, Qt 6 (app layer only)

**Build/Test commands:**
```
cmake --build build --config RelWithDebInfo --parallel 8
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `src/libs/core/include/opengeolab/core/color_map.hpp` | CREATE | RenderColor, HighlightStyle, ColorMapConfig, ColorMap namespace |
| `src/libs/core/src/color_map.cpp` | CREATE | active(), setOverride() implementation |
| `src/libs/core/test/color_map_test.cpp` | CREATE | Tests for defaults, override, reset |
| `src/libs/core/CMakeLists.txt` | MODIFY | Add color_map.hpp, color_map.cpp, test |
| `src/libs/render/include/opengeolab/render/frame_state.hpp` | MODIFY | Add HighlightEntry, activePickMask; replace DrawRange vectors |
| `src/libs/render/src/pass/selection_pass.hpp` | MODIFY | Add PickMask parameter to render() |
| `src/libs/render/src/pass/selection_pass.cpp` | MODIFY | Filter draw calls by PickMask |
| `src/libs/render/src/render_pipeline.cpp` | MODIFY | pickAt() uses readPickRegion; pass pickMask to SelectionPass |
| `src/libs/render/src/pass/highlight_pass.hpp` | MODIFY | Add point shader; accept HighlightEntry |
| `src/libs/render/src/pass/highlight_pass.cpp` | MODIFY | Entity-type-aware rendering with ColorMap |
| `src/libs/render/test/selection_pass_test.cpp` | MODIFY | Add PickMask-aware test |
| `src/libs/render/CMakeLists.txt` | NO CHANGE | color_map is in core; render already links Core |
| `src/app/src/gl_viewport_renderer.cpp` | MODIFY | synchronize() builds HighlightEntry; passes pickMask to FrameState |
| `src/app/include/opengeolab/app/gl_viewport_renderer.hpp` | MODIFY | Cache HighlightEntry vectors instead of DrawRange |

---

### Task 1: ColorMap — Header and constexpr Defaults

**Files:**
- Create: `src/libs/core/include/opengeolab/core/color_map.hpp`
- Create: `src/libs/core/test/color_map_test.cpp`
- Modify: `src/libs/core/CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `src/libs/core/test/color_map_test.cpp`:

```cpp
/**
 * @file color_map_test.cpp
 * @brief Tests for ColorMap configuration system
 */

#include <opengeolab/core/color_map.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::ColorMap;
using OpenGeoLab::Core::ColorMapConfig;
using OpenGeoLab::Core::HighlightStyle;
using OpenGeoLab::Core::RenderColor;

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
```

- [ ] **Step 2: Write the header**

Create `src/libs/core/include/opengeolab/core/color_map.hpp`:

```cpp
/**
 * @file color_map.hpp
 * @brief Centralized highlight color and size configuration
 *
 * Provides constexpr defaults matching the OGL reference palette and a
 * runtime override mechanism for user customization.
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

/// RGBA color in [0,1] float range.
struct RenderColor {
    float r{0.F};
    float g{0.F};
    float b{0.F};
    float a{1.F};
};

/// Visual style for a highlighted entity category.
struct HighlightStyle {
    RenderColor color;
    float lineWidth{1.5F};  ///< Edge rendering width (pixels).
    float pointScale{1.0F}; ///< Vertex point-size multiplier relative to defaultPointSize.
};

/// Complete color/style configuration for hover and selection states.
struct ColorMapConfig {
    HighlightStyle hoverEdgeVertex;     ///< Edge & Vertex hover.
    HighlightStyle hoverFace;           ///< Face hover.
    HighlightStyle selectionEdgeVertex; ///< Edge & Vertex selection.
    HighlightStyle selectionFace;       ///< Face selection.
    RenderColor defaultEdge;            ///< Default edge color.
    RenderColor defaultVertex;          ///< Default vertex color.
    float defaultEdgeWidth{1.5F};       ///< Default edge line width.
    float defaultPointSize{6.0F};       ///< Default vertex point size.
};

/// Access and override the active color map configuration.
namespace ColorMap {

/// Compile-time default configuration matching OGL reference palette.
inline constexpr ColorMapConfig kDefault{
    // hoverEdgeVertex: orange #ff7f00, lineWidth 2.5, pointScale 1.5
    {.color = {1.F, 0.498F, 0.F, 1.F}, .lineWidth = 2.5F, .pointScale = 1.5F},
    // hoverFace: blue #4b55e9, alpha 0.6
    {.color = {0.294F, 0.333F, 0.914F, 0.6F}, .lineWidth = 1.5F, .pointScale = 1.F},
    // selectionEdgeVertex: red-pink #ff165d, lineWidth 2.0, pointScale 1.2
    {.color = {1.F, 0.086F, 0.365F, 1.F}, .lineWidth = 2.F, .pointScale = 1.2F},
    // selectionFace: deep blue #4116ff, alpha 0.6
    {.color = {0.255F, 0.086F, 1.F, 0.6F}, .lineWidth = 1.5F, .pointScale = 1.F},
    // defaultEdge: #ffd460
    {1.F, 0.831F, 0.376F, 1.F},
    // defaultVertex: #3490de
    {0.204F, 0.565F, 0.871F, 1.F},
    // defaultEdgeWidth, defaultPointSize
    1.5F,
    6.0F,
};

/// Returns the currently active configuration (thread-safe read).
const ColorMapConfig& active();

/// Overrides the active configuration at runtime.
/// Pass @c kDefault to reset to defaults.
void setOverride(const ColorMapConfig& config);

} // namespace ColorMap
} // namespace OpenGeoLab::Core
```

- [ ] **Step 3: Write the implementation**

Create `src/libs/core/src/color_map.cpp`:

```cpp
#include <opengeolab/core/color_map.hpp>

#include <atomic>

namespace OpenGeoLab::Core::ColorMap {

namespace {

ColorMapConfig g_override = kDefault;
std::atomic<const ColorMapConfig*> g_active{&g_override};

} // namespace

const ColorMapConfig& active() { return *g_active.load(std::memory_order_acquire); }

void setOverride(const ColorMapConfig& config) {
    g_override = config;
    g_active.store(&g_override, std::memory_order_release);
}

} // namespace OpenGeoLab::Core::ColorMap
```

- [ ] **Step 4: Update CMakeLists.txt**

In `src/libs/core/CMakeLists.txt`:

Add `include/opengeolab/core/color_map.hpp` to `core_public_headers`.
Add `src/color_map.cpp` to `core_sources`.
Add a new test block:

```cmake
    opengeolab_add_doctest_test(
        opengeolab_color_map_test SOURCES test/color_map_test.cpp LINKS
        OpenGeoLab::Core)
```

- [ ] **Step 5: Build and run tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo -R color_map --output-on-failure`
Expected: All new tests PASS.

- [ ] **Step 6: Commit**

```
git add src/libs/core/include/opengeolab/core/color_map.hpp \
        src/libs/core/src/color_map.cpp \
        src/libs/core/test/color_map_test.cpp \
        src/libs/core/CMakeLists.txt
git commit -m "feat(core): add ColorMap configuration with OGL reference palette

Provides constexpr kDefault with hover/selection colors and sizes
matching the OGL reference. active() returns the current config;
setOverride() allows runtime customization.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: FrameState — Add HighlightEntry and activePickMask

**Files:**
- Modify: `src/libs/render/include/opengeolab/render/frame_state.hpp`

- [ ] **Step 1: Modify FrameState**

Replace the current `selectedDrawRanges` and `hoveredDrawRanges` with entity-type-aware entries and add the active pick mask:

In `src/libs/render/include/opengeolab/render/frame_state.hpp`, add includes and the new struct:

```cpp
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/pick_mask.hpp>
```

Add `HighlightEntry` struct before `FrameState`:

```cpp
/// A draw range annotated with entity type for style-differentiated highlighting.
struct HighlightEntry {
    Scene::DrawRange range;
    Core::EntityType entityType{Core::EntityType::GeoFace};
};
```

Replace the two vectors in `FrameState`:

```cpp
    // Old:
    // std::vector<Scene::DrawRange> selectedDrawRanges;
    // std::vector<Scene::DrawRange> hoveredDrawRanges;

    // New:
    std::vector<HighlightEntry> selectedEntries;
    std::vector<HighlightEntry> hoveredEntries;

    /// Pick mask passed to SelectionPass for GPU-level entity filtering.
    Core::PickMask activePickMask{Core::PickMask::All};
```

- [ ] **Step 2: Fix compilation — update HighlightPass**

The HighlightPass currently reads `state.selectedDrawRanges` and `state.hoveredDrawRanges`. It will fail to compile after the FrameState change. Apply a temporary fix so it compiles (full rewrite in Task 5):

In `src/libs/render/src/pass/highlight_pass.cpp`, update `render()` method to extract DrawRange vectors from the new HighlightEntry vectors. Replace lines 163-190:

```cpp
void HighlightPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if((state.selectedEntries.empty() && state.hoveredEntries.empty()) ||
       !buffers.hasData()) {
        return;
    }

    // Temporary: extract flat DrawRange lists for backward-compatible rendering.
    // Task 5 replaces this with entity-type-aware rendering.
    std::vector<Scene::DrawRange> selected_ranges;
    selected_ranges.reserve(state.selectedEntries.size());
    for(const auto& e : state.selectedEntries) {
        selected_ranges.push_back(e.range);
    }
    std::vector<Scene::DrawRange> hovered_ranges;
    hovered_ranges.reserve(state.hoveredEntries.size());
    for(const auto& e : state.hoveredEntries) {
        hovered_ranges.push_back(e.range);
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(state.viewMatrix));
    const float alpha = 1.0f;
    const FaceTransforms transforms{mvp, state.viewMatrix, normal_matrix};

    buffers.bindMainVao();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glLineWidth(HIGHLIGHT_LINE_WIDTH);

    drawHighlightedFaces(m_faceShader, selected_ranges, transforms, SELECTED_COLOR, alpha);
    drawHighlightedEdges(m_edgeShader, selected_ranges, mvp, SELECTED_COLOR, alpha);

    drawHighlightedFaces(m_faceShader, hovered_ranges, transforms, HOVERED_COLOR, alpha);
    drawHighlightedEdges(m_edgeShader, hovered_ranges, mvp, HOVERED_COLOR, alpha);

    glLineWidth(DEFAULT_LINE_WIDTH);
    glDepthFunc(GL_LESS);
    buffers.unbind();
}
```

- [ ] **Step 3: Fix compilation — update GLViewportRenderer**

In `src/app/src/gl_viewport_renderer.cpp`, update `synchronize()`:

Replace lines 167-168 (the old DrawRange assignment):
```cpp
    m_frameState.selectedDrawRanges = m_resolvedSelectedRanges;
    m_frameState.hoveredDrawRanges = m_resolvedHoveredRanges;
```

With HighlightEntry assignment (temporary, flat conversion — Task 6 does proper entityType):
```cpp
    // Build HighlightEntry vectors — entityType will be populated properly in Task 6
    m_frameState.selectedEntries.clear();
    m_frameState.selectedEntries.reserve(m_resolvedSelectedRanges.size());
    for(const auto& r : m_resolvedSelectedRanges) {
        m_frameState.selectedEntries.push_back({r, Core::EntityType::GeoFace});
    }
    m_frameState.hoveredEntries.clear();
    m_frameState.hoveredEntries.reserve(m_resolvedHoveredRanges.size());
    for(const auto& r : m_resolvedHoveredRanges) {
        m_frameState.hoveredEntries.push_back({r, Core::EntityType::GeoFace});
    }
    m_frameState.activePickMask = m_selectionActive
        ? m_selectionPickMask
        : Core::PickMask::All;
```

Also add include at top:
```cpp
#include <opengeolab/core/entity_tag.hpp>
```

- [ ] **Step 4: Update gl_viewport_renderer.hpp**

In `src/app/include/opengeolab/app/gl_viewport_renderer.hpp`, no changes needed — the cached vectors are `std::vector<Scene::DrawRange>` which are still used internally. The conversion to `HighlightEntry` happens in `synchronize()`.

- [ ] **Step 5: Build and run all tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All 27+ tests PASS (compilation fix ensures nothing regresses).

- [ ] **Step 6: Commit**

```
git add src/libs/render/include/opengeolab/render/frame_state.hpp \
        src/libs/render/src/pass/highlight_pass.cpp \
        src/app/src/gl_viewport_renderer.cpp
git commit -m "refactor(render): add HighlightEntry and activePickMask to FrameState

Replace plain DrawRange vectors with HighlightEntry (DrawRange + EntityType)
to enable entity-type-aware highlighting. Add activePickMask for GPU-level
pick filtering. HighlightPass uses temporary flat extraction pending Task 5.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: SelectionPass GPU Mask Filtering

**Files:**
- Modify: `src/libs/render/src/pass/selection_pass.hpp`
- Modify: `src/libs/render/src/pass/selection_pass.cpp`
- Modify: `src/libs/render/src/render_pipeline.cpp`

- [ ] **Step 1: Update SelectionPass interface**

In `src/libs/render/src/pass/selection_pass.hpp`, the `render()` override signature must match `RenderPassBase`. Instead of changing the virtual signature, read the mask from `FrameState::activePickMask` inside the method body (FrameState is already passed to `render()`).

No header change needed — `FrameState` already carries `activePickMask` from Task 2.

- [ ] **Step 2: Implement GPU filtering in SelectionPass::render()**

In `src/libs/render/src/pass/selection_pass.cpp`, add includes at top:

```cpp
#include <opengeolab/core/pick_mask.hpp>
```

Add a helper function in the anonymous namespace:

```cpp
/// Check whether a PickMask has any of the given bits set.
[[nodiscard]] constexpr bool hasAny(Core::PickMask mask, Core::PickMask bits) {
    return (mask & bits) != Core::PickMask::None;
}
```

Then modify the draw calls in `render()` to be conditional on the mask. Replace the unconditional triangle/line/point draw blocks (lines 88-103) with:

```cpp
    const Core::PickMask mask = state.activePickMask;

    // Triangles — draw only if Face or Solid bits are set
    if(hasAny(mask, Core::PickMask::Face | Core::PickMask::Solid)) {
        const auto triangle_batch = BatchUtils::buildIndexedBatch(
            buffers.triangleRanges(), [](const Scene::DrawRange&) { return true; });
        BatchUtils::multiDrawElements(GL_TRIANGLES, triangle_batch);
    }

    // GL_LEQUAL so edges/vertices at equal depth can overwrite face pick IDs.
    glDepthFunc(GL_LEQUAL);

    // Lines — draw only if Edge or Wire bits are set
    if(hasAny(mask, Core::PickMask::Edge | Core::PickMask::Wire)) {
        glLineWidth(PICK_LINE_WIDTH);
        const auto line_batch = BatchUtils::buildIndexedBatch(
            buffers.lineRanges(), [](const Scene::DrawRange&) { return true; });
        BatchUtils::multiDrawElements(GL_LINES, line_batch);
    }

    // Points — draw only if Vertex bit is set
    if(hasAny(mask, Core::PickMask::Vertex)) {
        glEnable(GL_PROGRAM_POINT_SIZE);
        const auto point_batch = BatchUtils::buildArrayBatch(
            buffers.pointRanges(), [](const Scene::DrawRange&) { return true; });
        BatchUtils::multiDrawArrays(GL_POINTS, point_batch);
    }
```

**Important**: When Face is disabled but Edge/Vertex are enabled, face geometry must NOT write to the depth buffer (otherwise it would occlude edges/vertices). The current code renders triangles first with GL_LESS, then edges/vertices with GL_LEQUAL. When triangles are skipped entirely, edges/vertices still work correctly since there's nothing occluding them. No special handling needed.

- [ ] **Step 3: Pass activePickMask through render pipeline**

In `src/libs/render/src/render_pipeline.cpp`, the `render()` method already passes `state` to `selectionPass.render(state, buffers)`. Since `state.activePickMask` is now part of FrameState (Task 2), it's automatically available. No change needed here.

- [ ] **Step 4: Build and run tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 5: Commit**

```
git add src/libs/render/src/pass/selection_pass.cpp
git commit -m "feat(render): add GPU-level pick mask filtering to SelectionPass

Skip GL_TRIANGLES when Face/Solid disabled, GL_LINES when Edge/Wire
disabled, GL_POINTS when Vertex disabled. Reads mask from
FrameState::activePickMask. Prevents box-select from returning
entities of disabled types.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: pickAt() Neighborhood Priority

**Files:**
- Modify: `src/libs/render/src/render_pipeline.cpp`

- [ ] **Step 1: Change pickAt() to use readPickRegion**

In `src/libs/render/src/render_pipeline.cpp`, replace the `pickAt()` method (lines 80-87):

```cpp
PickResult RenderPipeline::pickAt(int x, int y, PickMask mask) const {
    if(!m_impl->pickResolver) {
        return {};
    }

    // Read 13×13 neighborhood sorted by distance from center.
    // PickResolver applies Vertex > Edge > Face priority in VEF mode.
    constexpr int PICK_NEIGHBORHOOD_RADIUS = 6;
    const auto raw_pick_ids =
        m_impl->selectionPass.pickFbo().readPickRegion(x, y, PICK_NEIGHBORHOOD_RADIUS);
    return m_impl->pickResolver->resolve(raw_pick_ids, Detail::pickModeFromMask(mask));
}
```

- [ ] **Step 2: Build and run tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS. `pickAt()` now reads 169 pixels and uses V>E>F priority via PickResolver.

- [ ] **Step 3: Commit**

```
git add src/libs/render/src/render_pipeline.cpp
git commit -m "feat(render): use 13×13 neighborhood for pickAt() priority

Replace single-pixel readPickId with readPickRegion(x, y, 6).
PickResolver already implements Vertex > Edge > Face priority
in VEF mode. Users no longer need pixel-precise clicks.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: HighlightPass Entity-Type-Aware Rendering

**Files:**
- Modify: `src/libs/render/src/pass/highlight_pass.hpp`
- Modify: `src/libs/render/src/pass/highlight_pass.cpp`

- [ ] **Step 1: Add point shader to HighlightPass header**

In `src/libs/render/src/pass/highlight_pass.hpp`, add a third shader member:

```cpp
private:
    ShaderProgram m_faceShader; /**< Lit faces with highlight color mix */
    ShaderProgram m_edgeShader; /**< Flat-color edges with highlight */
    ShaderProgram m_pointShader; /**< Flat-color points with highlight */
```

- [ ] **Step 2: Rewrite highlight_pass.cpp**

Replace the entire `src/libs/render/src/pass/highlight_pass.cpp` with entity-type-aware rendering using ColorMap:

```cpp
#include "pass/highlight_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/render/batch_utils.hpp>

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

// --- Shaders (unchanged vertex/fragment sources) ---

constexpr std::string_view HIGHLIGHT_FACE_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat4 u_modelView;
uniform mat3 u_normalMatrix;
out vec3 v_normal;
out vec4 v_color;
out vec3 v_viewPos;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = normalize(u_normalMatrix * a_normal);
    v_color = a_color;
    v_viewPos = (u_modelView * vec4(a_position, 1.0)).xyz;
}
)glsl";

constexpr std::string_view HIGHLIGHT_FACE_FS = R"glsl(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec3 v_viewPos;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(-v_viewPos);
    float ambient      = 0.35;
    float headlamp     = abs(dot(N, V));
    float skyLight     = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting     = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 litColor = v_color.rgb * min(lighting, 1.0);
    vec3 finalColor = mix(litColor, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)glsl";

constexpr std::string_view HIGHLIGHT_EDGE_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
out vec4 v_color;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color = a_color;
}
)glsl";

constexpr std::string_view HIGHLIGHT_EDGE_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 finalColor = mix(v_color.rgb, u_highlightColor.rgb, u_highlightColor.a);
    float a = v_color.a * u_alpha;
    fragColor = vec4(finalColor * a, a);
}
)glsl";

constexpr std::string_view HIGHLIGHT_POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    gl_PointSize = u_pointSize;
    v_color = a_color;
}
)glsl";

constexpr std::string_view HIGHLIGHT_POINT_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 finalColor = mix(v_color.rgb, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor, u_alpha);
}
)glsl";

constexpr float DEFAULT_LINE_WIDTH = 1.0F;

/// Bundled transform matrices for face highlight rendering.
struct FaceTransforms {
    glm::mat4 mvp;
    glm::mat4 modelView;
    glm::mat3 normalMatrix;
};

/// Convert Core::RenderColor to glm::vec4.
glm::vec4 toVec4(const Core::RenderColor& c) { return {c.r, c.g, c.b, c.a}; }

/// Check whether an entity type represents face/solid geometry.
bool isFaceType(Core::EntityType t) {
    return t == Core::EntityType::GeoFace || t == Core::EntityType::GeoSolid;
}

/// Check whether an entity type represents edge/wire geometry.
bool isEdgeType(Core::EntityType t) {
    return t == Core::EntityType::GeoEdge || t == Core::EntityType::GeoWire;
}

/// Check whether an entity type represents vertex geometry.
bool isVertexType(Core::EntityType t) { return t == Core::EntityType::GeoVertex; }

/// Partition highlight entries into face, edge, and vertex range lists.
struct PartitionedRanges {
    std::vector<Scene::DrawRange> faces;
    std::vector<Scene::DrawRange> edges;
    std::vector<Scene::DrawRange> vertices;
};

PartitionedRanges partition(const std::vector<HighlightEntry>& entries) {
    PartitionedRanges out;
    for(const auto& e : entries) {
        if(isFaceType(e.entityType)) {
            out.faces.push_back(e.range);
        } else if(isEdgeType(e.entityType)) {
            out.edges.push_back(e.range);
        } else if(isVertexType(e.entityType)) {
            out.vertices.push_back(e.range);
        }
    }
    return out;
}

} // namespace

bool HighlightPass::onInitialize() {
    if(!m_faceShader.create(HIGHLIGHT_FACE_VS, HIGHLIGHT_FACE_FS)) {
        return false;
    }
    if(!m_edgeShader.create(HIGHLIGHT_EDGE_VS, HIGHLIGHT_EDGE_FS)) {
        m_faceShader.destroy();
        return false;
    }
    if(!m_pointShader.create(HIGHLIGHT_POINT_VS, HIGHLIGHT_POINT_FS)) {
        m_edgeShader.destroy();
        m_faceShader.destroy();
        return false;
    }
    return true;
}

void HighlightPass::onCleanup() {
    m_pointShader.destroy();
    m_edgeShader.destroy();
    m_faceShader.destroy();
}

void HighlightPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if((state.selectedEntries.empty() && state.hoveredEntries.empty()) || !buffers.hasData()) {
        return;
    }

    const auto& cfg = Core::ColorMap::active();
    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(state.viewMatrix));
    const FaceTransforms transforms{mvp, state.viewMatrix, normal_matrix};
    constexpr float alpha = 1.0F;

    buffers.bindMainVao();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // --- Selection highlight ---
    {
        auto [faces, edges, vertices] = partition(state.selectedEntries);

        // Faces
        if(!faces.empty()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0F, 5.0F);
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            m_faceShader.setMat4("u_modelView", transforms.modelView);
            const GLint loc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.selectionFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        // Edges
        if(!edges.empty()) {
            glLineWidth(cfg.selectionEdgeVertex.lineWidth);
            m_edgeShader.use();
            m_edgeShader.setMat4("u_mvp", mvp);
            m_edgeShader.setVec4("u_highlightColor", toVec4(cfg.selectionEdgeVertex.color));
            m_edgeShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                edges, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_LINES, batch);
        }

        // Vertices
        if(!vertices.empty()) {
            const float pt_size = cfg.defaultPointSize * cfg.selectionEdgeVertex.pointScale;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.selectionEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildArrayBatch(
                vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    // --- Hover highlight ---
    {
        auto [faces, edges, vertices] = partition(state.hoveredEntries);

        // Faces
        if(!faces.empty()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0F, 5.0F);
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            m_faceShader.setMat4("u_modelView", transforms.modelView);
            const GLint loc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.hoverFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        // Edges
        if(!edges.empty()) {
            glLineWidth(cfg.hoverEdgeVertex.lineWidth);
            m_edgeShader.use();
            m_edgeShader.setMat4("u_mvp", mvp);
            m_edgeShader.setVec4("u_highlightColor", toVec4(cfg.hoverEdgeVertex.color));
            m_edgeShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                edges, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_LINES, batch);
        }

        // Vertices
        if(!vertices.empty()) {
            const float pt_size = cfg.defaultPointSize * cfg.hoverEdgeVertex.pointScale;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.hoverEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildArrayBatch(
                vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    glLineWidth(DEFAULT_LINE_WIDTH);
    glDepthFunc(GL_LESS);
    buffers.unbind();
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Build and run tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 4: Commit**

```
git add src/libs/render/src/pass/highlight_pass.hpp \
        src/libs/render/src/pass/highlight_pass.cpp
git commit -m "feat(render): entity-type-aware HighlightPass with ColorMap

Partition entries by face/edge/vertex, apply per-type colors and sizes
from ColorMap::active(). Faces get polygon offset overlay, edges get
thickened lines, vertices get enlarged points. Add point shader for
vertex highlighting.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: GLViewportRenderer — Build HighlightEntry with EntityType

**Files:**
- Modify: `src/app/src/gl_viewport_renderer.cpp`
- Modify: `src/app/include/opengeolab/app/gl_viewport_renderer.hpp`

- [ ] **Step 1: Update cached vectors to use HighlightEntry**

In `src/app/include/opengeolab/app/gl_viewport_renderer.hpp`, change:

```cpp
    // Old:
    std::vector<Scene::DrawRange> m_resolvedSelectedRanges;
    std::vector<Scene::DrawRange> m_resolvedHoveredRanges;

    // New:
    std::vector<Render::HighlightEntry> m_resolvedSelectedEntries;
    std::vector<Render::HighlightEntry> m_resolvedHoveredEntries;
```

- [ ] **Step 2: Update synchronize() to build HighlightEntry with entityType**

In `src/app/src/gl_viewport_renderer.cpp`, update the selection/hover resolution block in `synchronize()`.

Replace lines 139-148 (selection resolution):
```cpp
        if(sel_ver != m_cachedSelectionVersion) {
            m_resolvedSelectedEntries.clear();
            for(const auto& entity : sel.selections()) {
                auto ranges = m_pipeline.resolveEntityDrawRanges(
                    entity.shapeId, entity.entityType, entity.localId);
                for(const auto& r : ranges) {
                    m_resolvedSelectedEntries.push_back({r, entity.entityType});
                }
            }
            m_cachedSelectionVersion = sel_ver;
        }
```

Replace lines 150-158 (hover resolution):
```cpp
        if(hov_ver != m_cachedHoverVersion) {
            m_resolvedHoveredEntries.clear();
            if(const auto hovered = sel.hovered(); hovered.has_value()) {
                auto ranges = m_pipeline.resolveEntityDrawRanges(
                    hovered->shapeId, hovered->entityType, hovered->localId);
                for(const auto& r : ranges) {
                    m_resolvedHoveredEntries.push_back({r, hovered->entityType});
                }
            }
            m_cachedHoverVersion = hov_ver;
        }
```

Replace the FrameState assignment (the temporary code from Task 2):
```cpp
    m_frameState.selectedEntries = m_resolvedSelectedEntries;
    m_frameState.hoveredEntries = m_resolvedHoveredEntries;
    m_frameState.activePickMask = m_selectionActive
        ? m_selectionPickMask
        : Core::PickMask::All;
```

- [ ] **Step 3: Build and run all tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 4: Commit**

```
git add src/app/include/opengeolab/app/gl_viewport_renderer.hpp \
        src/app/src/gl_viewport_renderer.cpp
git commit -m "feat(app): populate HighlightEntry with correct entityType

synchronize() now builds HighlightEntry vectors with the actual
EntityType from SelectionState, enabling the HighlightPass to
apply per-type colors and sizes from ColorMap.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: Commit Pending Fixes and Verify End-to-End

**Files:**
- All uncommitted changes from previous debugging sessions (shapeId=0 fix, bitmask fixes, SVG icons, UI redesign)

- [ ] **Step 1: Stage and commit all pending bug fixes**

These are the uncommitted changes from the previous debugging session that fix critical runtime issues:

```
git add -A
git commit -m "fix(app): fix entity pick filtering, shapeId=0 validity, and redesign selection UI

- EntityRef::isValid() no longer rejects shapeId=0 (ShapeStore starts at 0)
- Fix maskFace bitmask 4→8, default mask 7→11 in QML
- Add GPU pick mask override via SelectionState in renderer
- Add maskForEntityType() post-filter in dispatch functions
- Create 4 SVG entity type icons (vertex, edge, face, solid)
- Redesign EntityTypeSelector with AbstractButton + AppIcon + tooltips
- Redesign GeoQueryPage with pulsing dot indicator and Flickable chips
- Update EntityChip with monospace font

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

**Note**: This commit must be done FIRST, before the other tasks, since subsequent tasks build on these files.

- [ ] **Step 2: Full build and test**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 3: Manual verification**

Launch the application and verify:
1. Open GeoQueryPage, enable pick mode
2. Create a box shape
3. Hover over edges → orange highlight, thickened line
4. Hover over faces → blue overlay
5. Hover over vertices → orange, enlarged point
6. Click to select → red-pink edge/vertex, deep blue face
7. Disable Vertex in EntityTypeSelector → hovering over vertices shows nothing
8. Box-select → only enabled entity types appear in results
9. Selection chips appear in the GeoQueryPage Flickable area

---

## Task Execution Order

Tasks 1-6 have a linear dependency chain. Task 7 (pending fixes commit) should be done **before** Task 1 to avoid merge conflicts.

**Recommended order:**
1. **Task 7** — Commit pending fixes first
2. **Task 1** — ColorMap
3. **Task 2** — FrameState refactor
4. **Task 3** — SelectionPass GPU filtering
5. **Task 4** — pickAt() neighborhood
6. **Task 5** — HighlightPass rewrite
7. **Task 6** — GLViewportRenderer HighlightEntry
