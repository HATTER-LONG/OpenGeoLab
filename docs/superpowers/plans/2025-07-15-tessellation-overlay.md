# Tessellation Overlay — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Add a toolbar toggle that overlays tessellation triangle edges (thin white lines) and vertices (purple dots) on top of the existing solid+wireframe rendering.

**Architecture:** New `TessellationOverlayPass` render pass reuses existing `triangleRanges` GPU data with `glPolygonMode(GL_LINE)` + `GL_POINTS`, controlled by a `showTessellation` bool threaded from QML button → `GLViewport` → `FrameState` → pass. No new geometry generated.

**Tech Stack:** C++20, OpenGL 3.3 core, Qt 6 Quick (QML), CMake/Ninja.

**Spec:** `docs/superpowers/specs/2025-07-15-tessellation-overlay-design.md`

---

## File Map

| File | Action | Responsibility |
|------|--------|----------------|
| `src/libs/render/include/opengeolab/render/frame_state.hpp` | Modify | Add `bool showTessellation` field |
| `src/libs/render/src/pass/tessellation_overlay_pass.hpp` | **Create** | Pass class header |
| `src/libs/render/src/pass/tessellation_overlay_pass.cpp` | **Create** | Shader + render logic |
| `src/libs/render/src/render_pipeline.cpp` | Modify | Register + execute new pass |
| `src/libs/render/CMakeLists.txt` | Modify | Add new source file |
| `src/app/include/opengeolab/app/gl_viewport.hpp` | Modify | Add Q_PROPERTY + toggle |
| `src/app/src/gl_viewport.cpp` | Modify | Implement setter + toggle |
| `src/app/src/gl_viewport_renderer.cpp` | Modify | Read flag in synchronize() |
| `src/app/resource/qml/components/ViewportToolbar.qml` | Modify | Add button + signal |
| `src/app/resource/qml/sections/ViewportPanel.qml` | Modify | Wire signal to viewport |
| `src/app/resource/icons/viewMesh.svg` | **Create** | Mesh wireframe icon |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | Modify | Add translation |

---

### Task 1: Add `showTessellation` to FrameState

**Files:**
- Modify: `src/libs/render/include/opengeolab/render/frame_state.hpp:48`

- [ ] **Step 1: Add the field**

In `frame_state.hpp`, add `bool showTessellation{false};` immediately after the existing `bool xRayMode{false};` line:

```cpp
    bool xRayMode{false};
    bool showTessellation{false};  ///< Overlay tessellation triangle edges and vertices.
```

- [ ] **Step 2: Build render library to confirm no compilation errors**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4`
Expected: Build succeeds (FrameState is a POD struct with no source dependencies).

- [ ] **Step 3: Commit**

```bash
git add src/libs/render/include/opengeolab/render/frame_state.hpp
git commit -m "feat(render): add showTessellation flag to FrameState"
```

---

### Task 2: Create TessellationOverlayPass

**Files:**
- Create: `src/libs/render/src/pass/tessellation_overlay_pass.hpp`
- Create: `src/libs/render/src/pass/tessellation_overlay_pass.cpp`
- Modify: `src/libs/render/CMakeLists.txt:25`

- [ ] **Step 1: Create the header**

Create `src/libs/render/src/pass/tessellation_overlay_pass.hpp`:

```cpp
/**
 * @file tessellation_overlay_pass.hpp
 * @brief Draws tessellation triangle edges (white) and vertices (purple) as debug overlay
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Debug overlay rendering tessellation mesh wireframe and vertex dots.
 *
 * Re-draws existing triangleRanges with glPolygonMode(GL_LINE) for white triangle
 * edges, then with GL_POINTS for purple vertex markers. Only active when
 * FrameState::showTessellation is true.
 */
class TessellationOverlayPass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_shader;
};

} // namespace OpenGeoLab::Render
```

- [ ] **Step 2: Create the implementation**

Create `src/libs/render/src/pass/tessellation_overlay_pass.cpp`:

```cpp
#include "pass/tessellation_overlay_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/render/batch_utils.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view TESS_OVERLAY_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
uniform float u_pointSize;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    gl_PointSize = u_pointSize;
}
)glsl";

constexpr std::string_view TESS_OVERLAY_FS = R"glsl(
#version 330 core
uniform vec4 u_color;
out vec4 fragColor;
void main() {
    fragColor = u_color;
}
)glsl";

/// Semi-transparent white for triangle edge lines.
constexpr glm::vec4 EDGE_COLOR{1.0F, 1.0F, 1.0F, 0.6F};

