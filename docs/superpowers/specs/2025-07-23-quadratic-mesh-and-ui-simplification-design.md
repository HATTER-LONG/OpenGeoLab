# Quadratic (Second-Order) 2D Mesh Support & Generate Mesh UI Simplification

## Problem

The Generate Mesh feature currently only supports first-order elements (Tri3, Quad4). gmsh already supports higher-order mesh generation (`Mesh.ElementOrder` is set in code), but the downstream data structures, type mapping, and rendering pipeline discard second-order elements.

Additionally, the MeshGeneratePage QML is cluttered with an unnecessary Advanced fold containing min/max size and mesh order inputs. The UI should be simplified to surface common options directly.

## Scope

- **In scope**: Second-order 2D elements (Tri6, Quad8, Quad9), UI simplification, percentage-based sizing
- **Out of scope**: Second-order 3D elements (Tet10, Hex20, Hex27), curved-edge rendering, mesh split for second-order elements

## Approach

Straight-edge rendering: second-order elements render using corner nodes only (same visual as first-order), mid-edge nodes displayed as points. This minimizes rendering changes while preserving the full second-order node data.

---

## 1. Backend — Data Layer Changes

### 1.1 MeshElementType Enum

Add three new variants to `mesh_element_type.hpp`:

```cpp
enum class MeshElementType : uint8_t {
    Triangle = 0, ///< 3-node triangle (2D)
    Quad     = 1, ///< 4-node quadrilateral (2D)
    Tetra    = 2, ///< 4-node tetrahedron (3D)
    Hexa     = 3, ///< 8-node hexahedron (3D)
    Prism    = 4, ///< 6-node prism/wedge (3D)
    Pyramid  = 5, ///< 5-node pyramid (3D)
    Tri6     = 6, ///< 6-node quadratic triangle (2D)
    Quad8    = 7, ///< 8-node serendipity quadrilateral (2D)
    Quad9    = 8, ///< 9-node quadratic quadrilateral (2D)
};
```

### 1.2 K_MAX_ELEMENT_NODES

Increase from 8 to 9 (Quad9 has 9 nodes):

```cpp
inline constexpr uint8_t K_MAX_ELEMENT_NODES = 9;
```

### 1.3 nodeCount()

Add cases:
- `Tri6` → 6
- `Quad8` → 8
- `Quad9` → 9

### 1.4 elementTypePrefix()

- `Tri6` → `"Tri6"`
- `Quad8` → `"Q8"`
- `Quad9` → `"Q9"`

### 1.5 elementDimension()

- `Tri6`, `Quad8`, `Quad9` → 2

### 1.6 cornerCount() (new helper)

Returns the number of corner (vertex) nodes for an element type, used by the rendering pipeline to determine which nodes form the straight-edge skeleton:

```cpp
[[nodiscard]] constexpr uint8_t cornerCount(MeshElementType type) noexcept;
```

- Triangle, Tri6 → 3
- Quad, Quad8, Quad9 → 4
- Tetra → 4, Hexa → 8, Prism → 6, Pyramid → 5

### 1.7 linearEquivalent() (new helper)

Maps a second-order type to its first-order counterpart for rendering dispatch:

```cpp
[[nodiscard]] constexpr MeshElementType linearEquivalent(MeshElementType type) noexcept;
```

- Tri6 → Triangle
- Quad8, Quad9 → Quad
- All others → identity

---

## 2. Backend — gmsh Type Mapping

Update `mapGmshElementType()` in `generate_mesh_action.cpp`:

| gmsh type code | Element | Mapping |
|---|---|---|
| 2 | 3-node triangle | Triangle |
| 3 | 4-node quad | Quad |
| 9 | 6-node triangle | Tri6 |
| 10 | 9-node quad | Quad9 |
| 16 | 8-node quad (serendipity) | Quad8 |

Existing 3D mappings (4, 5, 6, 7) remain unchanged.

---

## 3. Backend — Rendering Pipeline

### 3.1 mesh_render_builder.cpp

`appendElementTriangles()` — add cases for Tri6, Quad8, Quad9 that reuse the corner-only triangulation:

- Tri6 → same as Triangle: `{0, 1, 2}`
- Quad8/Quad9 → same as Quad: `{0, 1, 2}`, `{0, 2, 3}`

`appendElementEdges()` — add cases for Tri6, Quad8, Quad9 that only connect corner nodes:

- Tri6 → same as Triangle: `{0,1}, {1,2}, {2,0}`
- Quad8/Quad9 → same as Quad: `{0,1}, {1,2}, {2,3}, {3,0}`

