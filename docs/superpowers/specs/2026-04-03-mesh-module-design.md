# Mesh Module Design — OpenGeoLab

> **Date**: 2026-04-03
> **Status**: Approved
> **Scope**: gmsh-based mesh generation, mesh data management, mesh query UI

---

## 1. Overview

Add a dedicated **mesh module** (`src/libs/mesh/`) to OpenGeoLab that provides:

1. **Gmsh-based mesh generation** — pick geometry Face/Solid entities, configure meshing
   parameters, run gmsh, store results.
2. **Mesh Generate Panel** — QML page for selecting geometry, setting parameters, and
   triggering mesh generation.
3. **Mesh Query Panel** — QML page (analogous to Geometry Query) for picking mesh
   nodes/edges/elements with hover highlight, selection highlight, and label display.

The mesh module is **independent** from the geometry module but **shares** the scene
module for rendering, selection, hover, labels, and picking — all of which were designed
with mesh extensibility in mind.

### Design Principles

- Mesh data is stored in an independent `MeshStore`, grouped by source `shapeId`.
- Contiguous `std::vector` storage with `localId = index + 1` for O(1) lookup — no maps.
- Existing `EntityType::MeshNode/MeshEdge/MeshElement`, `PickMask`, `PickId`,
  `SelectionState`, `LabelManager` are reused without modification.
- Existing render passes (opaque, wireframe, highlight, selection, label) handle mesh
  data via the same `RenderMeshData` / `IRenderComponent` interface.
- OGL mesh code is reference only — not copied; adapted to OpenGeoLabNew's framework.

---

## 2. Module Structure

```
src/libs/mesh/
├── include/opengeolab/mesh/
│   ├── mesh_module.hpp            // MeshModule : Core::ModuleBase
│   ├── mesh_store.hpp             // MeshStore — per-shapeId grouped storage
│   ├── mesh_entry.hpp             // MeshEntry — one group's data
│   ├── mesh_node.hpp              // MeshNode value type
│   ├── mesh_element.hpp           // MeshElement value type
│   └── mesh_element_type.hpp      // enum class MeshElementType
├── src/
│   ├── mesh_module.cpp
│   ├── mesh_store.cpp
│   ├── mesh_scene_bridge.hpp/cpp  // MeshStore → SceneGraph sync
│   ├── mesh_render_builder.hpp/cpp // MeshEntry → RenderMeshData
│   └── action/
│       ├── generate_mesh_action.hpp/cpp
│       ├── clear_mesh_action.hpp/cpp
│       └── query_mesh_info_action.hpp/cpp
├── test/
│   └── mesh_store_test.cpp        // doctest unit tests
└── CMakeLists.txt
```

### Dependencies

```
mesh → core       (ModuleBase, IAction, EntityRef, EntityType, PickId)
mesh → geometry   (ShapeStore::subShape() for OCC shape access)
mesh → scene      (SceneGraph, IRenderComponent, RenderMeshData)
```

Build order: core → geometry → scene → **mesh** → render → command → app.

---

## 3. Data Model

### 3.1 MeshElementType

```cpp
enum class MeshElementType : uint8_t {
    Triangle = 0,  ///< 3-node triangle (2D)
    Quad     = 1,  ///< 4-node quadrilateral (2D)
    Tetra    = 2,  ///< 4-node tetrahedron (3D)
    Hexa     = 3,  ///< 8-node hexahedron (3D)
    Prism    = 4,  ///< 6-node prism/wedge (3D)
    Pyramid  = 5,  ///< 5-node pyramid (3D)
};
```

Utility: `nodeCount(MeshElementType)` returns 3/4/4/8/6/5.

### 3.2 MeshNode

```cpp
struct MeshNode {
    float position[3];  ///< World-space coordinates
};
```

- Stored contiguously in `MeshEntry::nodes`.
- `localId = vector_index + 1` (1-based, consistent with geometry convention).
- No per-node ID member — position in the vector *is* the identity.

### 3.3 MeshElement

```cpp
struct MeshElement {
    MeshElementType type;                   ///< Element topology
    std::array<uint32_t, 8> nodeLocalIds{}; ///< Node connectivity (localIds, first N valid)
};
```

- Stored contiguously in `MeshEntry::elements`.
- `localId = vector_index + 1`.
- `nodeLocalIds` references into the same `MeshEntry::nodes` vector.
- First `nodeCount(type)` entries are valid; remaining are zero-filled.

### 3.4 MeshEntry

