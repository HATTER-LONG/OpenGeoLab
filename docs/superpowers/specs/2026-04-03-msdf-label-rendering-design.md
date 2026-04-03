# Phase 2: MSDF Label Rendering

Standalone spec for Phase 2 of the Selection & Geometry Query system.
Parent spec: `2026-03-31-selection-query-design.md`.

> **Scope note:** This spec expands on the parent's Phase 2 outline by adding
> `describe_labels` action (for LLM observability), SelectionService label methods,
> auto-label behavior, and a shared label color header. These additions were
> decided during brainstorming and should be reflected back into the parent spec's
> Phase 2 scope section.

## Problem Statement

When entities are selected in the Geometry Query panel, the user needs on-screen labels
(V:1, E:3, F:6, S:1) attached to each entity so that:

1. The selected set is immediately recognizable in the 3D viewport.
2. Subsequent viewport screenshots can be fed to an LLM together with a structured
   `describe_labels` response, enabling the LLM to reference entities by their IDs.

---

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| MSDF atlas source | Offline pre-generation via msdf-atlas-gen | Zero runtime dependency; ASCII is sufficient for `V:1 E:3 F:6 S:1` |
| Character set | 95 printable ASCII (0x20–0x7E) via `-charset ascii` | Covers entity IDs, shape names, LLM tags, common symbols |
| Render mode | Billboard (always faces camera) | Natural 3D spatial relationship with geometry |
| Depth behavior | Read depth, no write; occluded → reduced alpha | Labels always visible but signal occlusion |
| Size strategy | World-space base + pixel clamp [12px, 48px] | Readable at any zoom level |
| Visual style | Rounded-rect background + pointer triangle toward anchor | Polished, unambiguous association with entity |
| Label colors | Per-entity-type: V=red, E=blue, F=green, S=orange | Fast visual classification |
| Overlap handling | Same-anchor labels stack vertically | Simple, predictable, low implementation cost |
| LLM observability | `describe_labels` scene action returns color legend + label list | Screenshot + JSON gives LLM full context |

---

## Architecture Overview

### Layer Placement

```
core            ← EntityRef, EntityTag, EntityType (unchanged)
  ↑
scene           ← LabelManager (Phase 1, already implemented)
  ↑
render          ← FontAtlas (NEW), LabelPass (NEW)
  ↑
app             ← SelectionService extension, GeoQueryPage label wiring
```

No new libraries introduced. All new code lives in existing `render` and `app` targets.

### Data Flow

```
                       ┌─────────────────────────────────┐
                       │  GeoQueryPage (QML)              │
                       │  onEntitySelected → addLabel()   │
                       │  onPanelClosed   → clearLabels() │
                       └──────────┬──────────────────────┘
                                  │ Q_INVOKABLE
                       ┌──────────▼──────────────────────┐
                       │  SelectionService (app bridge)   │
                       │  addLabelForSelection()          │
                       │  removeLabelForSelection()       │
                       └──────────┬──────────────────────┘
                                  │ calls
                       ┌──────────▼──────────────────────┐
                       │  LabelManager (scene)            │
                       │  addLabel({entity, text, colors})│
                       │  version++  →  labelsChanged     │
                       └──────────┬──────────────────────┘
                                  │ signal
              ┌───────────────────▼──────────────────────────┐
              │  GLViewportRenderer::synchronize()            │
              │  if labelVersion changed:                     │
              │    snapshot labels → FrameState.labelSnapshot │
              └───────────────────┬──────────────────────────┘
                                  │
              ┌───────────────────▼──────────────────────────┐
              │  LabelPass::render(FrameState, GpuBufferMgr) │
              │  1. Resolve EntityRef → 3D anchor             │
              │  2. Project to screen space                   │
              │  3. Compute billboard quads                   │
              │  4. Stack overlapping labels                  │
              │  5. Render MSDF glyphs + backgrounds          │
              └──────────────────────────────────────────────┘
```

---

## Component Specifications

### 1. MSDF Font Atlas (render layer)

**New files:**
- `src/libs/render/src/font/font_atlas.hpp`
- `src/libs/render/src/font/font_atlas.cpp`

