# Selection & Geometry Query Design

## Problem Statement

OpenGeoLabNew needs a selection/picking system with a Geometry Query tool panel. The system must:
1. Provide a reusable selection infrastructure (hover, select, box-select)
2. Render entity ID labels (F:1, V:1, E:3, S:1) attached to selected entities in the 3D viewport
3. Support Python/JSON access for programmatic selection control
4. Separate selection (generic capability) from label display (business behavior)

## Architecture Overview

### Dependency Chain (unidirectional, no cycles)

```
core            ← EntityRef, EntityType, PickMask
  ↑
scene           ← TopologyIndex, DrawRange, RenderMeshData, LabelManager
  ↑
selection       ← SelectionState, SelectionModule (NEW library)
  ↑
render          ← HighlightPass, LabelPass, SelectionPass, PickResolver
  ↑
app             ← SelectionService (QML bridge), GLViewport, Query Panel
```

### Key Design Decisions

1. **MSDF text rendering** for entity ID labels — supports future complex symbols for LLM identification
2. **libs/selection** as a new library — not in app layer, accessible via JSON action protocol for Python
3. **render depends on selection** — render writes pick/hover results to SelectionState, reads selection state for highlighting
4. **EntityRef in core** — breaks circular dependency between render and selection
5. **LabelManager in scene** — label display is scene annotation, independent of selection
6. **Auto-anchor computation** — UI only passes EntityRef + text; render computes 3D anchor from geometry each frame
7. **Selection ≠ Labels** — Selection manages hover/select state + highlight; Query panel orchestrates label creation as business logic

---

## Component Specifications

### 1. EntityRef (core layer)

**File:** `src/libs/core/include/opengeolab/core/entity_ref.hpp`

Universal entity reference used across all layers. Replaces render-specific PickResult for cross-layer communication.

```cpp
struct EntityRef {
    uint32_t   shapeId{};       // Owning shape/part ID
    EntityType entityType{};    // Entity type (GeoVertex, GeoEdge, GeoFace, GeoSolid, etc.)
    uint32_t   localId{};       // Type-scoped local ID within the shape

    bool isValid() const;
    bool isGeometry() const;    // GeoVertex/GeoEdge/GeoWire/GeoFace/GeoSolid
    bool isMesh() const;        // MeshNode/MeshEdge/MeshElement

    bool operator==(const EntityRef&) const = default;
    auto operator<=>(const EntityRef&) const = default;
};
```

**Relationship to PickId:** `PickId::encode(shapeId, type, localId) ↔ EntityRef{shapeId, type, localId}` — zero-cost conversion.

Move `PickMask` enum from render to core (or define a core-level equivalent).

### 2. SelectionState (libs/selection)

**File:** `src/libs/selection/include/opengeolab/selection/selection_state.hpp`

Thread-safe selection state manager. Pure C++, no Qt dependency.

```cpp
class SelectionState {
public:
    // Pick configuration
    void setPickEnabled(bool enabled);
    bool pickEnabled() const;
    void setPickMask(PickMask mask);
    PickMask pickMask() const;

    // Selection management
    void addSelection(const EntityRef& entity);
    void removeSelection(const EntityRef& entity);
    void clearSelection();
    std::vector<EntityRef> selections() const;
    bool isSelected(const EntityRef& entity) const;

    // Hover management
    void setHovered(const EntityRef& entity);
    void clearHover();
    std::optional<EntityRef> hovered() const;

    // Version tracking (for render cache invalidation)
    uint64_t selectionVersion() const;
    uint64_t hoverVersion() const;

    // Signals (Core::Signal, not Qt)
    Signal<EntityRef> entitySelected;
    Signal<EntityRef> entityDeselected;
    Signal<> selectionCleared;
    Signal<EntityRef> hoverChanged;
    Signal<> pickConfigChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<EntityRef> m_selections;
    std::optional<EntityRef> m_hovered;
    PickMask m_pickMask{PickMask::None};
    bool m_pickEnabled{false};
    std::atomic<uint64_t> m_selectionVersion{0};
    std::atomic<uint64_t> m_hoverVersion{0};
};
```

**Thread safety:** `shared_mutex` — multiple readers (render synchronize, QML reads) or single writer (render pick dispatch, Python actions, QML commands).

### 3. SelectionModule (libs/selection)

**File:** `src/libs/selection/src/selection_module.cpp`

Registers actions for JSON/Python access via CommandDispatcher.

**Actions:**