```cpp
struct MeshEntry {
    uint32_t shapeId;                  ///< Source geometry shape ID
    std::vector<MeshNode> nodes;       ///< All nodes for this shape's mesh
    std::vector<MeshElement> elements; ///< All elements for this shape's mesh
    uint64_t version{0};               ///< Monotonic dirty counter

    void markUpdated() { ++version; }
};
```

Edge data (for wireframe rendering) is extracted on-the-fly in `MeshRenderBuilder` and
not persisted — avoids duplication and keeps storage minimal.

### 3.5 MeshStore

```cpp
class MeshStore {
public:
    /// Replace or create mesh data for a shape. Emits meshAdded.
    void setMesh(uint32_t shapeId, MeshEntry entry);

    /// Remove mesh data for a shape. Emits meshRemoved.
    void removeMesh(uint32_t shapeId);

    /// Remove all mesh data. Emits storeCleared.
    void clear();

    /// O(1) lookup. Returns nullptr if no mesh for this shapeId.
    [[nodiscard]] const MeshEntry* find(uint32_t shapeId) const;

    /// All shape IDs that have mesh data.
    [[nodiscard]] std::vector<uint32_t> allShapeIds() const;

    // Signals
    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshAdded;
    Kangaroo::Util::Signal<uint32_t> meshRemoved;
    Kangaroo::Util::Signal<> storeCleared;

private:
    std::unordered_map<uint32_t, MeshEntry> m_entries;
    mutable std::shared_mutex m_mutex;
};
```

**Query pattern**: `store.find(shapeId)->nodes[localId - 1]` — O(1).

---

## 4. MeshModule

```cpp
class MeshModule : public Core::ModuleBase {
public:
    explicit MeshModule(Kangaroo::PluginComponentFactory& factory);

    [[nodiscard]] MeshStore& meshStore();

    /// Deferred initialization — called after geometry and scene modules are ready.
    void initBridge(SceneGraph& scene, Geometry::ShapeStore& shapeStore);

private:
    MeshStore m_meshStore;
    std::unique_ptr<MeshSceneBridge> m_bridge;
};
```

### Registered Actions

| Action Key | Class | Purpose |
|-----------|-------|---------|
| `mesh.generate_mesh` | `GenerateMeshAction` | Gmsh meshing of picked faces/solids |
| `mesh.clear_mesh` | `ClearMeshAction` | Clear mesh for shape or all |
| `mesh.query_mesh_info` | `QueryMeshInfoAction` | Return node/element details |

### Module Registration

In `module_registry.cpp`:
```cpp
factory.bindSingleton<Core::ModuleBase, Mesh::MeshModule>("mesh", factory);
```

In `main.cpp` wiring (after geometry and scene modules):
```cpp
meshModule.initBridge(sceneModule.sceneGraph(), geoModule.shapeStore());
```

---

## 5. Actions

### 5.1 generate_mesh

**Request**:
```json
{
  "module": "mesh",
  "action": "generate_mesh",
  "param": {
    "entities": [
      {"shapeId": 1, "type": "GeoFace", "localId": 2},
      {"shapeId": 1, "type": "GeoFace", "localId": 5},
      {"shapeId": 2, "type": "GeoSolid", "localId": 1}
    ],
    "elementSize": 1.0,
    "dimension": 2,
    "elementType": "triangle",
    "algorithm": "delaunay",
    "advanced": {
      "minSize": 0.1,
      "maxSize": 10.0,
      "order": 1,
      "optimize": true
    }
  }
}
```

**Algorithm**:

1. Parse and validate entities and mesh parameters.
2. Group entities by `shapeId`.
3. For each group:
   a. Collect OCC shapes via `ShapeStore::subShape(shapeId, type, localId)`.
   b. Build `TopoDS_Compound` from collected shapes.
   c. Initialize gmsh session (scoped RAII guard).
   d. Import compound via `gmsh::model::occ::importShapesNativePointer()`.
   e. Apply mesh parameters (element size, algorithm, etc.).
   f. Generate mesh: `gmsh::model::mesh::generate(dimension)`.
   g. Extract nodes → `std::vector<MeshNode>`.
   h. Extract elements → `std::vector<MeshElement>` (map gmsh element types).
   i. Build `MeshEntry`, write to `MeshStore::setMesh(shapeId, entry)`.
4. Return success with per-shape node/element counts.

**Gmsh Element Type Mapping**:

| Gmsh Code | MeshElementType |
|-----------|----------------|
| 2 | Triangle |
| 3 | Quad |
| 4 | Tetra |
| 5 | Hexa |
| 6 | Prism |
| 7 | Pyramid |

**Gmsh Algorithm Mapping (2D)**:

| Name | Gmsh ID |
|------|---------|
| `automatic` | 2 |
| `meshadapt` | 1 |
| `delaunay` | 5 |
| `frontal` | 6 |
| `bamg` | 7 |
| `frontal_quad` | 8 |

**Gmsh Algorithm Mapping (3D)**:

| Name | Gmsh ID |
|------|---------|
| `delaunay` | 1 |
| `frontal` | 4 |
| `mmg3d` | 7 |
| `rtree` | 9 |
| `hxt` | 10 |

**Response**:
```json
{
  "ok": true,
  "action": "generate_mesh",
  "results": [
    {"shapeId": 1, "nodeCount": 234, "elementCount": 456},
    {"shapeId": 2, "nodeCount": 120, "elementCount": 200}
  ]
}
```

### 5.2 clear_mesh

**Request**:
```json
{
  "module": "mesh",
  "action": "clear_mesh",
  "param": { "shapeId": 1 }
}
```

Omit `shapeId` to clear all mesh data.

### 5.3 query_mesh_info

**Request**:
```json
{
  "module": "mesh",
  "action": "query_mesh_info",
  "param": {
    "entities": [
      {"shapeId": 1, "type": "MeshNode", "localId": 5},
      {"shapeId": 1, "type": "MeshElement", "localId": 3}
    ]
  }
}
```

**Response**:
```json
{
  "ok": true,
  "action": "query_mesh_info",
  "entities": [
    {
      "shapeId": 1, "type": "MeshNode", "localId": 5,
      "position": [1.2, 3.4, 0.0]
    },
    {
      "shapeId": 1, "type": "MeshElement", "localId": 3,
      "elementType": "Triangle",
      "nodeLocalIds": [5, 12, 8]
    }
  ]
}
```

---

## 6. Scene Integration

### 6.1 Existing Support (No Modifications Needed)

| Component | Mesh Support |
|-----------|-------------|
| `EntityType` | `MeshNode=10`, `MeshEdge=11`, `MeshElement=12` |
| `PickMask::maskForEntityType()` | MeshNode→Vertex, MeshEdge→Edge, MeshElement→Face |
| `PickId::encode()` | 64-bit encoding supports mesh EntityTypes |
| `SelectionState` | Generic EntityRef — works for mesh entities |
| `LabelManager` | Generic Label3D — works for mesh entities |
| `SceneNode` | Composition via IRenderComponent + IPickComponent |

### 6.2 MeshSceneBridge

```cpp
class MeshSceneBridge {
public:
    MeshSceneBridge(SceneGraph& scene, MeshStore& meshStore);
    ~MeshSceneBridge();

private:
    void onMeshAdded(uint32_t shapeId, const MeshEntry& entry);
    void onMeshRemoved(uint32_t shapeId);
    void onStoreCleared();

    SceneGraph& m_scene;
    MeshStore& m_meshStore;
    std::unordered_map<uint32_t, NodeId> m_meshToNode;
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};
```

**onMeshAdded**:
1. Build `RenderMeshData` via `MeshRenderBuilder::build(shapeId, entry)`.
2. Create SceneNode with `sourceType="mesh"`, `sourceId=shapeId`.
3. Attach `MeshRenderComponent` (implements `IRenderComponent`).
4. Register in `m_meshToNode` map.

**onMeshRemoved**: Remove SceneNode, erase from map.

### 6.3 MeshRenderBuilder

Converts `MeshEntry` → `RenderMeshData`:

**Triangle Ranges** (for OpaquePass):
- Each 2D element (Triangle/Quad) → one `DrawRange` in `triangleRanges`
  - `entityType = MeshElement`, `localId = element index + 1`
  - Quads split into 2 triangles (6 indices)
- Each 3D element → exterior faces as triangles
  - Tetra: 4 triangular faces, Hexa: 6 quad faces (12 triangles), etc.

**Line Ranges** (for WireframePass):
- Extract unique edges from all elements using sorted node-pair deduplication.
- Each unique edge → one `DrawRange` in `lineRanges`
  - `entityType = MeshEdge`, `localId` = auto-assigned sequential ID

**Point Ranges** (for vertex display):
- Each node → one `DrawRange` in `pointRanges`
  - `entityType = MeshNode`, `localId = node index + 1`

