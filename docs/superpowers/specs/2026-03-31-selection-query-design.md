# Selection & Geometry Query Design

## Problem Statement

OpenGeoLabNew needs a selection/picking system with a Geometry Query tool panel. The system must:
1. Provide a reusable selection infrastructure (hover, select, box-select)
2. Support Python/JSON access for programmatic selection control
3. Separate selection (generic capability) from label display (business behavior)
4. Render entity ID labels (F:1, V:1, E:3, S:1) attached to selected entities in the 3D viewport (Phase 2)

## Implementation Phases

- **Phase 1**: Selection infrastructure + entity highlight + GeoQueryPage (chip list, no labels)
- **Phase 2**: MSDF font rendering + LabelPass + label display on selected entities

---

## Architecture Overview

### Dependency Chain (unidirectional, no cycles)

```
core            ← EntityRef, EntityTag, EntityType, PickMask
  ↑
scene           ← SceneGraph, SelectionState, LabelManager, TopologyIndex, DrawRange
  ↑
render          ← HighlightPass, LabelPass(Phase 2), SelectionPass, PickResolver
  ↑
app             ← SelectionService (QML bridge), GLViewport, GeoQueryPage
```

No new libraries are introduced. `SelectionState` lives in the scene library as a member of `SceneGraph`, accessible via `sceneGraph.selectionState()`.
Selection actions are registered as sub-actions of the existing `SceneModule`.

### Key Design Decisions

1. **SelectionState as SceneGraph member** — accessible via `sceneGraph.selectionState()`. SceneGraph retains node-level selection for tree-view highlighting; SelectionState handles entity-level selection for 3D picking. GLViewportRenderer accesses it through the existing `scene.selectionState()` path during `synchronize()`. No new library needed.
2. **Selection actions as SceneModule sub-actions** — accessed via `{"module": "scene", "action": "select", ...}`. No separate SelectionModule.
3. **EntityRef in core** — `{shapeId, entityType, localId}` serves as the globally unique entity address. Coexists with `EntityTag{type, localId}` which is a shape-scoped relative address.
4. **PickMask moved to core** — scene needs it but must not depend on render.
5. **FrameState snapshot + version-based dirty tracking** — selection/hover/label data is resolved during `synchronize()` only when versions change, and passed to render passes via FrameState. Render passes never lock or access scene-layer objects directly. This extends the existing dirty-tracking pattern used by GpuBufferManager.
6. **LabelManager in scene** — label display is scene annotation, independent of selection.
7. **Auto-anchor computation** — UI only passes EntityRef + text; render computes 3D anchor from geometry (Phase 2).
8. **Selection ≠ Labels** — Selection manages hover/select state + highlight; Query panel orchestrates label creation as business logic (Phase 2).
9. **MSDF text rendering** (Phase 2) — for entity ID labels, supports future complex symbols for LLM identification.

### Data Flow Principles

All derived render data uses version-based dirty tracking to avoid per-frame recomputation:

| Data | Source | Version Field | When Recomputed |
|------|--------|---------------|-----------------|
| GPU geometry buffers | SceneGraph | `scene.version()` vs `m_uploadedVersion` | Node add/remove/update (existing) |
| Selected DrawRanges | SelectionState | `selectionVersion()` vs cached | Entity selected/deselected |
| Hovered DrawRanges | SelectionState | `hoverVersion()` vs cached | Hover entity changed |
| Label render data | LabelManager | `version()` vs cached | Label add/remove (Phase 2) |
| Camera/viewport | GLViewport | — | Every frame (cheap copy) |

---

## Component Specifications

### 1. EntityRef (core layer)

**File:** `src/libs/core/include/opengeolab/core/entity_ref.hpp`

Universal entity reference used across all layers. Globally unique "absolute address" for any entity in the scene.

```cpp
struct EntityRef {
    uint32_t   shapeId{};       ///< Owning shape/part ID
    EntityType entityType{};    ///< Entity type (GeoVertex, GeoEdge, GeoFace, GeoSolid, MeshNode, etc.)
    uint32_t   localId{};       ///< Type-scoped local ID within the shape

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isGeometry() const;    ///< GeoVertex/GeoEdge/GeoWire/GeoFace/GeoSolid
    [[nodiscard]] bool isMesh() const;        ///< MeshNode/MeshEdge/MeshElement

    bool operator==(const EntityRef&) const = default;
    auto operator<=>(const EntityRef&) const = default;
};
```

