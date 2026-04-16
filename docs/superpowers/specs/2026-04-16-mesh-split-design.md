# Mesh Split Design Spec

## Overview

Implement a mesh split feature that allows users to subdivide mesh elements (triangles and quads) by selecting edges or nodes. The algorithm produces new sub-elements according to 6 predefined split patterns, with automatic neighbor propagation for adjacent elements sharing selected edges.

### Reference

- Algorithm reference: `imMeshSpliter` class from external codebase (images in `docs/task/`)
- Topology diagrams: `C:\Users\layton\Desktop\tmp\mesh_split.html`
- UI reference: `C:\Users\layton\Desktop\tmp\ui.png`, `ui2.png`

---

## Split Modes

| Mode | Enum Value | Source | Result | Condition |
|------|-----------|--------|--------|-----------|
| TriaOneQuadThree | 1 | EdgeType | 3□+1△ | Quad with 3 selected edges (center P variant) |
| TriaOneQuadTwo | 2 | EdgeType | 2□+1△ | Quad with 3 selected edges (Mr connects) |
| TriaThreeQuadTwo | 4 | EdgeType | 2□+3△ | Quad with 3 selected edges (center P variant 2) |
| TriaFour | 8 | EdgeType | 4△ | Triangle with 3 selected edges (uniform) |
| QuadThree | 16 | EdgeType | 3□ | Triangle with 3 selected edges (centroid) |
| TriaThree | 32 | NodeType | 3△ | Triangle with 3 selected nodes (centroid) |

### Auto-Selection Rules

- Quad with 1 selected edge → auto: midpoint split along that edge (1□+1△)
- Quad with 2 adjacent selected edges → auto: midpoint splits (specific topology)
- Quad with 2 opposite selected edges → auto: midpoint splits (specific topology)
- Quad with 3 selected edges → user must choose: TriaOneQuadThree / TriaOneQuadTwo / TriaThreeQuadTwo
- Quad with 4 selected edges → auto: 4□ (midpoint quad subdivision)
- Triangle with 1-2 selected edges → auto: midpoint split(s) (specific topology)
- Triangle with 3 selected edges → user must choose: TriaFour / QuadThree
- Triangle with 3 selected nodes → only TriaThree
- Neighbor propagation: midpoint → opposite vertex → 1△+1□ (automatic, no cascading)
- When the user-selected mode is not applicable to a specific element's edge count, the algorithm falls back to the auto-determined pattern for that element

---

## Architecture

### Approach

Independent algorithm class + thin Action layer (Approach A). Algorithm code maintains name correspondence with reference `imMeshSpliter` for cross-referencing, but uses clean C++20 structure.

### Key Decisions

1. **Edge data**: On-demand `MeshTopology` structure (not stored in MeshEntry)
2. **Modification strategy**: In-place MeshEntry modification + new `meshModified` signal
3. **Neighbor propagation**: Automatic, no user confirmation
4. **Undo**: Not implemented in this iteration
5. **Scope**: Backend algorithm + Action + QML frontend, all together

---

## Data Structures

### SplitMode

```cpp
enum class SplitMode : uint8_t {
    TriaOneQuadThree = 1,   // 3□+1△
    TriaOneQuadTwo   = 2,   // 2□+1△
    TriaThreeQuadTwo = 4,   // 2□+3△
    TriaFour         = 8,   // 4△
    QuadThree        = 16,  // 3□
    TriaThree        = 32,  // 3△
};
```

Values match reference `imMeshSpliter::EdgeType` / `NodeType` for cross-referencing.

### MeshTopology

On-demand topology structure built from MeshEntry. Uses flat vectors indexed by localId for O(1) lookup. Discarded after use.