| Action | Parameters | Description |
|--------|-----------|-------------|
| `select` | `{entities: [{shapeId, type, localId}], append: bool}` | Add entities to selection |
| `deselect` | `{entities: [{shapeId, type, localId}]}` | Remove entities from selection |
| `clear` | `{}` | Clear all selections |
| `query` | `{}` | Return current selections |
| `set_mode` | `{pickMask: int, enabled: bool}` | Configure pick mode |
| `hover` | `{entity: {shapeId, type, localId}}` | Set hover entity |

**Request/Response format:**
```json
// Request
{"module": "selection", "action": "select", "param": {"entities": [{"shapeId": 1, "type": "GeoFace", "localId": 3}]}}

// Response
{"ok": true, "action": "select", "selected": 1}
```

### 4. LabelManager (scene layer)

**File:** `src/libs/scene/include/opengeolab/scene/label_manager.hpp`

Manages 3D annotation labels. Independent of selection — any tool can add labels.

```cpp
struct Label3D {
    EntityRef entity;        // Which entity to attach to
    std::string text;        // Display text ("F:3", "V:1")
    glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};    // White default
    glm::vec4 bgColor{0.0f, 0.0f, 0.0f, 0.7f};       // Semi-transparent black
};

class LabelManager {
public:
    void addLabel(Label3D label);
    void removeByEntity(const EntityRef& entity);
    void clearLabels();
    std::span<const Label3D> labels() const;
    uint64_t version() const;

    Signal<> labelsChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Label3D> m_labels;
    std::atomic<uint64_t> m_version{0};
};
```

**Anchor computation:** LabelPass reads `Label3D.entity` and computes anchor position from the entity's geometry data in GpuBufferManager each frame. No 3D coordinates stored in Label3D.

**Anchor strategies by entity type:**

| EntityType | Anchor Position |
|-----------|----------------|
| GeoVertex | Vertex position from pointRange |
| GeoEdge | Midpoint of edge vertices from lineRange |
| GeoFace | Centroid of triangle vertices from triangleRange |
| GeoSolid | Average centroid of all constituent faces |

### 5. MSDF Font Rendering (render layer)

**New files:**
- `src/libs/render/src/font/font_atlas.hpp/.cpp` — MSDF atlas loader (texture + glyph metrics)
- `src/libs/render/src/pass/label_pass.hpp/.cpp` — Billboard label rendering pass

**Font Atlas:**
- Pre-generated using msdf-atlas-gen (offline tool)
- ASCII character set (0-9, A-Z, a-z, common symbols)
- Single 512×512 MSDF texture + JSON glyph metrics
- Packaged as embedded resource

