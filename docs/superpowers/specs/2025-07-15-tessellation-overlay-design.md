# Tessellation Overlay — Design Spec

> **Branch:** `dev/action-review-and-prompt-improvement` (or new branch TBD)
> **Status:** Approved design, pending implementation plan

## Problem

Users need to inspect the underlying tessellation mesh of CAD geometry — the actual triangles, edges and vertices produced by `BRepMesh_IncrementalMesh`. Currently the viewport only shows solid shading + topological BRep edges. There is no way to visualise the discretised triangle mesh that drives rendering and meshing.

## Solution

Add a **Tessellation Overlay** toggle button next to the existing X-Ray button in `ViewportToolbar`. When active, it overlays:

- **Thin white lines** (alpha ≈ 0.6) tracing every triangle edge of the tessellation
- **Purple dots** on every tessellation vertex

The overlay is **additive** — solid shading, topological edges, and highlight passes all remain visible underneath.

## Architecture

```
QML Button  →  GLViewport::toggleShowTessellation()
            →  FrameState.showTessellation = true
            →  RenderPipeline → TessellationOverlayPass::render()
```

No new geometry is generated. The pass reuses existing `triangleRanges` in `GpuBufferManager` and re-draws them with `glPolygonMode(GL_LINE)` and `GL_POINTS`.

---

## Component Breakdown

### 1. FrameState (render lib)

**File:** `src/libs/render/include/opengeolab/render/frame_state.hpp`

Add one field after `xRayMode`:

```cpp
bool showTessellation{false};
```

No other changes to FrameState.

### 2. TessellationOverlayPass (render lib — new)

**New files:**
- `src/libs/render/include/opengeolab/render/pass/tessellation_overlay_pass.hpp`
- `src/libs/render/src/pass/tessellation_overlay_pass.cpp`

**Class interface:**

```cpp
class TessellationOverlayPass {
public:
    void initialize();
    void render(const FrameState& state, const GpuBufferManager& buffers);

private:
    ShaderProgram m_shader;
};
```

**Shader (embedded strings):**

Vertex shader — minimal MVP transform:
```glsl
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    gl_PointSize = 3.0;
}
```

Fragment shader — uniform flat color:
```glsl
#version 330 core
uniform vec4 u_color;
out vec4 fragColor;
void main() {
    fragColor = u_color;
}
```

**Render logic:**

```
render(state, buffers):
    if (!state.showTessellation) return;

    bind shader
    set u_mvp = projMatrix * viewMatrix

    bindMainVao()

    // --- Triangle edges (thin white lines) ---
    glEnable(GL_POLYGON_OFFSET_LINE)
    glPolygonOffset(-1.0, -1.0)         // bias towards camera to avoid z-fighting
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
    glLineWidth(1.0)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    u_color = vec4(1.0, 1.0, 1.0, 0.6)  // semi-transparent white
    for each range in buffers.triangleRanges():
        glDrawElements(GL_TRIANGLES, range.indexCount, GL_UNSIGNED_INT,
                       offset(range.indexOffset))
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)  // restore
    glDisable(GL_POLYGON_OFFSET_LINE)

    // --- Tessellation vertices (purple dots) ---
    glEnable(GL_PROGRAM_POINT_SIZE)
    u_color = vec4(0.6, 0.2, 0.9, 1.0)  // purple
    gl_PointSize in shader = 3.0 * state.devicePixelRatio
    for each range in buffers.triangleRanges():
        glDrawElements(GL_POINTS, range.indexCount, GL_UNSIGNED_INT,
                       offset(range.indexOffset))
    glDisable(GL_BLEND)
```

**Pipeline integration position:** After `wireframePass`, before `labelPass`.

### 3. RenderPipeline integration (render lib)

**File:** `src/libs/render/src/render_pipeline.cpp`

Changes:
- Add `TessellationOverlayPass tessellationOverlayPass;` to `Impl` struct
- Call `m_impl->tessellationOverlayPass.initialize()` in `initialize()`
- Call `m_impl->tessellationOverlayPass.render(state, m_impl->bufferManager)` after wireframePass

Render order becomes:
```
opaquePass → highlightPass → wireframePass → tessellationOverlayPass → labelPass → selectionPass
```

### 4. GLViewport (app lib)

**File:** `src/app/include/opengeolab/app/gl_viewport.hpp`

Add (mirroring xRayMode pattern):
```cpp
Q_PROPERTY(bool showTessellation READ showTessellation
           WRITE setShowTessellation NOTIFY showTessellationChanged)

bool showTessellation() const { return m_showTessellation; }
void setShowTessellation(bool enabled);
Q_SLOT void toggleShowTessellation();
Q_SIGNAL void showTessellationChanged();

bool m_showTessellation{false};
```