```cpp
struct MeshTopology {
    // edges[i] = sorted (node1, node2), edge localId = i + 1
    std::vector<std::pair<uint32_t, uint32_t>> edges;

    // edgeToElements[i] = element indices sharing edges[i]
    std::vector<std::vector<uint32_t>> edgeToElements;

    // nodeToElements[localId] = element indices containing this node
    // Index 0 unused (localId is 1-based)
    std::vector<std::vector<uint32_t>> nodeToElements;

    // nodeToEdges[localId] = edge indices containing this node
    std::vector<std::vector<uint32_t>> nodeToEdges;

    // O(log E) - find edge index by node pair (binary search in sorted edges)
    std::optional<uint32_t> findEdgeIndex(uint32_t node1, uint32_t node2) const;

    // O(1) - resolve selection localId to node pair
    std::pair<uint32_t, uint32_t> resolveEdge(uint32_t localId) const;

    static MeshTopology build(const MeshEntry& entry);
};
```

Performance rationale: `std::map` has poor cache locality for large meshes. Flat vectors indexed by localId give O(1) access. Edge-by-node-pair lookup uses binary search (O(log E)) on the already-sorted edges vector.

The edge derivation logic must produce the same ordering as `MeshRenderBuilder` to ensure edge localId consistency between rendering and algorithm. The existing edge derivation code in `mesh_render_builder.cpp` (lines 244-259) will be extracted into `MeshTopology::build()` as a shared implementation.

### SplitResult

```cpp
struct SplitResult {
    struct NewNode { double x, y, z; };
    struct ElementReplacement {
        uint32_t originalIndex;
        std::vector<MeshElement> newElems;
    };
    std::vector<NewNode> newNodes;
    std::vector<ElementReplacement> replacements;
};
```

---

## Algorithm: MeshSplitAlgorithm

### Class Structure

```cpp
class MeshSplitAlgorithm {
public:
    SplitResult compute(
        const MeshEntry& entry,
        const MeshTopology& topology,
        const std::vector<uint32_t>& selectedEdgeLocalIds,
        const std::vector<uint32_t>& selectedNodeLocalIds,
        SplitMode mode) const;

private:
    // Names correspond to imMeshSpliter functions for cross-referencing
    void processQuadEdges(...);      // processQuadNodesEdge, processQuadN2, etc.
    void processTriaEdges(...);      // processTriaEdges, processTriaAllEdges
    void processNodeSplit(...);      // processNodeSolve
    void processNeighborCut(...);    // neighbor propagation logic

    uint32_t findOppositeNode(...);  // findOppositeNodeIdOfEdgeTriQuad
    SplitResult::NewNode computeMidpoint(const MeshNode& a, const MeshNode& b) const;
    SplitResult::NewNode computeCentroid(const MeshEntry& entry,
                                         const MeshElement& elem) const;
};
```

### Processing Flow

```
compute():
  1. Group selected edges/nodes by element
     → elementSelections: map<elemIdx, {edgeIndices[], nodeLocalIds[]}>

  2. For each affected element:
     a. Determine element type (Tri/Quad) + count of selected edges/nodes
     b. Validate or auto-select SplitMode
     c. Call processQuadEdges / processTriaEdges / processNodeSplit
        → produces NewNodes + ElementReplacement
     d. For each selected edge: find neighbor element via edgeToElements
        → call processNeighborCut if neighbor not already processed
        → produces additional NewNodes + neighbor ElementReplacement

  3. Collect all NewNodes + ElementReplacements → return SplitResult
```

---

## Action API

### SplitMeshAction

Inherits `IAction`, registered in MeshModule.

#### Request

```json
{
    "module": "mesh",
    "action": "split_mesh",
    "param": {
        "shapeId": 1,
        "selections": [
            {"type": "edge", "localId": 5},
            {"type": "edge", "localId": 8}
        ],
        "mode": "auto"
    }
}
```

| Param | Type | Required | Description |
|-------|------|----------|-------------|
| shapeId | int | yes | Target mesh shape ID |
| selections | array | yes | Selected edges/nodes with type and localId |
| mode | string | no | Split mode name or "auto" (default: "auto") |

Mode values: `tria_one_quad_three`, `tria_one_quad_two`, `tria_three_quad_two`, `tria_four`, `quad_three`, `tria_three`, `auto`.

#### Response

```json
{
    "ok": true,
    "action": "split_mesh",
    "shapeId": 1,
    "summary": "Split completed: 3 elements replaced, 4 new nodes, 8 new elements"
}
```

#### execute() Flow

