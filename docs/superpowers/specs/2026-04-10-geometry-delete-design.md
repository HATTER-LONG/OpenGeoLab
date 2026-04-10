# Geometry Delete Feature — Design Specification

## 1. Problem Statement

OpenGeoLab currently supports deleting **entire top-level shapes** via `delete_shape`, but lacks the
ability to remove **sub-entities** (faces, solids) from a shape. Users need two complementary
deletion workflows:

1. **Sub-entity deletion**: Select specific faces or solids in the viewport and remove them from the
   parent shape (defeaturing, assembly trimming).
2. **Whole-shape deletion from sidebar**: Right-click a shape in the Scene Explorer and delete it
   entirely — currently only possible via the command protocol, not the UI.

### Scope

- **v1**: Face and Solid deletion only.
- **Deferred**: Edge and Vertex deletion — OCC lacks direct APIs; requires complex shape healing with
  high failure rates. Interfaces will be designed to accommodate future extensions.

## 2. Architecture

### 2.1 Signal Flow

```
User picks face/solid → SelectionState (EntityRef list)
→ DeleteEntityPage shows entity chips
→ User clicks Execute
→ JSON request: { module: "geometry", action: "delete_entity", param: { entities: [...] } }
→ CommandDispatcher → GeometryModule → DeleteEntityAction
→ OCC operation (Defeaturing / compound rebuild)
→ ShapeStore::replaceShape(shapeId, newShape)
→ shapeUpdated signal
→ GeometrySceneBridge refreshes SceneNode (re-tessellate + rebuild render/pick components)
→ Viewport renders updated geometry
```

### 2.2 Component Diagram

```
┌─ QML ─────────────────────────────────────────────────────────────┐
│  RibbonConfig (Modify group: Delete button)                       │
│  → MainPages.handleAction("deleteEntity")                         │
│  → DeleteEntityPage                                               │
│     ├─ EntityTypeSelector (Face | Solid)                          │
│     ├─ Pick mode activation (scene.set_pick_mode)                 │
│     ├─ EntityChip list (from SelectionService.selections)         │
│     └─ Execute → RequestService.submitAsync(delete_entity JSON)   │
│                                                                   │
│  ShapeListItem (right-click → context menu → "Delete Shape")     │
│  → RequestService.submitAsync(delete_shape JSON)                  │
└───────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─ C++ Command Layer ───────────────────────────────────────────────┐
│  CommandDispatcher → GeometryModule.process()                     │
│  ├─ DeleteEntityAction (NEW)                                      │
│  │   ├─ Face: BRepAlgoAPI_Defeaturing                             │
│  │   └─ Solid: Compound rebuild via BRep_Builder                  │
│  └─ DeleteShapeAction (EXISTING, unchanged)                       │
└───────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─ Geometry Store ──────────────────────────────────────────────────┐
│  ShapeStore                                                       │
│  ├─ replaceShape(shapeId, newShape) [NEW]                         │
│  │   └─ Rebuilds sub-shape maps, clears tessellation cache        │
│  │   └─ Emits shapeUpdated(shapeId, entry)                        │
│  └─ remove(shapeId) [EXISTING]                                    │
└───────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─ Scene Layer (auto-sync) ─────────────────────────────────────────┐
│  GeometrySceneBridge listens to shapeUpdated                      │
│  → Rebuilds IRenderComponent + IPickComponent                     │
│  → TopologyIndex rebuilt                                          │
│  → SelectionState: stale EntityRefs cleared                       │
└───────────────────────────────────────────────────────────────────┘
```

## 3. C++ Backend Design

### 3.1 DeleteEntityAction

**Location**: `src/libs/geometry/include/opengeolab/geometry/delete_entity_action.hpp` and
`src/libs/geometry/src/delete_entity_action.cpp`

**Module**: `geometry`  
**Action name**: `delete_entity`

#### Request Format

```json
{
  "module": "geometry",
  "action": "delete_entity",
  "param": {
    "entities": [
      { "shapeId": 1, "type": "GeoFace", "localId": 3 },
      { "shapeId": 1, "type": "GeoFace", "localId": 7 },
      { "shapeId": 2, "type": "GeoSolid", "localId": 1 }
    ]
  }
}
```

#### Response Format (success)

```json
{
  "ok": true,
  "action": "delete_entity",
  "results": [
    { "shapeId": 1, "status": "modified", "removedFaces": 2 },
    { "shapeId": 2, "status": "removed", "removedSolids": 1 }
  ]
}
```

#### Response Format (partial failure)

```json
{
  "ok": false,
  "action": "delete_entity",
  "results": [
    { "shapeId": 1, "status": "failed", "error": "Defeaturing failed: cannot extend adjacent faces" }
  ]
}
```

#### Processing Logic

1. **Parse** `entities` array → group by `shapeId`.
2. **Validate** each shapeId exists in ShapeStore and each localId is within bounds.
3. **For each shapeId**, classify entities by type:
   - **GeoFace**: Collect all face sub-shapes → `BRepAlgoAPI_Defeaturing`.
   - **GeoSolid**: Collect all solid sub-shapes → rebuild compound without them.