**Relationship to existing types:**
- `EntityRef` = shapeId + `EntityTag`. EntityTag is a shape-scoped relative address; EntityRef is a scene-wide absolute address.
- `PickId::encode(shapeId, type, localId) ↔ EntityRef{shapeId, type, localId}` — zero-cost conversion.
- `PickResult` (render) remains as the render-layer pick output with an additional `valid` field. Conversion: `EntityRef{result.shapeId, result.entityType, result.localId}`.

**Extensibility:** The `{shapeId, EntityType, localId}` triple supports both geometry (GeoVertex/GeoEdge/GeoFace/GeoSolid) and mesh entities (MeshNode/MeshEdge/MeshElement). Future mesh element subtypes only require extending the EntityType enum (8-bit, 256 slots).

### 2. PickMask (core layer)

**Move from:** `src/libs/render/include/opengeolab/render/pick_mask.hpp`
**Move to:** `src/libs/core/include/opengeolab/core/pick_mask.hpp`

```cpp
enum class PickMask : uint32_t {
    None    = 0,
    Vertex  = 1 << 0,
    Edge    = 1 << 1,
    Wire    = 1 << 2,
    Face    = 1 << 3,
    Solid   = 1 << 4,
    Part    = 1 << 5,
    All     = 0xFFFFFFFF,
};

enum class PickMode : uint8_t {
    VEF,    ///< Vertex > Edge > Face priority
    Wire,   ///< Edge → resolve to Wire
    Solid,  ///< Face → resolve to Solid
    Part,   ///< Any → resolve to Part (shapeId)
};
```

Render layer headers update to `#include <opengeolab/core/pick_mask.hpp>`. Bitwise operators (`|`, `&`) preserved.

### 3. SelectionState (scene layer)

**File:** `src/libs/scene/include/opengeolab/scene/selection_state.hpp`

Thread-safe entity-level selection state manager. Pure C++, no Qt dependency.
Coexists with SceneGraph's node-level selection (which remains for tree-view highlighting).

```cpp
class SelectionState {
public:
    // Pick configuration
    void setPickEnabled(bool enabled);
    [[nodiscard]] bool pickEnabled() const;
    void setPickMask(PickMask mask);
    [[nodiscard]] PickMask pickMask() const;

    // Selection management
    void addSelection(const EntityRef& entity);
    void removeSelection(const EntityRef& entity);
    void clearSelection();
    [[nodiscard]] std::vector<EntityRef> selections() const;
    [[nodiscard]] bool isSelected(const EntityRef& entity) const;

    // Hover management
    void setHovered(const EntityRef& entity);
    void clearHover();
    [[nodiscard]] std::optional<EntityRef> hovered() const;

    // Version tracking (for render dirty-check)
    [[nodiscard]] uint64_t selectionVersion() const;
    [[nodiscard]] uint64_t hoverVersion() const;

    // Signals (Kangaroo::Util::Signal)
    Kangaroo::Util::Signal<EntityRef> entitySelected;
    Kangaroo::Util::Signal<EntityRef> entityDeselected;
    Kangaroo::Util::Signal<> selectionCleared;
    Kangaroo::Util::Signal<EntityRef> hoverChanged;
    Kangaroo::Util::Signal<> pickConfigChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<EntityRef> m_selections;  // Sorted for O(log n) lookup; consider flat_set if box-select volumes grow large
    std::optional<EntityRef> m_hovered;
    PickMask m_pickMask{PickMask::None};
    bool m_pickEnabled{false};
    std::atomic<uint64_t> m_selectionVersion{0};
    std::atomic<uint64_t> m_hoverVersion{0};
};
```

**Thread safety:** `shared_mutex` — multiple concurrent readers (render synchronize, QML reads) or single writer (render pick dispatch, Python actions, QML commands). Writers increment version atomically.

**Ownership:** `SceneGraph` creates and owns the `SelectionState` instance, exposed via accessor:
```cpp
[[nodiscard]] SelectionState& selectionState();
[[nodiscard]] const SelectionState& selectionState() const;
```

This enables GLViewportRenderer to access selection state through the existing `scene.selectionState()` path during `synchronize()`, consistent with the unidirectional data flow pattern.

### 4. Selection Actions (SceneModule sub-actions)

Registered in `SceneModule` constructor alongside existing `list_nodes` and `set_visibility` actions.

**Actions:**