**Embedded resources:**
- `src/libs/render/resource/fonts/label_atlas.png` — 512×512 MSDF texture
- `src/libs/render/resource/fonts/label_atlas.json` — Glyph metrics

#### Atlas Generation (offline, developer tooling)

```bash
msdf-atlas-gen \
  -font <monospace-ttf> \
  -charset ascii \
  -type msdf \
  -dimensions 512 512 \
  -pxrange 4 \
  -json label_atlas.json \
  -imageout label_atlas.png
```

The generated atlas and JSON are committed to the repository as binary resources.
No runtime font dependency.

#### FontAtlas Class

```cpp
/// @file font_atlas.hpp
/// @brief Loads a pre-generated MSDF font atlas for label rendering.

struct GlyphMetrics {
    float advance;             ///< Horizontal advance in EM units
    float planeBounds[4];      ///< left, bottom, right, top in EM space
    float atlasBounds[4];      ///< left, bottom, right, top in atlas pixels
};

class FontAtlas {
public:
    /// Load atlas texture + metrics from embedded resource paths.
    /// Must be called with a valid GL context.
    bool initialize(const std::string& atlasImagePath,
                    const std::string& metricsJsonPath);

    void cleanup();

    /// Bind the atlas texture to the given unit.
    void bind(GLuint textureUnit = 0) const;

    /// Look up glyph metrics by Unicode code point.
    /// Returns nullptr for missing glyphs.
    [[nodiscard]] const GlyphMetrics* glyph(uint32_t codePoint) const;

    /// Atlas texture dimensions.
    [[nodiscard]] glm::ivec2 atlasSize() const;

    /// Font metrics (common to all glyphs).
    [[nodiscard]] float lineHeight() const;
    [[nodiscard]] float ascender() const;
    [[nodiscard]] float descender() const;

private:
    GLuint m_texture{0};
    glm::ivec2 m_atlasSize{};
    float m_lineHeight{};
    float m_ascender{};
    float m_descender{};
    std::unordered_map<uint32_t, GlyphMetrics> m_glyphs;
};
```

**JSON parsing:** Use `nlohmann::json` (already a project dependency via CPM) to
parse glyph metrics at load time.

**Texture format:** `GL_RGB8` for MSDF (3-channel signed distance fields).
Filtering: `GL_LINEAR` (required for MSDF interpolation).

---

### 2. LabelPass (render layer)

**New files:**
- `src/libs/render/src/pass/label_pass.hpp`
- `src/libs/render/src/pass/label_pass.cpp`

#### Position in Pipeline

```
1. OpaquePass           (filled geometry)
2. HighlightPass        (selection/hover overlays)
3. WireframePass        (edge wireframes)
4. LabelPass            (MSDF billboard labels)  ← NEW
5. SelectionPass        (GPU pick FBO — separate target)
```

LabelPass renders after all geometry so labels appear on top of the scene
but respect depth for occlusion feedback.

#### Resolved Label Structure

Intermediate data computed during `synchronize()` and stored in FrameState:

```cpp
struct ResolvedLabel {
    glm::vec3 anchorWorld;     ///< 3D world-space anchor point
    std::string text;          ///< Display text ("F:3")
    glm::vec4 textColor;       ///< Entity-type color
    glm::vec4 bgColor;         ///< Background color (with alpha)
    Core::EntityType entityType; ///< For color lookup fallback
    uint32_t stackIndex{0};    ///< Vertical offset for overlapping anchors
    bool occluded{false};      ///< True if anchor behind geometry
};
```

#### Anchor Computation

Anchors are computed from GPU vertex data via GpuBufferManager's `lookupEntity()`:

| EntityType | Strategy | Detail |
|-----------|----------|--------|
| GeoVertex | Vertex position | First vertex in the point DrawRange |
| GeoEdge | Midpoint | Average of first and last vertex in line DrawRange |
| GeoFace | Centroid | Average of all vertices in triangle DrawRange |
| GeoSolid | Average of all vertices | Pool all vertices from every constituent face DrawRange, compute single centroid |

The anchor is in **world space**. LabelPass projects it to screen space for quad placement.