Mid-edge nodes (indices 3-5 for Tri6, 4-7 for Quad8, 4-8 for Quad9) are automatically rendered as points by the existing node rendering pass — no extra work needed.

### 3.2 mesh_topology.cpp

Same changes as render builder: `appendElementEdges()` for the three new types uses corner-only edges.

---

## 4. Backend — Size Mode (Percentage)

Add support for percentage-based element sizing in `generate_mesh_action.cpp`:

- New optional param `sizeMode`: `"absolute"` (default) or `"percentage"`
- When `sizeMode == "percentage"`, compute the bounding box diagonal of the compound shape, then `elementSize = diagonal * (elementSize / 100.0)`
- The `minSize` and `maxSize` in advanced params are also scaled by the diagonal when in percentage mode

The bounding box computation uses `Bnd_Box` + `BRepBndLib::Add()` from OCC, which is already available as a dependency.

---

## 5. QML — MeshGeneratePage Simplification

### Current Layout (to be replaced)

```
Target Geometry (Face/Solid selector)
Selection chips
Size (DimensionInput)
Dimension (2D/3D toggle)
Element (Tri/Quad toggle)
Algorithm (chip selector)
▸ Advanced
    Min size
    Max size
    P (order)
    Optimize toggle
Helper text
Clear All Mesh button
```

### New Layout

```
Target Geometry (Face/Solid selector)
Selection chips

Size  [absolute ▾]  [input field]    ← dropdown for abs/percentage
Element    [Tri] [Quad]
Order      [Linear] [Quadratic]
Algorithm  (chip flow — same as current)
Dimension  [2D] [3D]
Optimize   [switch]

Helper text
Clear All Mesh button
```

Key changes:
1. **Remove Advanced fold** — all options flat
2. **Remove minSize/maxSize** — single Size input only
3. **Add size mode** — dropdown/toggle for absolute vs percentage
4. **Add Order** — `[Linear] [Quadratic]` button group (maps to order=1/2)
5. **Keep Algorithm, Dimension, Element, Optimize** — promoted to main panel

### getParameters() Change

```javascript
param: {
    entities: entities,
    elementSize: root.elementSize,
    sizeMode: root.sizeMode,      // "absolute" or "percentage"
    dimension: root.meshDimension,
    elementType: root.elementType,
    algorithm: root.algorithm,
    advanced: {
        order: root.meshOrder,     // 1 or 2
        optimize: root.optimizeMesh
    }
}
```

Note: `minSize`/`maxSize` removed from frontend but still supported in backend `advanced` params for API/script users.

---

## 6. Mesh Split Interaction

Second-order elements are **not supported** by the mesh split algorithm. When the user attempts to split a mesh containing second-order elements:

- The split action should return an error: `"Mesh split is not supported for second-order elements. Convert to linear elements first."`
- Future work: add a `convert_to_linear` action that strips mid-edge nodes

---

## 7. Testing

### Backend Tests

- New test cases in `mesh_element_type` tests:
  - `nodeCount()` for Tri6/Quad8/Quad9
  - `cornerCount()` for all types
  - `linearEquivalent()` for all types
  - `elementDimension()` for new types

- New test cases in `generate_mesh_action` tests:
  - Generate mesh with `order: 2`, verify Tri6/Quad8/Quad9 elements created
  - Percentage size mode: verify computed size matches expected diagonal fraction
  - Edge case: percentage mode with empty geometry

### Manual Testing

- Visual verification:
  - Generate second-order tri mesh → verify triangles render with mid-edge nodes visible as points
  - Generate second-order quad mesh → verify quads render with mid-edge nodes
  - Verify edge picking works on second-order elements
  - Verify node picking selects mid-edge nodes

---

## 8. File Change Summary

| File | Change |
|---|---|
| `mesh_element_type.hpp` | Add Tri6, Quad8, Quad9 enums; bump K_MAX_ELEMENT_NODES to 9; add cornerCount(), linearEquivalent() |
| `generate_mesh_action.cpp` | Add gmsh type mappings; add sizeMode/percentage support |
| `generate_mesh_action.hpp` | (no change expected) |
| `mesh_render_builder.cpp` | Add triangulation/edge cases for Tri6, Quad8, Quad9 |
| `mesh_topology.cpp` | Add edge cases for Tri6, Quad8, Quad9 |
| `split_mesh_action.cpp` | Add guard to reject second-order elements |
| `MeshGeneratePage.qml` | Simplify UI, add Order toggle, add size mode |
| `opengeolab_zh_CN.ts` | Update translations for new UI strings |
| `mesh_element_type_test.cpp` (or equivalent) | New test cases |