4. **Apply** the modification:
   - If all solids in a shape are removed → call `ShapeStore::remove()` (entire shape gone).
   - Otherwise → call `ShapeStore::replaceShape()` with the modified shape.
5. **Return** per-shape status in results array.

#### Face Deletion (Defeaturing)

```cpp
BRepAlgoAPI_Defeaturing defeaturing;
defeaturing.SetShape(entry.shape);
for (const auto& face : facesToRemove) {
    defeaturing.AddFaceToRemove(face);
}
defeaturing.Build();
if (!defeaturing.IsDone()) {
    // Return error with diagnostic message
}
const TopoDS_Shape& result = defeaturing.Shape();
```

- Requires linking `TKBO` library.
- Input shape must be SOLID, COMPSOLID, or COMPOUND of solids.
- Faces must belong to the input shape.
- The algorithm extends neighboring faces to fill the gap. May fail on complex geometry.

#### Solid Deletion (Compound Rebuild)

```cpp
BRep_Builder builder;
TopoDS_Compound result;
builder.MakeCompound(result);
for (TopoDS_Iterator it(entry.shape); it.More(); it.Next()) {
    if (!solidsToRemove.Contains(it.Value())) {
        builder.Add(result, it.Value());
    }
}
```

- If the parent shape is a single solid (not compound) and it's the target → fall back to
  `ShapeStore::remove()` (equivalent to deleting the entire shape).
- If all solids are removed → `ShapeStore::remove()`.
- Otherwise → `ShapeStore::replaceShape()` with the new compound.

#### Unsupported Types

- `GeoEdge` and `GeoVertex` return an error:
  `{ "status": "unsupported", "error": "Edge/Vertex deletion is not supported in v1" }`

### 3.2 ShapeStore::replaceShape()

**New public method on ShapeStore:**

```cpp
void replaceShape(uint32_t shape_id, const TopoDS_Shape& new_shape);
```

**Behavior:**
1. Acquire lock.
2. Validate `shape_id` exists.
3. Replace `entry.shape` with `new_shape`.
4. Clear old sub-shape maps and tessellation cache.
5. Rebuild sub-shape index via `buildSubShapeIndex()`.
6. Release lock.
7. Emit `shapeUpdated(shape_id, entry)`.

This triggers the existing GeometrySceneBridge → SceneNode refresh pipeline.

**Important**: `replaceShape()` clears the tessellation cache. The caller (DeleteEntityAction) must
call `ShapeStore::tessellate(shapeId)` after `replaceShape()` to regenerate visual data. This
follows the same pattern as `CreateBoxAction` which calls `tessellate()` after `add()`. The bridge's
`onShapeUpdated` handler only rebuilds render data when `entry.visualData` is non-null.

### 3.3 CMake Changes

In `src/libs/geometry/CMakeLists.txt`, add `TKBO` to the link dependencies:

```cmake
target_link_libraries(opengeolab_geometry
  PUBLIC  opengeolab_core
  PRIVATE TKernel TKMath TKG3d TKGeomBase TKBRep TKTopAlgo TKPrim TKMesh
          TKDESTEP TKDE TKXSBase TKShHealing
          TKBO)  # NEW — for BRepAlgoAPI_Defeaturing
```

### 3.4 Selection Cleanup After Deletion

When `ShapeStore::replaceShape()` is called, the sub-shape maps change — old localIds become
invalid. The GeometrySceneBridge should clear selection for the affected shapeId on `shapeUpdated`.

This is already partially handled: when the bridge rebuilds render/pick components, the
TopologyIndex is rebuilt. We need to ensure SelectionState removes stale EntityRefs for the modified
shapeId.

**Approach**: In GeometrySceneBridge's `shapeUpdated` handler, call
`selectionState.removeSelectionsForShape(shapeId)`. This requires a new convenience method on
SelectionState (or filter + remove existing selections).

### 3.5 GeometryModule Registration

In `geometry_module.cpp`, add:

```cpp
registerAction<DeleteEntityAction>(std::ref(m_shapeStore));
```

### 3.6 describe() Schema

The `DeleteEntityAction::describe()` method returns a JSON schema matching the request/response
formats above, enabling LLM and UI introspection.

## 4. QML Frontend Design

### 4.1 RibbonConfig.qml

Add a `Delete` entry to the Geometry tab's **Modify** group:

```js
{
    "key": "deleteEntity",
    "title": qsTr("Delete"),
    "icon": "trash",
    "accentOne": "accentD",
    "accentTwo": "accentC"
}
```

Uses the existing `trash.svg` icon already in the project's icons folder.

### 4.2 DeleteEntityPage.qml

**Location**: `src/app/resource/qml/components/pages/DeleteEntityPage.qml`

Extends `FunctionPageBase`. Visual layout follows `GeoQueryPage` conventions.

**Structure:**
1. **Entity Filter** — `EntityTypeSelector` configured for Face + Solid only (mask = Face | Solid =
   8 | 16 = 24). Vertex and Edge buttons hidden.