If `lookupEntity()` returns empty (entity not tessellated), the label is skipped.

#### Overlap Stacking

After projecting all anchors to screen space, labels sharing the same screen
position (within a 4px tolerance) are stacked vertically:

```
stackIndex = 0:   ┌────┐
                   │F:3 │
                   └──┬─┘     ← pointer at anchor
stackIndex = 1:   ┌────┐
                   │E:5 │
                   └────┘     ← offset upward by labelHeight + gap
```

Stacking is computed per frame in screen space. The offset is
`stackIndex * (labelHeight + gapPx)` upward.

#### Billboard Quad Generation

Each label is a screen-aligned quad:

1. Project `anchorWorld` to NDC via `projMatrix * viewMatrix`.
2. Compute text bounds from glyph metrics: `textWidth`, `textHeight`.
3. Add padding for background: `bgWidth = textWidth + 2*padH`, `bgHeight = textHeight + 2*padV`.
4. Apply world-space base size with pixel clamping:
   ```
   worldSize = baseWorldHeight  (e.g. 0.05 units)
   pixelSize = worldSize * (viewportHeight / orthoHeight)
   clampedPx = clamp(pixelSize, minPx=12, maxPx=48)
   scale = clampedPx / pixelSize
   ```
5. Generate 4 vertices (pos + texcoord) for background quad.
6. Generate 4 vertices per glyph character.
7. Generate 3 vertices for the pointer triangle below the background.

#### Depth and Blending Behavior

```cpp
// In LabelPass::render():
glEnable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);          // Read depth, don't write
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

For occluded labels:
- During `resolveLabels()` on the CPU (in `synchronize()`), project `anchorWorld`
  to NDC and read the depth buffer at that screen pixel via `glReadPixels`.
- Compare the projected anchor Z (in NDC) to the stored depth value.
- If anchor Z > stored depth (anchor is behind geometry), set `ResolvedLabel::occluded = true`.
- Pass occlusion as a per-vertex attribute (`a_occlusionAlpha`): `1.0` for visible,
  `0.3` for occluded. All vertices of a given label share the same value.
- In the fragment shader, multiply final `fragColor.a` by `a_occlusionAlpha`.

#### Shader

**Vertex Shader:**
```glsl
#version 330 core

layout(location = 0) in vec2 a_pos;            // Screen-space quad vertex
layout(location = 1) in vec2 a_texCoord;       // Atlas UV
layout(location = 2) in vec4 a_color;          // Text or background color
layout(location = 3) in float a_isMsdf;        // 1.0 = glyph, 0.0 = background/pointer
layout(location = 4) in float a_occlusionAlpha; // 1.0 = visible, 0.3 = occluded

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
```

**Fragment Shader:**
```glsl
#version 330 core

uniform sampler2D u_atlas;
uniform float u_pxRange;       // MSDF pixel range (typically 4.0)

in vec2 v_texCoord;
in vec4 v_color;
in float v_isMsdf;