/// Opaque purple for tessellation vertex dots.
constexpr glm::vec4 VERTEX_COLOR{0.6F, 0.2F, 0.9F, 1.0F};

/// Base point size in logical pixels (scaled by devicePixelRatio).
constexpr float POINT_SIZE = 3.0F;

} // namespace

bool TessellationOverlayPass::onInitialize() {
    return m_shader.create(TESS_OVERLAY_VS, TESS_OVERLAY_FS);
}

void TessellationOverlayPass::onCleanup() { m_shader.destroy(); }

void TessellationOverlayPass::render(const FrameState& state,
                                     const GpuBufferManager& buffers) {
    if(!state.showTessellation || !buffers.hasData()) {
        return;
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;

    m_shader.use();
    m_shader.setMat4("u_mvp", mvp);

    buffers.bindMainVao();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Triangle edges: thin white wireframe ---
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0F, -1.0F);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0F);

    m_shader.setVec4("u_color", EDGE_COLOR);
    m_shader.setFloat("u_pointSize", 1.0F);

    const auto edge_batch = BatchUtils::buildIndexedBatch(
        buffers.triangleRanges(), [](const Scene::DrawRange& /*r*/) { return true; });
    BatchUtils::multiDrawElements(GL_TRIANGLES, edge_batch);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);

    // --- Tessellation vertices: purple dots ---
    glEnable(GL_PROGRAM_POINT_SIZE);

    m_shader.setVec4("u_color", VERTEX_COLOR);
    m_shader.setFloat("u_pointSize", POINT_SIZE * state.devicePixelRatio);

    const auto point_batch = BatchUtils::buildIndexedBatch(
        buffers.triangleRanges(), [](const Scene::DrawRange& /*r*/) { return true; });
    BatchUtils::multiDrawElements(GL_POINTS, point_batch);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);

    buffers.unbind();
}

} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Register the new source in CMakeLists.txt**

In `src/libs/render/CMakeLists.txt`, add `src/pass/tessellation_overlay_pass.cpp` after line 24 (`src/pass/label_pass.cpp`):

```cmake
set(render_sources
    src/render_pipeline.cpp
    src/batch_utils.cpp
    src/pick_resolver.cpp
    src/core/shader_program.cpp
    src/core/gpu_buffer_manager.cpp
    src/core/thick_line_renderer.cpp
    src/core/pick_fbo.cpp
    src/pass/opaque_pass.cpp
    src/pass/wireframe_pass.cpp
    src/pass/highlight_pass.cpp
    src/pass/selection_pass.cpp
    src/pass/label_pass.cpp
    src/pass/tessellation_overlay_pass.cpp
    src/label_anchor.cpp
    src/font/font_atlas.cpp)
```

- [ ] **Step 4: Build render library to verify compilation**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4`
Expected: Build succeeds with the new pass compiled.

- [ ] **Step 5: Commit**

```bash
git add src/libs/render/src/pass/tessellation_overlay_pass.hpp \
        src/libs/render/src/pass/tessellation_overlay_pass.cpp \
        src/libs/render/CMakeLists.txt
git commit -m "feat(render): add TessellationOverlayPass for mesh wireframe overlay

Renders existing triangleRanges with glPolygonMode(GL_LINE) for white
triangle edges and GL_POINTS for purple vertex dots. Only active when
FrameState::showTessellation is true. Uses a minimal flat-color shader."
```

---

### Task 3: Integrate pass into RenderPipeline

**Files:**
- Modify: `src/libs/render/src/render_pipeline.cpp:12-17` (includes), `:47-61` (Impl), `:72-81` (initialize), `:126-130` (render), `:180-191` (cleanup)

- [ ] **Step 1: Add include**

In `render_pipeline.cpp`, add after line 16 (`#include "pass/wireframe_pass.hpp"`):

```cpp
#include "pass/tessellation_overlay_pass.hpp"
```

- [ ] **Step 2: Add to Impl struct**

In the `RenderPipeline::Impl` struct (line 47-61), add after `LabelPass labelPass;` (line 55):

```cpp
    TessellationOverlayPass tessellationOverlayPass;
```

- [ ] **Step 3: Initialize the pass**

In `RenderPipeline::initialize()`, add after `m_impl->selectionPass.initialize();` (line 76):

```cpp
    m_impl->tessellationOverlayPass.initialize();
```

- [ ] **Step 4: Add render call**

In `RenderPipeline::render()`, add after `m_impl->wireframePass.render(...)` (line 128) and before `m_impl->labelPass.render(...)` (line 129):

```cpp
    m_impl->tessellationOverlayPass.render(state, m_impl->bufferManager);
```

