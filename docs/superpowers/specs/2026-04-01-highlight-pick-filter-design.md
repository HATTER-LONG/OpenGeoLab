# Selection Highlight, Pick Priority & GPU Filter Design

## Problem Statement

After implementing Phase 1 of the Selection & Geometry Query system, four runtime issues were identified:

1. **No pick priority** — `pickAt()` reads a single pixel; nearby vertices/edges are missed. Users must click precisely on a pixel to pick an entity.
2. **No visual feedback** — Hover and selection produce no color change, no edge thickening, no vertex enlargement. The HighlightPass exists but uses hardcoded blue/cyan with no entity-type differentiation.
3. **No centralized color/style management** — Highlight colors, line widths, and point sizes are scattered as magic numbers. No single source of truth for the visual style.
4. **Box-select ignores pick mask** — The SelectionPass renders ALL geometry to the pick FBO regardless of the active pick mask. Disabling Vertex pick still returns vertices in box-select results.

## Scope

This spec covers four tightly coupled improvements that together deliver a working interactive pick-and-highlight experience. All changes stay within the existing `core`, `render`, and `app` layers.

### In Scope

- ColorMap configuration structure in core with constexpr defaults and runtime override
- GPU-level pick mask filtering in SelectionPass
- Neighborhood-based pick with Vertex > Edge > Face priority
- Entity-type-aware HighlightPass (color, line width, point scale)
- FrameState extension to carry entity type metadata with draw ranges

### Out of Scope

- Label rendering (Phase 2)
- Script export / replay
- Multi-select toolbar / UI changes beyond what's already built
- Mesh entity highlight (only geometry entities: Vertex, Edge, Face, Solid)

## Architecture

### Module 1 — ColorMap (core library)

**Location**: `src/libs/core/include/opengeolab/core/color_map.hpp` + `.cpp`

**Purpose**: Single source of truth for all highlight/selection visual parameters.

```cpp
namespace OpenGeoLab::Core {

/// RGBA color in [0,1] float range.
struct RenderColor {
    float r{0.f};
    float g{0.f};
    float b{0.f};
    float a{1.f};
};

/// Visual style for a highlighted entity category.
struct HighlightStyle {
    RenderColor color;
    float lineWidth{1.5f};   ///< Edge rendering width (pixels).
    float pointScale{1.0f};  ///< Vertex point-size multiplier.
};

/// Complete color/style configuration for hover and selection states.
struct ColorMapConfig {
    // --- Hover styles ---
    HighlightStyle hoverEdgeVertex;    ///< Edge & Vertex hover: orange #ff7f00
    HighlightStyle hoverFace;          ///< Face hover: blue #4b55e9

    // --- Selection styles ---
    HighlightStyle selectionEdgeVertex; ///< Edge & Vertex selection: red-pink #ff165d
    HighlightStyle selectionFace;       ///< Face selection: deep blue #4116ff

    // --- Default geometry colors ---
    RenderColor defaultEdge;    ///< Default edge color: #ffd460
    RenderColor defaultVertex;  ///< Default vertex color: #3490de

    // --- Default sizes ---
    float defaultEdgeWidth{1.5f};
    float defaultPointSize{6.0f};
};

/// Access and override the active color map configuration.
namespace ColorMap {

/// Compile-time default configuration matching OGL reference.
constexpr ColorMapConfig kDefault{
    // hoverEdgeVertex: orange, line 2.5, point ×1.5
    {.color = {1.f, 0.498f, 0.f, 1.f}, .lineWidth = 2.5f, .pointScale = 1.5f},
    // hoverFace: blue, no line/point change
    {.color = {0.294f, 0.333f, 0.914f, 0.6f}, .lineWidth = 1.5f, .pointScale = 1.0f},
    // selectionEdgeVertex: red-pink, line 2.0, point ×1.2
    {.color = {1.f, 0.086f, 0.365f, 1.f}, .lineWidth = 2.0f, .pointScale = 1.2f},
    // selectionFace: deep blue
    {.color = {0.255f, 0.086f, 1.f, 0.6f}, .lineWidth = 1.5f, .pointScale = 1.0f},
    // defaultEdge: #ffd460
    {1.f, 0.831f, 0.376f, 1.f},
    // defaultVertex: #3490de
    {0.204f, 0.565f, 0.871f, 1.f},
    // defaultEdgeWidth, defaultPointSize
    1.5f,
    6.0f,
};

/// Returns the currently active configuration.
const ColorMapConfig& active();

/// Overrides the active configuration at runtime.
/// Pass kDefault to reset.
void setOverride(const ColorMapConfig& config);

} // namespace ColorMap
} // namespace OpenGeoLab::Core
```