out vec4 fragColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    if (v_isMsdf > 0.5) {
        // MSDF glyph rendering
        vec3 msd = texture(u_atlas, v_texCoord).rgb;
        float sd = median(msd.r, msd.g, msd.b);

        // Compute screen-space distance for resolution-independent edges
        vec2 unitRange = u_pxRange / vec2(textureSize(u_atlas, 0));
        vec2 screenTexSize = 1.0 / fwidth(v_texCoord);
        float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);

        float opacity = smoothstep(0.5 - 1.0/screenPxRange,
                                   0.5 + 1.0/screenPxRange, sd);
        fragColor = vec4(v_color.rgb, v_color.a * opacity);
    } else {
        // Background / pointer triangle
        fragColor = v_color;
    }
}
```

#### Rounded Rectangle and Pointer

The background is rendered as two sub-elements, both using `v_isMsdf = 0.0`:

1. **Rounded rectangle:** Pre-tessellated as a triangle fan with corner vertices
   approximating the rounded shape (8 vertices per corner, 32 + center = ~34 vertices
   total). This avoids per-label uniforms and is compatible with single-draw-call
   batching. Corner radius is a compile-time constant (e.g. 4px).
   
2. **Pointer triangle:** A small triangle (6px tall) pointing from the bottom-center
   of the background down to the entity anchor point. Rendered as 3 vertices with
   the background color.

#### Performance Budget

- **Max labels per frame:** 256 (soft limit from FrameState snapshot)
- **Vertex buffer:** Dynamic, rebuilt each frame from resolved labels
  - Per label (avg 4 chars): ~4 glyph quads (24 verts) + 1 rounded rect (~34 verts) + 1 pointer (3 verts) ≈ 61 vertices
  - 256 labels × 61 = ~15600 vertices — negligible GPU cost
- **Draw calls:** 2 per frame (background pass + glyph pass) via instanced draw
  or single VBO with interleaved bg/glyph vertices
- **Atlas texture bind:** 1 texture unit, bound once per frame

---

### 3. FrameState Extension

**File:** `src/libs/render/include/opengeolab/render/frame_state.hpp`

Add label snapshot data alongside existing selection/hover entries:

```cpp
struct ResolvedLabel {
    glm::vec3 anchorWorld;
    std::string text;
    glm::vec4 textColor;
    glm::vec4 bgColor;
    Core::EntityType entityType;
    uint32_t stackIndex{0};
};

struct FrameState {
    // ... existing fields (viewMatrix, projMatrix, selectedEntries, etc.) ...

    // Phase 2: Label rendering
    std::vector<ResolvedLabel> resolvedLabels;
    bool labelsVisible{true};
};
```

#### Dirty Tracking in GLViewportRenderer::synchronize()

```cpp
// In synchronize():
auto currentLabelVersion = scene.labelManager().version();
if (currentLabelVersion != m_cachedLabelVersion) {
    m_cachedLabelVersion = currentLabelVersion;
    resolveLabels(scene, bufferManager, frameState);
}
```

`resolveLabels()` iterates `labelManager().labels()`, computes anchor positions
from `GpuBufferManager::lookupEntity()`, and populates `FrameState::resolvedLabels`.

---

### 4. RenderPipeline Integration

**File:** `src/libs/render/src/render_pipeline.cpp`

#### Impl Extension

```cpp
struct RenderPipeline::Impl {
    // ... existing passes ...
    LabelPass labelPass;          // NEW
    FontAtlas fontAtlas;          // NEW
};
```

#### Initialization

```cpp
bool RenderPipeline::initialize(GlLoaderFunc loader) {
    // ... existing pass init ...

    // Phase 2: Label rendering
    if (!m_impl->fontAtlas.initialize(":/fonts/label_atlas.png",
                                       ":/fonts/label_atlas.json")) {
        return false;
    }
    if (!m_impl->labelPass.initialize(&m_impl->fontAtlas)) {
        return false;
    }
    return true;
}
```

#### Render Order

```cpp
void RenderPipeline::render(const FrameState& state) {
    // 1. Clear + depth setup (existing)
    // 2. OpaquePass (existing)
    // 3. HighlightPass (existing)
    // 4. WireframePass (existing)

    // 5. LabelPass (NEW)
    if (state.labelsVisible && !state.resolvedLabels.empty()) {
        m_impl->labelPass.render(state, m_impl->bufferManager);
    }

    // 6. SelectionPass (existing, separate FBO)
}
```

---

### 5. SelectionService Extension (app layer)

**File:** `src/app/include/opengeolab/app/selection_service.hpp`

New methods for label management:

```cpp
/// Add a label for the given entity with auto-generated text.
/// Text is derived from entity type: "V:1", "E:3", "F:6", "S:1".
Q_INVOKABLE void addLabelForSelection(int shapeId, int entityType, int localId);

/// Remove the label for the given entity.
Q_INVOKABLE void removeLabelForSelection(int shapeId, int entityType, int localId);

/// Toggle label visibility in the viewport.
Q_INVOKABLE void setLabelsVisible(bool visible);