- [ ] **Step 5: Add cleanup call**

In `RenderPipeline::cleanup()`, add after `m_impl->wireframePass.cleanup();` (line 186):

```cpp
    m_impl->tessellationOverlayPass.cleanup();
```

- [ ] **Step 6: Build and test render library**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4`
Expected: Build succeeds.

Run: `ctest --test-dir build -C RelWithDebInfo -R opengeolab_render_test --output-on-failure`
Expected: All existing render tests pass.

- [ ] **Step 7: Commit**

```bash
git add src/libs/render/src/render_pipeline.cpp
git commit -m "feat(render): integrate TessellationOverlayPass into pipeline

Render order: opaque → highlight → wireframe → tessellation overlay → label → selection."
```

---

### Task 4: Add showTessellation property to GLViewport

**Files:**
- Modify: `src/app/include/opengeolab/app/gl_viewport.hpp:40-42` (Q_PROPERTY), `:79-82` (accessors), `:142-152` (methods), `:154-162` (signals), `:182` (member)
- Modify: `src/app/src/gl_viewport.cpp:80-88` (setter), `:142` (toggle)

- [ ] **Step 1: Add Q_PROPERTY declaration**

In `gl_viewport.hpp`, add after the `xRayMode` Q_PROPERTY (line 40):

```cpp
    Q_PROPERTY(bool showTessellation READ showTessellation WRITE setShowTessellation NOTIFY
                   showTessellationChanged)
```

- [ ] **Step 2: Add accessor, setter, and toggle declarations**

After `void setXRayMode(bool enabled);` (line 82), add:

```cpp
    /** @brief Whether tessellation overlay is enabled. */
    [[nodiscard]] bool showTessellation() const { return m_showTessellation; }

    /** @brief Enable or disable tessellation mesh overlay. */
    void setShowTessellation(bool enabled);
```

After `Q_INVOKABLE void toggleXRay();` (line 152), add:

```cpp
    /** @brief Toggle tessellation overlay on or off. */
    Q_INVOKABLE void toggleShowTessellation();
```

- [ ] **Step 3: Add signal declaration**

After `void xRayModeChanged();` (line 157), add:

```cpp
    void showTessellationChanged();
```

- [ ] **Step 4: Add member variable**

After `bool m_xRayMode{false};` (line 182), add:

```cpp
    bool m_showTessellation{false};
```

- [ ] **Step 5: Implement setter and toggle in gl_viewport.cpp**

After `void GLViewport::toggleXRay() { setXRayMode(!m_xRayMode); }` (line 142), add:

```cpp
void GLViewport::setShowTessellation(bool enabled) {
    if(m_showTessellation == enabled) {
        return;
    }

    m_showTessellation = enabled;
    Q_EMIT showTessellationChanged();
    update();
}

void GLViewport::toggleShowTessellation() { setShowTessellation(!m_showTessellation); }
```

- [ ] **Step 6: Wire into GLViewportRenderer::synchronize()**

In `gl_viewport_renderer.cpp`, after line 122 (`m_frameState.xRayMode = viewport->xRayMode();`), add:

```cpp
    m_frameState.showTessellation = viewport->showTessellation();
```

- [ ] **Step 7: Build app to verify compilation**

Run: `cmake --build build --config RelWithDebInfo --parallel 4`
Expected: Full build succeeds.

- [ ] **Step 8: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests pass (no regressions).

- [ ] **Step 9: Commit**

```bash
git add src/app/include/opengeolab/app/gl_viewport.hpp \
        src/app/src/gl_viewport.cpp \
        src/app/src/gl_viewport_renderer.cpp
git commit -m "feat(app): add showTessellation property to GLViewport