| Action | Module | Parameters | Description |
|--------|--------|-----------|-------------|
| `select` | `scene` | `{entities: [{shapeId, type, localId}], append: bool}` | Add entities to selection |
| `deselect` | `scene` | `{entities: [{shapeId, type, localId}]}` | Remove entities from selection |
| `clear_selection` | `scene` | `{}` | Clear all selections |
| `query_selection` | `scene` | `{}` | Return current selections |
| `set_pick_mode` | `scene` | `{pickMask: int, pickMode: string, enabled: bool}` | Configure pick mode. `pickMode` is one of "VEF"/"Wire"/"Solid"/"Part"; if omitted, derived from pickMask (V/E/F bits → VEF, Solid bit → Solid, etc.) |
| `set_hover` | `scene` | `{entity: {shapeId, type, localId}}` | Set hover entity |

**Request/Response format:**
```json
// Request
{"module": "scene", "action": "select", "param": {"entities": [{"shapeId": 1, "type": "GeoFace", "localId": 3}], "append": true}}

// Response
{"ok": true, "action": "select", "selected": 1}
```

### 5. LabelManager (scene layer)

**File:** `src/libs/scene/include/opengeolab/scene/label_manager.hpp`

Manages 3D annotation labels. Independent of selection — any tool can add labels.

```cpp
struct Label3D {
    EntityRef entity;        ///< Which entity to attach to
    std::string text;        ///< Display text ("F:3", "V:1")
    glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};    ///< White default
    glm::vec4 bgColor{0.0f, 0.0f, 0.0f, 0.7f};       ///< Semi-transparent black
};

class LabelManager {
public:
    void addLabel(Label3D label);
    void removeByEntity(const EntityRef& entity);
    void clearLabels();
    [[nodiscard]] std::vector<Label3D> labels() const;
    [[nodiscard]] uint64_t version() const;

    Kangaroo::Util::Signal<> labelsChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Label3D> m_labels;
    std::atomic<uint64_t> m_version{0};
};
```

**Ownership:** `SceneGraph` creates and owns the `LabelManager` instance, exposed via accessor (alongside SelectionState).

**Anchor computation (Phase 2):** LabelPass reads `Label3D.entity` and computes anchor position from the entity's geometry data in GpuBufferManager. No 3D coordinates stored in Label3D.

**Anchor strategies by entity type:**

| EntityType | Anchor Position |
|-----------|----------------|
| GeoVertex | Vertex position from pointRange |
| GeoEdge | Midpoint of edge vertices from lineRange |
| GeoFace | Centroid of triangle vertices from triangleRange |
| GeoSolid | Average centroid of all constituent faces |

### 6. MSDF Font Rendering (render layer) — Phase 2

**New files:**
- `src/libs/render/src/font/font_atlas.hpp/.cpp` — MSDF atlas loader (texture + glyph metrics)
- `src/libs/render/src/pass/label_pass.hpp/.cpp` — Billboard label rendering pass

**Font Atlas:**
- Pre-generated using msdf-atlas-gen (offline tool)
- ASCII character set (0-9, A-Z, a-z, common symbols)
- Single 512×512 MSDF texture + JSON glyph metrics
- Packaged as embedded resource