**File:** `src/app/src/gl_viewport.cpp`

```cpp
void GLViewport::setShowTessellation(bool enabled) {
    if (m_showTessellation == enabled) return;
    m_showTessellation = enabled;
    Q_EMIT showTessellationChanged();
    update();
}

void GLViewport::toggleShowTessellation() {
    setShowTessellation(!m_showTessellation);
}
```

### 5. GLViewportRenderer synchronize (app lib)

**File:** `src/app/src/gl_viewport_renderer.cpp`

In `synchronize()`, after reading `xRayMode`:
```cpp
m_frameState.showTessellation = m_viewport->showTessellation();
```

### 6. ViewportToolbar.qml (QML)

**File:** `src/app/resource/qml/components/ViewportToolbar.qml`

Add after the xRayToggled signal:
```qml
signal showTessellationToggled
property bool showTessellationActive: false
```

Add new button right after the Xray button (no separator needed — they're in the same "display mode" group):
```qml
ViewportToolButton {
    theme: root.theme
    iconKind: "viewMesh"
    tooltip: qsTr("Toggle tessellation wireframe")
    toggled: root.showTessellationActive
    onClicked: root.showTessellationToggled()
}
```

### 7. ViewportPanel.qml (QML)

**File:** `src/app/resource/qml/sections/ViewportPanel.qml`

Add to the ViewportToolbar block:
```qml
showTessellationActive: viewport.showTessellation
onShowTessellationToggled: viewport.toggleShowTessellation()
```

### 8. Icon: viewMesh.svg

**File:** `src/app/resource/icons/viewMesh.svg`

24×24 SVG icon showing a triangulated mesh pattern. Uses `stroke="currentColor"` for theme colorization. Design: a square/polygon subdivided into visible triangles to convey "mesh wireframe".

### 9. Translation

**File:** `src/app/resource/translations/opengeolab_zh_CN.ts`

Add `ViewportToolbar` context entry:
```xml
<message>
    <source>Toggle tessellation wireframe</source>
    <translation>切换离散网格线框</translation>
</message>
```

---

## Rendering Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Triangle edge color | `vec4(1.0, 1.0, 1.0, 0.6)` | Semi-transparent white |
| Triangle edge line width | `1.0` | Thin, unobtrusive |
| Vertex color | `vec4(0.6, 0.2, 0.9, 1.0)` | Purple, opaque |
| Vertex point size | `3.0 × devicePixelRatio` | Scales with DPI |
| Depth bias | `glPolygonOffset(-1, -1)` | Prevents z-fighting with solid |
| Blending | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` | Standard alpha blend |
| Pipeline position | After wireframePass, before labelPass | Overlays on edges |

## Interactions with Other Modes

- **X-Ray + Tessellation:** Both can be active simultaneously. Tessellation lines drawn on top of transparent geometry.
- **Display modes (Solid/Wireframe/Points):** Tessellation overlay is independent of per-node DisplayMode. It always draws triangle edges from `triangleRanges` regardless of display mode.
- **Selection/Hover:** Tessellation overlay does not interfere with picking (no pick IDs involved).

## Files Changed

| File | Change |
|------|--------|
| `render/frame_state.hpp` | Add `bool showTessellation` |
| `render/pass/tessellation_overlay_pass.hpp` | **New** — pass header |
| `render/pass/tessellation_overlay_pass.cpp` | **New** — pass implementation |
| `render/render_pipeline.cpp` | Add pass to Impl + init + render |
| `render/CMakeLists.txt` | Add new source files |
| `app/gl_viewport.hpp` | Add Q_PROPERTY + methods |
| `app/gl_viewport.cpp` | Implement toggle/setter |
| `app/gl_viewport_renderer.cpp` | Read showTessellation in synchronize() |
| `qml/components/ViewportToolbar.qml` | Add button + signal |
| `qml/sections/ViewportPanel.qml` | Wire signal to viewport |
| `icons/viewMesh.svg` | **New** — mesh wireframe icon |
| `translations/opengeolab_zh_CN.ts` | Add translation string |

## Non-Goals

- No new geometry generation (reuses existing tessellation data)
- No per-node toggle (this is a global viewport toggle)
- No action/command integration (purely UI toggle, not exposed to AI tools)
- No shader-based edge detection (uses simple `glPolygonMode`)

## Testing

- Build: `cmake --build build --config RelWithDebInfo --parallel 4`
- C++ tests: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- Manual: Import a STEP model → toggle tessellation overlay → verify white triangle edges + purple vertices visible on top of solid shading