Mirrors the xRayMode pattern: Q_PROPERTY + setter + toggle + signal.
GLViewportRenderer reads the flag during synchronize() and writes it
into FrameState for the TessellationOverlayPass."
```

---

### Task 5: Create viewMesh icon and add QML toolbar button

**Files:**
- Create: `src/app/resource/icons/viewMesh.svg`
- Modify: `src/app/resource/qml/components/ViewportToolbar.qml:20-22` (signals), `:101-107` (button)
- Modify: `src/app/resource/qml/sections/ViewportPanel.qml:89-94` (binding)

- [ ] **Step 1: Create the viewMesh.svg icon**

Create `src/app/resource/icons/viewMesh.svg` — a 24×24 SVG showing a triangulated surface:

```svg
<svg width="24" height="24" viewBox="0 0 24 24"
     xmlns="http://www.w3.org/2000/svg"
     fill="none" stroke="currentColor"
     stroke-linecap="round" stroke-linejoin="round">

  <!-- Outer quad representing a surface patch -->
  <polygon points="3,19 12,4 21,19" stroke-width="1.6"/>

  <!-- Internal triangle subdivision lines -->
  <line x1="7.5" y1="11.5" x2="16.5" y2="11.5" stroke-width="1.2"/>
  <line x1="7.5" y1="11.5" x2="12" y2="19" stroke-width="1.2"/>
  <line x1="16.5" y1="11.5" x2="12" y2="19" stroke-width="1.2"/>

  <!-- Vertex dots -->
  <circle cx="12" cy="4" r="1.5" fill="currentColor" stroke="none"/>
  <circle cx="3" cy="19" r="1.5" fill="currentColor" stroke="none"/>
  <circle cx="21" cy="19" r="1.5" fill="currentColor" stroke="none"/>
  <circle cx="7.5" cy="11.5" r="1.2" fill="currentColor" stroke="none"/>
  <circle cx="16.5" cy="11.5" r="1.2" fill="currentColor" stroke="none"/>
  <circle cx="12" cy="19" r="1.2" fill="currentColor" stroke="none"/>

</svg>
```

- [ ] **Step 2: Add signal and property to ViewportToolbar.qml**

In `ViewportToolbar.qml`, after `signal xRayToggled` (line 20), add:

```qml
    signal showTessellationToggled
```

After `property bool xRayActive: false` (line 22), add:

```qml
    property bool showTessellationActive: false
```

- [ ] **Step 3: Add the toolbar button**

In `ViewportToolbar.qml`, after the Xray ViewportToolButton closing brace (after line 107), add a new button (no separator — same display-mode group):

```qml
        ViewportToolButton {
            theme: root.theme
            iconKind: "viewMesh"
            tooltip: qsTr("Toggle tessellation wireframe")
            toggled: root.showTessellationActive
            onClicked: root.showTessellationToggled()
        }
```

- [ ] **Step 4: Wire the signal in ViewportPanel.qml**

In `ViewportPanel.qml`, in the ViewportToolbar block (lines 84-95), after `xRayActive: viewport.xRayMode` (line 90), add:

```qml
        showTessellationActive: viewport.showTessellation
```

After `onXRayToggled: viewport.toggleXRay()` (line 94), add:

```qml
        onShowTessellationToggled: viewport.toggleShowTessellation()
```

- [ ] **Step 5: Update translation file**

In `src/app/resource/translations/opengeolab_zh_CN.ts`, add a `<context>` block for ViewportToolbar (if it doesn't already exist). Search for the end of the last `</context>` before `</TS>` and add:

```xml
<context>
    <name>ViewportToolbar</name>
    <message>
        <source>Toggle tessellation wireframe</source>
        <translation>切换离散网格线框</translation>
    </message>
</context>
```

- [ ] **Step 6: Build full project**

Run: `cmake --build build --config RelWithDebInfo --parallel 4`
Expected: Build succeeds (QML files are copied, SVG discovered at runtime).

- [ ] **Step 7: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests pass.

- [ ] **Step 8: Commit**

```bash
git add src/app/resource/icons/viewMesh.svg \
        src/app/resource/qml/components/ViewportToolbar.qml \
        src/app/resource/qml/sections/ViewportPanel.qml \
        src/app/resource/translations/opengeolab_zh_CN.ts
git commit -m "feat(app): add tessellation wireframe toggle to viewport toolbar

New viewMesh button next to the X-Ray toggle. Wired to
GLViewport::showTessellation property through the same signal
pattern as xRayMode. Includes viewMesh.svg icon and zh_CN translation."
```

---

### Task 6: Manual verification

- [ ] **Step 1: Launch the application**

Run: `.\build\bin\RelWithDebInfo\OpenGeoLab.exe` (or the appropriate build output path)

- [ ] **Step 2: Import a STEP/BREP model to get tessellated geometry**

- [ ] **Step 3: Click the new mesh wireframe button in the toolbar**

Verify:
- Button appears next to X-Ray button
- Button has toggle highlight when active
- Thin white lines appear on top of solid surfaces showing triangle mesh edges
- Purple dots appear at every tessellation vertex
- Existing solid shading and topological edges remain visible underneath

- [ ] **Step 4: Toggle off and verify overlay disappears cleanly**

- [ ] **Step 5: Enable both X-Ray + Tessellation simultaneously**

Verify both effects work together without visual artifacts.

- [ ] **Step 6: Final commit if any fixes were needed**

If manual testing revealed issues, fix and commit with descriptive message.