**LabelPass rendering pipeline:**
1. Read labels from LabelManager
2. For each label, find entity's DrawRange in GpuBufferManager
3. Compute 3D anchor from DrawRange vertex data (centroid/midpoint/position)
4. Apply world transform (node's worldMatrix)
5. Generate billboard quads in screen space (fixed pixel size)
6. Render with MSDF fragment shader (smoothstep edge + outline)

**MSDF Fragment Shader (conceptual):**
```glsl
uniform sampler2D u_atlas;
uniform vec4 u_textColor;
uniform vec4 u_bgColor;

in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec3 msd = texture(u_atlas, v_texCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float opacity = smoothstep(0.45, 0.55, sd);
    // Background quad + text
    fragColor = mix(u_bgColor, u_textColor, opacity);
}
```

### 6. SelectionService (app layer — QML bridge)

**File:** `src/app/include/opengeolab/app/selection_service.hpp`

Thin QML singleton bridging SelectionState to QML. No business logic.

```cpp
class SelectionService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool pickEnabled READ pickEnabled WRITE setPickEnabled NOTIFY pickEnabledChanged)
    Q_PROPERTY(int pickMask READ pickMask WRITE setPickMask NOTIFY pickMaskChanged)
    Q_PROPERTY(QVariantList selections READ selections NOTIFY selectionChanged)

public:
    Q_INVOKABLE void activatePickMode(int mask);
    Q_INVOKABLE void deactivatePickMode();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void removeSelection(int shapeId, int entityType, int localId);

Q_SIGNALS:
    void entitySelected(int shapeId, int entityType, int localId);
    void entityDeselected(int shapeId, int entityType, int localId);
    void selectionCleared();
    void hoverChanged(int shapeId, int entityType, int localId);
    void pickEnabledChanged();
    void pickMaskChanged();
    void selectionChanged();
};
```

### 7. GLViewport Changes

**Mouse interaction model:**

| Action | Condition | Behavior |
|--------|-----------|----------|
| Mouse move | pickEnabled | Update hover via SelectionState.setHovered() |
| Left click (no drag) | pickEnabled | SelectionState.addSelection(pickResult) |
| Left drag | pickEnabled + distance > 4px | Box select: pickRegion() → batch addSelection() |
| Right click (no drag) | pickEnabled | SelectionState.removeSelection(pickResult) |
| Right drag | pickEnabled + distance > 4px | Box deselect: pickRegion() → batch removeSelection() |
| Middle/Ctrl+Left drag | any | Camera manipulation (existing trackball) |

**Box selection visual feedback:**
- Left drag: blue rubber-band rectangle
- Right drag: red rubber-band rectangle
- Rendered as QML overlay (not in GL) for simplicity

**Key changes to GLViewport:**
- Track drag start position for box selection
- Differentiate left-click vs left-drag for select vs box-select
- Differentiate right-click vs right-drag for deselect vs box-deselect
- Use existing `pickRegion()` for box selection resolution

### 8. Query Panel (QML)

**File:** `src/app/resource/qml/pages/GeoQueryPage.qml`

**Components:**
- `EntityTypeSelector.qml` — Pick type toggle buttons (Vertex/Edge/Face/Solid)
  - V/E/F can be combined (bitmask OR)
  - Solid is mutually exclusive with V/E/F
  - Activating Solid deselects V/E/F; activating any V/E/F deselects Solid
- Pick mode indicator (animated, shows instructions)
- Selected entities list (flow layout with removable chips)
- Clear All / Query buttons

**Business flow:**
1. User opens Query panel → pick mode activates with selected entity types
2. User clicks/box-selects entities → SelectionService signals update chip list
3. For each selected entity, Query panel calls LabelManager.addLabel({entity, text})
4. Label text format: `V:{localId}`, `E:{localId}`, `F:{localId}`, `S:{localId}`
5. User closes panel → LabelManager.clearLabels() + SelectionState.clearSelection()

**Label text mapping:**

| EntityType | Prefix | Example |
|-----------|--------|---------|
| GeoVertex | V: | V:1 |
| GeoEdge | E: | E:3 |
| GeoFace | F: | F:6 |
| GeoSolid | S: | S:1 |

### 9. Render Pipeline Changes

**New pass order:**
```
1. OpaquePass          (main geometry)
2. WireframePass       (wireframe overlay)
3. HighlightPass       (selection + hover highlighting)
4. LabelPass           (MSDF entity ID labels)  ← NEW
5. SelectionPass       (GPU picking FBO)
```

**HighlightPass changes:**
- Read SelectionState.selections() → look up DrawRanges → populate render data
- Read SelectionState.hovered() → look up DrawRanges → populate hover render data
- Highlight colors: Selected = blue (0.2, 0.4, 0.9, 0.6), Hovered = light blue (0.4, 0.7, 1.0, 0.4)

**FrameState changes:**
- Remove direct `selectedDrawRanges`/`hoveredDrawRanges` fields
- HighlightPass and LabelPass read directly from SelectionState/LabelManager references held by RenderPipeline

### 10. Data Flow Summary

```
[User Click]
    ↓
GLViewport::mouseReleaseEvent (main thread)
    ↓ pendingPick
GLViewportRenderer::render (render thread)
    ↓ pickAt() → PickResolver → EntityRef
    ↓
SelectionState.addSelection(entityRef)    ← render writes
    ↓ signal: entitySelected
    ├→ [QML] SelectionService.entitySelected → Query Panel updates chip list
    │      ↓
    │   LabelManager.addLabel({entityRef, "F:3"})    ← business logic
    │
    ├→ [Next Frame Render]
    │   HighlightPass reads SelectionState → draws blue highlight
    │   LabelPass reads LabelManager → computes anchor → draws MSDF "F:3"
    │
    └→ [Python] SelectionModule can query/modify at any time via JSON

[Model Rotation]
    ↓
Camera changes → new viewMatrix/projMatrix
    ↓
LabelPass recomputes anchor from geometry data → label follows entity
```

---

## Scope (First Version)

### In Scope
- EntityRef in core layer
- libs/selection (SelectionState + SelectionModule with actions)
- LabelManager in scene layer
- MSDF font atlas generation + LabelPass
- HighlightPass integration with SelectionState
- SelectionService QML bridge
- GeoQueryPage QML panel (Vertex/Edge/Face/Solid, mutually exclusive groups)
- Single click select/deselect
- Box select (left drag) / box deselect (right drag)
- Hover highlighting (no labels)
- Rubber-band rectangle visual feedback

### Out of Scope (Future)
- Mesh entity support (MeshNode/MeshEdge/MeshElement)
- Wire/Shell/CompSolid/Compound entity types
- Unicode/CJK character support in MSDF atlas
- Label size scaling with camera distance
- Query result details panel (property display)
- Undo/redo for selection operations
- Selection persistence across tool switches