**LabelPass rendering pipeline:**
1. Read resolved label data from FrameState (snapshot from synchronize)
2. For each label, compute 3D anchor from DrawRange vertex data (centroid/midpoint/position)
3. Apply world transform (node's worldMatrix)
4. Generate billboard quads in screen space (fixed pixel size)
5. Render with MSDF fragment shader (smoothstep edge + outline)

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
    fragColor = mix(u_bgColor, u_textColor, opacity);
}
```

### 7. SelectionService (app layer — QML bridge)

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

### 8. GLViewport Changes

**Mouse interaction model:**

| Action | Condition | Behavior |
|--------|-----------|----------|
| Mouse move | pickingEnabled | Update hover via existing signals; when selectionActive, also update SelectionState.setHovered() |
| Left click (no drag) | pickingEnabled | Emit entityPicked signal; when selectionActive, also SelectionState.addSelection(pickResult) |
| Left drag | selectionActive + distance > 4px | Box select: pickRect() → batch addSelection() |
| Right click (no drag) | selectionActive | SelectionState.removeSelection(pickResult) |
| Right drag | selectionActive + distance > 4px | Box deselect: pickRect() → batch removeSelection() |
| Ctrl+Left drag | any | Camera orbit (existing trackball) |
| Shift+Left drag / Middle drag | any | Camera pan (existing trackball) |
| Ctrl+Wheel | any | Camera zoom (existing wheelZoom) |
| Right drag | !selectionActive | Camera zoom (existing trackball) |

**Selection mode distinction:** `pickingEnabled` (existing, default true) controls GPU pick FBO readback for hover and click signals. `selectionActive` (new, default false, synced from `SelectionState::pickEnabled()`) controls selection-specific behaviors: right-click deselect, box select/deselect, right-drag reassignment from zoom to box-deselect.

**Camera control in selection mode:** When selectionActive, right-drag is reassigned from camera zoom to box-deselect. Zoom remains available via Ctrl+Wheel. Orbit and pan are unaffected (Ctrl+Left, Shift+Left, Middle).

**When selectionActive is false:** All camera controls work as before (including right-drag zoom). Hover picking still works via pickingEnabled.

**Box selection visual feedback:**
- Left drag: blue rubber-band rectangle
- Right drag: red rubber-band rectangle
- Rendered as QML overlay (not in GL) for simplicity

**Key changes to GLViewport:**
- Extend PendingPick to include `PickAction` field (Add/Remove):
  ```cpp
  struct PendingPick {
      bool active{false};
      float x{0.0F}, y{0.0F};
      PickAction action{PickAction::Add};  ///< Add (left-click) or Remove (right-click)
  };
  ```
- Add `PendingBoxSelect` for drag-based box selection:
  ```cpp
  struct PendingBoxSelect {
      bool active{false};
      float x1{0.0F}, y1{0.0F};  ///< Drag start (item space)
      float x2{0.0F}, y2{0.0F};  ///< Drag end (item space)
      PickAction action{PickAction::Add};
  };
  ```
- Track drag start position for box selection
- Differentiate left-click vs left-drag for select vs box-select
- Differentiate right-click vs right-drag for deselect vs box-deselect
- Add `pickRect(x1, y1, x2, y2, mask)` to PickFbo for rectangular region readback
- When pickEnabled, right-drag no longer triggers camera zoom (reassigned to box-deselect)
- Ensure right-button click (no drag) triggers deselect
- Camera zoom in pick mode available only via Ctrl+Wheel

### 9. Query Panel (QML) — Phase 1

**File:** `src/app/resource/qml/components/pages/GeoQueryPage.qml`

**Components:**
- `EntityTypeSelector.qml` — Pick type toggle buttons (Vertex/Edge/Face/Solid)
  - V/E/F can be combined (bitmask OR)
  - Solid is mutually exclusive with V/E/F
  - Activating Solid deselects V/E/F; activating any V/E/F deselects Solid
- Pick mode indicator (animated, shows instructions)
- Selected entities list (flow layout with removable chips)
- Clear All button

**Business flow (Phase 1):**
1. User opens Query panel → pick mode activates with selected entity types
2. User clicks/box-selects entities → SelectionService signals update chip list
3. Selected entities displayed as removable chips with highlight in viewport
4. User closes panel → SelectionState.clearSelection()

**Business flow addition (Phase 2):**
3b. For each selected entity, Query panel calls LabelManager.addLabel({entity, text})
5. User closes panel → LabelManager.clearLabels() + SelectionState.clearSelection()

**Label text mapping (Phase 2):**

| EntityType | Prefix | Example |
|-----------|--------|---------|
| GeoVertex | V: | V:1 |
| GeoEdge | E: | E:3 |
| GeoFace | F: | F:6 |
| GeoSolid | S: | S:1 |

### 10. Render Pipeline Changes

**Pass order (Phase 1):**
```
1. OpaquePass          (main geometry)
2. WireframePass       (wireframe overlay)
3. HighlightPass       (selection + hover highlighting)
4. SelectionPass       (GPU picking FBO)
```

**Pass order (Phase 2 addition):**
```
1. OpaquePass          (main geometry)
2. WireframePass       (wireframe overlay)
3. HighlightPass       (selection + hover highlighting)
4. LabelPass           (MSDF entity ID labels)  ← NEW in Phase 2
5. SelectionPass       (GPU picking FBO)
```

**FrameState changes:**
- `selectedDrawRanges` and `hoveredDrawRanges` remain in FrameState but are now populated from SelectionState entity-level data (previously from node-level SceneGraph selection)
- Resolution: EntityRef → DrawRange lookup via GpuBufferManager index during `synchronize()`
- Dirty tracking: only re-resolved when `selectionVersion`/`hoverVersion` changes
- Phase 2 adds label snapshot data to FrameState

**HighlightPass changes:**
- No change to HighlightPass itself — it still reads DrawRanges from FrameState
- The change is upstream: `synchronize()` now resolves entity-level EntityRef → DrawRange instead of node-level selection
- Highlight colors: Selected = blue (0.2, 0.4, 0.9, 0.6), Hovered = light blue (0.4, 0.7, 1.0, 0.4)

**EntityRef → DrawRange resolution:**
GpuBufferManager builds an index structure during `synchronize()` alongside DrawRange vectors. The index maps `(shapeId, entityType, localId)` to a list of DrawRanges (an entity may span multiple primitive types — triangles + lines + points).

```cpp
struct EntityRefKey {
    uint32_t shapeId;
    Core::EntityType entityType;
    uint32_t localId;
    bool operator==(const EntityRefKey&) const = default;
};