2. **Pick mode indicator** — Pulsing dot + instruction text (reused from GeoQueryPage pattern).
3. **Selected entities list** — `EntityChip` flow (from `SelectionService.selections`), scrollable.
4. **Empty state** — Prompt text when no entities selected.
5. **Clear All button** — Clears selection.
6. **Warning banner** — Shows when entities from unsupported types are selected.

**Lifecycle:**
- `open()`: Activate pick mode with Face|Solid mask, clear previous selection.
- `close()`: Deactivate pick mode, clear selection, clear labels.
- `execute()`: Build `delete_entity` request from current SelectionService.selections, submit via
  RequestService, close page.
- `getParameters()`: Constructs the JSON request with entity array.

**Entity type mapping** (entityType int → protocol string):
- 3 → "GeoFace"
- 4 → "GeoSolid"

### 4.3 MainPages.qml Registration

Add to `componentMap`:

```js
"deleteEntity": { path: "components/pages/DeleteEntityPage.qml" }
```

### 4.4 Sidebar Context Menu (ShapeListItem)

Add right-click handling to `ShapeListItem.qml`:

- On right-click → show `Menu` (Qt Quick Controls) with a single item: "Delete Shape".
- "Delete Shape" sends `{ module: "geometry", action: "delete_shape", param: { shapeId: N } }`.
- Uses existing `RequestService.submitAsync()`.

**Implementation**: Add `MouseArea` with `acceptedButtons: Qt.RightButton` (or extend existing
MouseArea to handle right-click), show a `Menu { MenuItem { text: qsTr("Delete Shape") } }`.

### 4.5 Translation (i18n)

New translatable strings to add to `opengeolab_zh_CN.ts`:

| Context | Source | Translation |
|---------|--------|-------------|
| DeleteEntityPage | "Delete Entity" | "删除实体" |
| DeleteEntityPage | "Entity Filter" | "实体过滤" |
| DeleteEntityPage | "Click entities in the viewport..." | "在视口中点击实体以选中..." |
| DeleteEntityPage | "Selected: %1" | "已选中: %1" |
| DeleteEntityPage | "Clear All" | "全部清除" |
| RibbonConfig | "Delete" | "删除" |
| ShapeListItem | "Delete Shape" | "删除形体" |

## 5. Error Handling

### 5.1 Defeaturing Failures

`BRepAlgoAPI_Defeaturing` may fail when:
- Adjacent faces cannot be extended to fill the gap (e.g., tangent surfaces).
- The resulting shape would be topologically invalid.
- The face is the last face of a solid.

**Strategy**: Return per-shape error in the response JSON. The frontend shows the error in the
terminal/activity log. The shape remains unchanged.

### 5.2 Edge Cases

| Scenario | Behavior |
|----------|----------|
| Delete all solids from a compound | Falls back to `ShapeStore::remove()` — deletes entire shape |
| Delete the only solid in a shape | Falls back to `ShapeStore::remove()` |
| Delete face from non-solid shape | Returns error: "Defeaturing requires a solid shape" |
| Mixed types in one request | Processes each type independently, reports per-shape status |
| Invalid shapeId or localId | Returns error for that entity, continues processing others |
| Empty entities array | Returns `{ ok: false, error: "No entities specified" }` |

## 6. Testing Strategy

### 6.1 Unit Tests (C++)

- `DeleteEntityAction` with face removal from a box (6 faces → 5 faces after removing one).
- `DeleteEntityAction` with solid removal from a compound of two boxes.
- `ShapeStore::replaceShape()` — verify sub-shape maps rebuilt, signals emitted.
- Error cases: invalid shapeId, invalid localId, unsupported entity type, defeaturing failure.

### 6.2 Integration Tests

- Full pipeline: create box → select face → delete_entity → verify shape updated in store.
- Sidebar delete: create box → delete_shape via protocol → verify shape removed.

## 7. Files Changed (Summary)

### New Files
- `src/libs/geometry/include/opengeolab/geometry/delete_entity_action.hpp`
- `src/libs/geometry/src/delete_entity_action.cpp`
- `src/app/resource/qml/components/pages/DeleteEntityPage.qml`

### Modified Files
- `src/libs/geometry/CMakeLists.txt` — add `TKBO` link dependency
- `src/libs/geometry/include/opengeolab/geometry/shape_store.hpp` — add `replaceShape()`
- `src/libs/geometry/src/shape_store.cpp` — implement `replaceShape()`
- `src/libs/geometry/src/geometry_module.cpp` — register `DeleteEntityAction`
- `src/app/resource/qml/RibbonConfig.qml` — add Delete to Modify group
- `src/app/resource/qml/MainPages.qml` — register DeleteEntityPage
- `src/app/resource/qml/components/ShapeListItem.qml` — add right-click context menu
- `src/app/resource/translations/opengeolab_zh_CN.ts` — add new translation strings

### Unchanged (verified compatible)
- `DeleteShapeAction` — unchanged, still handles whole-shape deletion
- `GeometrySceneBridge` — already handles `shapeUpdated` signal
- `SelectionState` — may need minor addition for `removeSelectionsForShape()`
- `EntityTypeSelector` — reused as-is with different mask configuration