**Design decisions**:
- `constexpr kDefault` allows compile-time usage and serves as documented reference.
- `active()` returns `kDefault` unless `setOverride()` has been called.
- Thread safety: `active()` reads a `std::atomic<const ColorMapConfig*>`; `setOverride()` copies into a `static ColorMapConfig` and publishes the pointer. Safe for render-thread reads.
- `RenderColor` is a simple POD; no dependency on Qt or OpenGL types.

### Module 2 — SelectionPass GPU Mask Filtering (render library)

**Location**: Modify existing `src/libs/render/src/passes/selection_pass.hpp/.cpp`

**Current behavior**: Renders all geometry (triangles, lines, points) to the pick FBO unconditionally.

**New behavior**: Accept a `PickMask` and skip draw calls for disabled entity types.

```
// In FrameState (or SelectionPass interface):
PickMask activePickMask{PickMask::All};  // default: render everything

// In SelectionPass::render():
if (mask has Face/Solid bit) → draw GL_TRIANGLES
if (mask has Edge/Wire bit)  → draw GL_LINES
if (mask has Vertex bit)     → draw GL_POINTS
```

**Mapping from PickMask bits to draw calls**:

| PickMask bit | Draw call | GL primitive |
|---|---|---|
| `Vertex` (1<<0) | `glDrawArrays(GL_POINTS, ...)` | Points |
| `Edge` (1<<1) or `Wire` (1<<2) | `glDrawElements(GL_LINES, ...)` | Lines |
| `Face` (1<<3) or `Solid` (1<<4) | `glDrawElements(GL_TRIANGLES, ...)` | Triangles |

**Key constraint**: The pick FBO must be **re-rendered** whenever the mask changes. Since `render()` is called every frame, and the mask is read from `FrameState`, this happens automatically.

**Depth pre-pass**: Only render depth occluders for enabled types. If Face is disabled, face geometry should not write to depth (otherwise it would occlude edges/vertices that ARE enabled).

### Module 3 — pickAt() Neighborhood Priority (render library)

**Location**: Modify `src/libs/render/src/render_pipeline.cpp`

**Current**: `pickAt()` calls `readPickId(x, y)` — single pixel.

**New**: `pickAt()` calls `readPickRegion(x, y, 6)` — 13×13 pixel neighborhood, sorted by distance from center.

```cpp
PickResult RenderPipeline::pickAt(int x, int y, PickMask mask) const {
    if (!m_impl->pickResolver) return {};

    // Read 13×13 neighborhood sorted by distance from center
    auto raw_ids = m_impl->selectionPass.pickFbo().readPickRegion(x, y, 6);
    return m_impl->pickResolver->resolve(raw_ids, Detail::pickModeFromMask(mask));
}
```

**PickResolver::resolve()** already implements priority:
- VEF mode: iterates all neighborhood hits, returns entity with highest `typePriority` (Vertex=3 > Edge=2 > Face=1).
- Part/Wire/Solid modes: returns first valid match (center-biased).

**No changes needed to PickResolver** — only the data source changes from 1 pixel to 169 pixels.

### Module 4 — HighlightPass Entity-Type-Aware Rendering (render library)

**Location**: Modify existing `src/libs/render/src/passes/highlight_pass.hpp/.cpp`

**Current behavior**: Renders all selected/hovered draw ranges with hardcoded blue/cyan color. No entity-type differentiation. No line width or point size changes.

**New behavior**: Read `ColorMap::active()` for colors and sizes. Render different entity types with different styles.

#### FrameState Extension

Current `FrameState` carries:
```cpp
std::vector<DrawRange> selectedDrawRanges;
std::vector<DrawRange> hoveredDrawRanges;
```

**New**: Extend `DrawRange` or add a parallel structure with entity type metadata:

```cpp
struct HighlightEntry {
    DrawRange range;
    Core::EntityType entityType;  ///< For style lookup in ColorMap.
};

// In FrameState:
std::vector<HighlightEntry> selectedEntries;
std::vector<HighlightEntry> hoveredEntries;
```

This allows HighlightPass to group entries by entity type and apply different GL state per group.

#### Rendering Strategy

HighlightPass groups entries by entity type and renders in batches:

1. **Face entries** (entityType == GeoFace or GeoSolid):
   - `glDrawElements(GL_TRIANGLES, ...)`
   - Color from `selectionFace.color` or `hoverFace.color`
   - Alpha blending for overlay effect
   - `glPolygonOffset` to avoid z-fighting