// Hash specialization for unordered_map usage
// Index rebuilt when scene version changes (same timing as buffer re-upload)
std::unordered_map<EntityRefKey, std::vector<DrawRange*>, EntityRefKeyHash> m_entityIndex;
```

During `synchronize()`, GLViewportRenderer reads `scene.selectionState()` and resolves selected/hovered EntityRefs to DrawRanges via this index, populating `FrameState::selectedDrawRanges` and `hoveredDrawRanges`. Resolution only runs when `selectionVersion`/`hoverVersion` has changed.

### 11. Data Flow Summary

```
[User Click]
    ↓
GLViewport::mouseReleaseEvent (main thread)
    ↓ pendingPick{active, x, y, action=Add|Remove}
GLViewportRenderer::synchronize (render thread, GUI blocked)
    ↓ consume pendingPick
GLViewportRenderer::render (render thread)
    ↓ pickAt() → PickResolver → PickResult
    ↓ convert to EntityRef
    ↓ dispatch to main thread via QMetaObject::invokeMethod
    ↓
Main thread: SelectionState.addSelection(entityRef) [or removeSelection]
    ↓ version++ → signal: entitySelected
    ├→ [QML] SelectionService.entitySelected → Query Panel updates chip list
    │      ↓ (Phase 2)
    │   LabelManager.addLabel({entityRef, "F:3"})
    │
    ├→ [Next synchronize()]
    │   selectionVersion changed → resolve EntityRef → DrawRange via GpuBufferManager index → FrameState
    │   HighlightPass draws blue highlight from FrameState
    │   (Phase 2) LabelPass draws MSDF label from FrameState
    │
    └→ [Python] scene.select / scene.query_selection via JSON

[Box Select]
    ↓
GLViewport left-drag (distance > 4px)
    ↓ pendingBoxSelect{x1, y1, x2, y2, action=Add}
GLViewportRenderer::render
    ↓ pickRect(x1, y1, x2, y2) → PickFbo::readPickRect → vector<uint64_t>
    ↓ PickResolver::resolveAll → vector<PickResult>
    ↓ dispatch all to main thread
    ↓
Main thread: batch SelectionState.addSelection(...)
```

---

## Scope

### Phase 1 — Selection Infrastructure + Highlight + Query Panel
- EntityRef in core layer
- PickMask moved from render to core
- PickAction enum in core (Add/Remove)
- SelectionState as SceneGraph member (entity-level selection)
- Selection actions as SceneModule sub-actions (select, deselect, clear_selection, query_selection, set_pick_mode, set_hover)
- LabelManager in scene layer (data structure only, no rendering)
- EntityRef → DrawRange index in GpuBufferManager
- HighlightPass integration via FrameState snapshot + dirty tracking
- PickFbo: rectangular region readback (`readPickRect`)
- GLViewport: single click select/deselect, box select/deselect, hover
- GLViewport: right-drag reassigned from zoom to box-deselect (when pickEnabled)
- PendingPick extended with PickAction; new PendingBoxSelect struct
- Rubber-band rectangle visual feedback (QML overlay)
- SelectionService QML bridge
- GeoQueryPage QML panel (EntityTypeSelector, chip list, highlight — no labels)

### Phase 2 — MSDF Label Rendering
- MSDF font atlas generation (msdf-atlas-gen, ASCII, 512×512)
- FontAtlas loader (texture + glyph metrics JSON)
- LabelPass (billboard rendering, anchor computation, MSDF shader)
- GeoQueryPage: trigger LabelManager.addLabel on selection, clearLabels on close
- Label data snapshot in FrameState with version-based dirty tracking

### Out of Scope (Future)
- Mesh entity picking support (MeshNode/MeshEdge/MeshElement)
- Wire/Shell/CompSolid/Compound entity type picking
- Unicode/CJK character support in MSDF atlas
- Label size scaling with camera distance
- Query result details panel (property display)
- Undo/redo for selection operations
- Selection persistence across tool switches