**PickId Encoding**:
- Every vertex gets a `PickIdEntry` with `PickId::encode(shapeId, entityType, localId)`.
- Triangle vertices → MeshElement pick, edge vertices → MeshEdge pick,
  point vertices → MeshNode pick.

---

## 7. Rendering

All existing render passes handle mesh data through the generic `RenderMeshData`
interface. **No render code modifications required.**

| Pass | Mesh Usage |
|------|-----------|
| **OpaquePass** | Draws `triangleRanges` — lit mesh surfaces |
| **WireframePass** | Draws `lineRanges` — mesh edge wireframe |
| **HighlightPass** | Overlays selected/hovered mesh entities with highlight color |
| **SelectionPass** | Off-screen FBO with `PickIdEntry` for GPU color picking |
| **LabelPass** | Renders `Label3D` entries from `LabelManager` for mesh labels |

### Mesh Color Scheme

- Mesh surfaces: distinct hue from source geometry (e.g., lighter tint or semi-transparent)
  to visually separate mesh from geometry.
- Mesh edges: uniform thin line color (e.g., dark gray).
- Hover/selection highlight: reuses existing `ColorMap` settings.

---

## 8. QML UI

### 8.1 EntityTypeSelector Refactor

Extract from `GeoQueryPage` into a reusable component. Parent pages configure visible
types via property:

```qml
EntityTypeSelector {
    availableTypes: [
        {key: "MeshNode",    label: qsTr("Node"),    mask: 1},
        {key: "MeshEdge",    label: qsTr("Edge"),    mask: 2},
        {key: "MeshElement", label: qsTr("Element"), mask: 8}
    ]
}
```

GeoQueryPage refactored to use the same component with geometry types.

### 8.2 MeshGeneratePage

Registered in RibbonConfig under "Mesh" tab → "Generate" group.

**Layout**:
```
┌─────────────────────────────────┐
│  Entity Picker                  │
│  [Face] [Solid]                 │  ← EntityTypeSelector (GeoFace, GeoSolid)
│  ● Pick mode active             │
│  Selected: 3 entities           │
│  [[1]F:2] [[1]F:5] [×]         │  ← EntityChip list
├─────────────────────────────────┤
│  Basic Parameters               │
│  Element Size:  [1.0        ]   │
│  Dimension:     [2D       ▼]    │
│  Element Type:  [Triangle ▼]    │
│  Algorithm:     [Delaunay ▼]    │
├─────────────────────────────────┤
│  ▶ Advanced Parameters          │  ← Expandable section
│  Min Size: [0.1]  Max: [10.0]   │
│  Order:    [1st ▼]              │
│  Optimize: [✓]                  │
├─────────────────────────────────┤
│  [Generate Mesh]   [Clear Mesh] │
└─────────────────────────────────┘
```

**Behavior**:
- Opens pick mode for GeoFace/GeoSolid.
- Entity chips show `[shapeId]Type:localId` format.
- "Generate Mesh" sends `generate_mesh` request.
- "Clear Mesh" sends `clear_mesh` request.
- Progress feedback via ActivityOverlay.

### 8.3 MeshQueryPage

Registered in RibbonConfig under "Mesh" tab → "Query" group.

**Layout**:
```
┌─────────────────────────────────┐
│  Entity Filter                  │
│  [Node] [Edge] [Element]        │  ← EntityTypeSelector (mesh types)
│  Auto-label: [✓]               │
│  ● Pick mode active             │
│  Selected: 2 entities           │
│  [[1]N:5] [[1]Tri:3] [×]       │  ← EntityChip list
│                                 │
│  Empty: Click to pick mesh      │
│  entities in the viewport       │
│  [Clear All]                    │
└─────────────────────────────────┘
```

**Behavior** (mirrors GeoQueryPage):
- Opens pick mode for MeshNode/MeshEdge/MeshElement.
- Auto-label toggle controls automatic label creation on selection.
- Hover highlights mesh entities under cursor.
- Click selects, showing label in viewport.
- Entity chips and labels use unified `[shapeId]Type:localId` format.
- "Clear All" removes all mesh selections and labels.

**Scene Commands** (same pattern as GeoQueryPage):
```javascript
sceneCommand("set_pick_mode", { pickMask: typeSelector.mask, enabled: true });
sceneCommand("set_labels_visible", { visible: true });
sceneCommand("set_auto_label", { enabled: true });
```

### 8.4 RibbonConfig Extension