2. **Edge entries** (entityType == GeoEdge or GeoWire):
   - `glDrawElements(GL_LINES, ...)`
   - Color from `selectionEdgeVertex.color` or `hoverEdgeVertex.color`
   - `glLineWidth(style.lineWidth)` — 2.0 for selected, 2.5 for hovered

3. **Vertex entries** (entityType == GeoVertex):
   - `glDrawArrays(GL_POINTS, ...)`
   - Color from `selectionEdgeVertex.color` or `hoverEdgeVertex.color`
   - `glPointSize(defaultPointSize × style.pointScale)` — ×1.2 for selected, ×1.5 for hovered

**Render order**: Faces → Edges → Vertices (back to front, same as SelectionPass).

#### resolveEntityDrawRanges() Enhancement

Current `RenderPipeline::resolveEntityDrawRanges()` returns `std::vector<DrawRange>`. Need to also return entity type.

**Option**: Change return type to `std::vector<HighlightEntry>` or accept entity type as input parameter (already known by caller).

Preferred: Caller already knows `EntityRef.entityType`, so construct `HighlightEntry` at the call site in `GLViewportRenderer::synchronize()`:

```cpp
for (const auto& entity : selectionState.selected()) {
    auto ranges = m_pipeline.resolveEntityDrawRanges(
        entity.shapeId, entity.entityType, entity.localId);
    for (auto& r : ranges) {
        m_frameState.selectedEntries.push_back({r, entity.entityType});
    }
}
```

## Data Flow

```
User clicks mouse
  → GLViewport::mousePressEvent()
  → GLViewportRenderer::render() triggers pick
  → SelectionPass renders pick FBO (GPU-filtered by PickMask)
  → pickAt(x, y, mask) reads 13×13 neighborhood
  → PickResolver selects highest-priority entity (V>E>F)
  → dispatchPickResult() updates SelectionState
  → next frame: synchronize() resolves entities → HighlightEntries
  → HighlightPass renders with ColorMap styles per entity type
```

## Testing Strategy

### ColorMap
- Verify `kDefault` values match OGL reference hex colors.
- Verify `active()` returns `kDefault` initially.
- Verify `setOverride()` changes `active()` result.

### SelectionPass GPU Filtering
- Existing pick_resolver_test covers priority logic.
- New test: verify SelectionPass respects mask — mock-verify that draw calls are skipped when mask bits are cleared. (May require integration test or visual verification.)

### pickAt() Neighborhood
- Existing pick_resolver_test already tests `resolve()` with multi-element input.
- Integration test: verify `pickAt()` returns Vertex when both Vertex and Face are within 6px radius.

### HighlightPass
- Visual verification: hover shows orange edge/vertex, blue face overlay.
- Unit test: verify HighlightPass reads ColorMap styles (mock-based or by checking GL state).

## Migration Notes

- `FrameState::selectedDrawRanges` and `hoveredDrawRanges` are replaced by `selectedEntries` and `hoveredEntries`. All consumers must update.
- The `pickMask` in `FrameState` is a new field; defaults to `PickMask::All` for backward compatibility.
- ColorMap header is in core, but the `.cpp` with override logic links into core. Render library depends on core (already the case).

## File Change Summary

| File | Change |
|---|---|
| `src/libs/core/include/opengeolab/core/color_map.hpp` | **NEW** — RenderColor, HighlightStyle, ColorMapConfig, ColorMap namespace |
| `src/libs/core/src/color_map.cpp` | **NEW** — active(), setOverride() implementation |
| `src/libs/core/CMakeLists.txt` | Add color_map.hpp, color_map.cpp |
| `src/libs/core/test/color_map_test.cpp` | **NEW** — Tests for defaults, override, reset |
| `src/libs/render/src/passes/selection_pass.hpp` | Add pickMask parameter to render() |
| `src/libs/render/src/passes/selection_pass.cpp` | Filter draw calls by pickMask |
| `src/libs/render/src/render_pipeline.cpp` | pickAt(): readPickId → readPickRegion(x,y,6) |
| `src/libs/render/include/opengeolab/render/frame_state.hpp` | Add HighlightEntry, replace DrawRange vectors, add pickMask |
| `src/libs/render/src/passes/highlight_pass.hpp` | Read ColorMap, render per entity type |
| `src/libs/render/src/passes/highlight_pass.cpp` | Implement entity-type-aware highlight rendering |
| `src/app/src/gl_viewport_renderer.cpp` | synchronize(): build HighlightEntry lists with entityType |