Q_PROPERTY(bool labelsVisible READ labelsVisible NOTIFY labelsVisibleChanged)
```

#### Label Text Generation

```cpp
QString SelectionService::labelText(Core::EntityType type, uint32_t localId) {
    static constexpr std::array<const char*, 5> prefixes = {"V", "E", "W", "F", "S"};
    auto idx = static_cast<uint8_t>(type);
    if (idx > 4) return QString("?:%1").arg(localId);
    return QString("%1:%2").arg(prefixes[idx]).arg(localId);
}
```

#### Auto-Label on Selection

When a new entity is selected while GeoQueryPage is active, SelectionService
automatically adds a label:

```cpp
void SelectionService::onEntitySelected(const Core::EntityRef& entity) {
    // ... existing chip list update ...

    // Phase 2: Auto-label
    if (m_autoLabel) {
        auto text = labelText(entity.entityType, entity.localId);
        Scene::Label3D label;
        label.entity = entity;
        label.text = text.toStdString();
        label.textColor = labelColorForType(entity.entityType);
        label.bgColor = glm::vec4(0.1F, 0.1F, 0.12F, 0.85F);
        m_scene->labelManager().addLabel(std::move(label));
    }
}
```

---

### 6. Label Color Scheme

Entity-type colors for label text (background is uniform dark):

| EntityType | Prefix | Color Name | Hex | RGBA (float) |
|-----------|--------|------------|-----|--------------|
| GeoVertex | V | Red | #E85454 | (0.91, 0.33, 0.33, 1.0) |
| GeoEdge | E | Blue | #4A90D9 | (0.29, 0.56, 0.85, 1.0) |
| GeoFace | F | Green | #5CB85C | (0.36, 0.72, 0.36, 1.0) |
| GeoSolid | S | Orange | #E8A654 | (0.91, 0.65, 0.33, 1.0) |

Background: `rgba(0.1, 0.1, 0.12, 0.85)` — dark semi-transparent.

Occluded labels: multiply all color alphas by `0.3`.

These colors are defined as constants in a shared header
(`src/libs/core/include/opengeolab/core/label_colors.hpp`) so that both
the render layer and describe action reference the same values.

---

### 7. GeoQueryPage Label Wiring (QML)

**File:** `src/app/resource/qml/components/pages/GeoQueryPage.qml`

#### Phase 2 Changes

1. **Auto-label toggle:** Add a small toggle button in the header area to
   enable/disable auto-labeling. Default: enabled.

2. **Panel close cleanup:** On panel close, clear both labels and selection:
   ```qml
   Component.onDestruction: {
       SelectionService.clearSelection()
       // Phase 2: also clear labels
       SelectionService.setLabelsVisible(false)
   }
   ```

3. **Per-chip label indicator:** Each entity chip shows a small colored dot
   matching the label color, confirming the label is active in the viewport.

4. **Label visibility toggle:** A toolbar button to show/hide all labels
   without removing them from LabelManager.

---

### 8. describe_labels Action (scene module)

**New files:**
- `src/libs/scene/include/opengeolab/scene/describe_labels_action.hpp`
- `src/libs/scene/src/describe_labels_action.cpp`

Registered as a sub-action of SceneModule: `{"module": "scene", "action": "describe_labels"}`.

#### Purpose

Provides a machine-readable description of all active labels and the visual
encoding scheme. Designed for LLM consumption alongside viewport screenshots.

#### Request

```json
{"module": "scene", "action": "describe_labels", "param": {}}
```

#### Response

```json
{
    "ok": true,
    "action": "describe_labels",
    "colorLegend": {
        "GeoVertex": {"prefix": "V", "color": "#E85454", "description": "Red label — topological vertex"},
        "GeoEdge":   {"prefix": "E", "color": "#4A90D9", "description": "Blue label — topological edge"},
        "GeoFace":   {"prefix": "F", "color": "#5CB85C", "description": "Green label — topological face"},
        "GeoSolid":  {"prefix": "S", "color": "#E8A654", "description": "Orange label — topological solid"}
    },
    "textFormat": "<prefix>:<localId>  (e.g. F:3 = Face #3 in the owning shape)",
    "occlusionBehavior": "Labels behind geometry appear semi-transparent (30% opacity)",
    "labels": [
        {
            "text": "F:3",
            "shapeId": 1,
            "entityType": "GeoFace",
            "localId": 3,
            "color": "#5CB85C"
        },
        {
            "text": "E:5",
            "shapeId": 1,
            "entityType": "GeoEdge",
            "localId": 5,
            "color": "#4A90D9"
        }
    ],
    "totalLabels": 2
}
```

#### Implementation

```cpp
nlohmann::json DescribeLabelsAction::execute(
    const nlohmann::json& /*param*/,
    const Core::ProgressCallback& /*progress*/) {

    auto labels = m_sceneGraph.labelManager().labels();

    nlohmann::json result;
    result["ok"] = true;
    result["action"] = "describe_labels";

    // Static color legend (always present)
    result["colorLegend"] = buildColorLegend();
    result["textFormat"] = "<prefix>:<localId>  (e.g. F:3 = Face #3)";
    result["occlusionBehavior"] =
        "Labels behind geometry appear semi-transparent (30% opacity)";

    // Current label list
    auto& arr = result["labels"];
    arr = nlohmann::json::array();
    for (const auto& lbl : labels) {
        arr.push_back({
            {"text",       lbl.text},
            {"shapeId",    lbl.entity.shapeId},
            {"entityType", Core::entityTypeName(lbl.entity.entityType)},
            {"localId",    lbl.entity.localId},
            {"color",      labelColorHex(lbl.entity.entityType)}
        });
    }
    result["totalLabels"] = labels.size();
    return result;
}
```

The `buildColorLegend()` helper returns the static legend table. This is always
included even when there are no active labels, so the LLM can learn the encoding
before any selections are made.

---

### 9. Python Access

The `describe_labels` action is accessible via the standard JSON dispatch:

```python
import opengeolab as ogl