```
1. Validate params (shapeId, selections, mode)
2. Get MeshEntry from MeshStore
3. Build MeshTopology from MeshEntry
4. Call MeshSplitAlgorithm::compute()
5. Apply SplitResult to MeshEntry (append nodes, replace elements)
6. Bump MeshEntry version
7. Emit meshModified signal
8. Return success response
```

---

## MeshStore Changes

Add `meshModified` signal and `modifyMesh()` method:

```cpp
// mesh_store.hpp
Kangaroo::Util::Signal<int32_t> meshModified; // emits shapeId

void modifyMesh(int32_t shapeId, std::function<void(MeshEntry&)> modifier);
```

`modifyMesh()` locks the store, applies the modifier function to the entry, bumps version, and emits `meshModified`.

The `MeshSceneBridge` will connect to `meshModified` to trigger re-rendering of the affected mesh.

---

## QML Frontend: MeshSplitPage

### Layout (Reference Style)

```
┌─────────────────────────────┐
│ ✕  Split Mesh               │
├─────────────────────────────┤
│ ☑ Edges   [◧][⬒][◫][△][□]  │  ← checkbox + 5 icon radio buttons
│ ☐ Nodes   [▽]               │  ← checkbox + 1 icon radio button
│─────────────────────────────│
│ Selected Entities            │
│ ┌─────────────────────────┐ │
│ │ Edge 5  (Node 3 – 7)  ✕│ │  ← selection list
│ │ Edge 8  (Node 1 – 4)  ✕│ │
│ │ Edge 12 (Node 4 – 9)  ✕│ │
│ └─────────────────────────┘ │
│         [Execute] [Cancel]   │
└─────────────────────────────┘
```

### Behavior

1. **Open**: Set pick mask to MeshEdge (Edges checked by default)
2. **Checkbox toggle**: Switch between Edge/Node pick mode, clear selections
3. **Icon buttons**: Radio buttons for split pattern (within each mode group)
4. **Selection list**: Shows picked entities, click ✕ to deselect
5. **Execute**: Build JSON request with selections + mode, submit via RequestService
6. **Close**: Disable pick mode, clear selections

### Registration

- `MainPages.qml`: Add `"splitMesh": { path: "components/pages/MeshSplitPage.qml" }`
- `RibbonConfig.qml`: Add Split button in Mesh tab's "Mesh" group

---

## File Organization

### New Files

| File | Purpose |
|------|---------|
| `mesh/include/.../mesh_topology.hpp` | MeshTopology struct |
| `mesh/include/.../split_mode.hpp` | SplitMode enum |
| `mesh/include/.../split_result.hpp` | SplitResult struct |
| `mesh/include/.../mesh_split_algorithm.hpp` | Algorithm class |
| `mesh/src/mesh_topology.cpp` | MeshTopology::build() |
| `mesh/src/mesh_split_algorithm.cpp` | Algorithm implementation |
| `mesh/src/action/split_mesh_action.hpp` | SplitMeshAction header |
| `mesh/src/action/split_mesh_action.cpp` | SplitMeshAction impl |
| `app/.../pages/MeshSplitPage.qml` | QML panel |

### Modified Files

| File | Change |
|------|--------|
| `mesh/src/mesh_module.cpp` | Register SplitMeshAction |
| `mesh/include/.../mesh_store.hpp` | Add meshModified signal, modifyMesh() |
| `mesh/src/mesh_store.cpp` | Implement modifyMesh() |
| `mesh/src/mesh_render_builder.cpp` | Extract edge derivation to use MeshTopology |
| `mesh/CMakeLists.txt` | Add new source files |
| `MainPages.qml` | Register splitMesh page |
| `RibbonConfig.qml` | Add Split ribbon button |

---

## Testing

### Unit Tests

| Test File | Coverage |
|-----------|----------|
| `mesh_topology_test.cpp` | build() correctness: single tri, shared-edge tris, quad, localId consistency |
| `mesh_split_algorithm_test.cpp` | All 6 split modes, neighbor propagation, edge cases (invalid selections) |
| `split_mesh_action_test.cpp` | describe() structure, valid/invalid requests, integration with MeshStore |

### Manual Testing

- QML interaction via application launch
- HTTP API testing via `--start-http-server` + action-remote-call