Add to "Mesh" tab groups:
```javascript
{
    tab: 1,  // Mesh tab
    title: "Mesh",
    actions: [
        { key: "generateMesh", label: "Generate", icon: "mesh" },
        { key: "meshQuery",    label: "Query",    icon: "search" }
    ]
}
```

### 8.5 MainPages Registration

```javascript
componentMap: {
    "generateMesh": { path: "components/pages/MeshGeneratePage.qml" },
    "meshQuery":    { path: "components/pages/MeshQueryPage.qml" }
}
```

---

## 9. Label Format (Unified)

All EntityChip displays, viewport labels, and query results use the format:

```
[shapeId]TypeAbbrev:localId
```

### Geometry Labels

| EntityType | Abbreviation | Example |
|-----------|-------------|---------|
| GeoVertex | V | `[1]V:2` |
| GeoEdge | E | `[1]E:5` |
| GeoWire | W | `[1]W:1` |
| GeoFace | F | `[1]F:3` |
| GeoSolid | S | `[1]S:1` |

### Mesh Labels

| EntityType | Abbreviation | Example |
|-----------|-------------|---------|
| MeshNode | N | `[1]N:5` |
| MeshEdge | L | `[1]L:12` |
| MeshElement(Triangle) | Tri | `[1]Tri:3` |
| MeshElement(Quad) | Q | `[1]Q:7` |
| MeshElement(Tetra) | Tet | `[1]Tet:2` |
| MeshElement(Hexa) | Hex | `[1]Hex:1` |
| MeshElement(Prism) | Pri | `[1]Pri:4` |
| MeshElement(Pyramid) | Pyr | `[1]Pyr:1` |

For MeshElement labels, the type abbreviation is derived from the element's
`MeshElementType`, not the generic `MeshElement` entity type. The label generation
logic queries `MeshStore::find(shapeId)->elements[localId - 1].type` to resolve
the concrete element type for display. This lookup is O(1).

---

## 10. Edge Extraction Algorithm

Mesh edges are not stored persistently. `MeshRenderBuilder` extracts them on-the-fly
during `RenderMeshData` construction:

1. Define edge connectivity tables per element type:
   - Triangle: 3 edges `[(0,1),(1,2),(2,0)]`
   - Quad: 4 edges `[(0,1),(1,2),(2,3),(3,0)]`
   - Tetra: 6 edges `[(0,1),(0,2),(0,3),(1,2),(1,3),(2,3)]`
   - Hexa: 12 edges
   - Prism: 9 edges
   - Pyramid: 8 edges
2. For each element, extract edges as sorted node-pair keys.
3. Deduplicate using `std::unordered_set<uint64_t>` with
   `key = (min(a,b) << 32) | max(a,b)`.
4. Assign sequential `localId` to unique edges (1-based).
5. Generate `lineRanges` with `entityType = MeshEdge`.

---

## 11. Gmsh Session Management

Use RAII guard for gmsh lifecycle:

```cpp
class GmshSession {
public:
    GmshSession();   // gmsh::initialize(), suppress terminal, add model
    ~GmshSession();  // gmsh::finalize()
    // Non-copyable, non-movable
};
```

On Windows, wrap with `WindowsProcessPathGuard` to handle DLL search path issues
(same pattern as OGL reference).

---

## 12. Thread Safety

| Component | Protection |
|-----------|-----------|
| `MeshStore` | `std::shared_mutex` — readers share, writers exclusive |
| `MeshEntry` | Immutable after `setMesh()` — safe for concurrent reads |
| `MeshSceneBridge` | Runs on main thread (signal handler) |
| `MeshRenderBuilder` | Stateless — thread-safe by design |
| Gmsh session | Single-threaded per session (gmsh is not thread-safe) |

---

## 13. Testing Strategy

- **Unit tests** (`mesh_store_test.cpp`): MeshStore CRUD, lookup by localId, signal
  emission, edge extraction correctness.
- **Integration tests**: generate_mesh action with simple geometry (box → 6 face mesh),
  verify node/element counts, verify MeshSceneBridge creates SceneNodes.
- **Manual testing**: QML pages interaction, pick/hover/select/label flow.

---

## 14. Future Considerations (Out of Scope)

- Cross-shape meshing (meshing faces from different shapes together)
- Mesh import from external files (e.g., .msh, .vtk)
- Higher-order elements (quadratic)
- Mesh quality metrics and visualization
- Mesh smoothing/optimization actions
- Mesh export functionality