# Query label state for LLM context
result = ogl.dispatch({"module": "scene", "action": "describe_labels"})
legend = result["colorLegend"]
labels = result["labels"]

# Compose LLM prompt
prompt = f"""
The viewport screenshot shows {len(labels)} labeled entities.
Color encoding: {legend}
Active labels: {labels}
Please identify which face is adjacent to E:5.
"""
```

---

## Scope

### In Scope

- MSDF font atlas pre-generation (msdf-atlas-gen, ASCII, 512×512)
- FontAtlas loader (texture + JSON glyph metrics)
- LabelPass (billboard rendering, anchor computation, MSDF shader)
- Rounded-rect background + pointer triangle visual
- Depth-aware occlusion feedback (reduced alpha)
- World-space size + pixel clamping [12px, 48px]
- Per-entity-type color coding (V=red, E=blue, F=green, S=orange)
- Same-anchor label vertical stacking
- FrameState label snapshot with version-based dirty tracking
- SelectionService label management methods
- GeoQueryPage auto-label on select, clear on close
- `describe_labels` scene action for LLM observability
- Label visibility toggle

### Out of Scope (Future)

- Unicode / CJK character support
- Label size scaling with camera distance (beyond pixel clamp)
- Interactive label editing / repositioning
- Label persistence across tool switches
- Property display in labels (area, length, volume)
- Measurement / dimension annotations
- Label-to-label leader lines

---

## Testing Strategy

### Unit Tests

1. **FontAtlas** — JSON parsing, glyph lookup for all ASCII chars, missing glyph returns null.
2. **LabelPass** — Anchor computation from DrawRange vertex data (mock GpuBufferManager).
3. **ResolvedLabel stacking** — Overlapping anchors produce sequential stackIndex.
4. **describe_labels action** — Color legend always present, label list matches LabelManager state.

### Integration Tests

1. **LabelManager → FrameState** — Version change triggers label resolution in synchronize.
2. **Full pipeline** — OpaquePass + HighlightPass + WireframePass + LabelPass renders without GL errors.

### Manual Verification

1. Create a box → select faces/edges/vertices → labels appear at correct anchors.
2. Rotate camera → labels billboard correctly.
3. Zoom in/out → label size clamps at pixel limits.
4. Occlude entity behind geometry → label becomes semi-transparent.
5. Select many entities on same face → labels stack vertically.
6. Call `describe_labels` → JSON matches viewport labels.
