# Second-Order Mesh Split Design

## Problem

The mesh split algorithm currently handles only first-order elements (Triangle, Quad).
Phase 3 added second-order mesh generation (Tri6, Quad8, Quad9), but the split guard
rejects any mesh containing these types. Users expect to split second-order meshes
and preserve their element order.

## Scope

- **In scope**: Tri6 and Quad8 edge-based and node-based splitting, mixed-order
  neighbor propagation
- **Out of scope**: Quad9 splitting (rejected with error), 3D element splitting

## Approach: Split-then-Upgrade

Reuse the existing linear split algorithm through three phases:

### Phase 1 — Pre-seed

Before any splitting, scan all elements. For each Tri6 or Quad8 element, register
its mid-edge nodes in the cache so `getOrCreateMidpoint()` returns the existing
node instead of creating a duplicate.

**Tri6 node layout** `[c0, c1, c2, m01, m12, m20]`:

| Corner pair | Mid-edge offset | Formula       |
|-------------|-----------------|---------------|
| (0, 1)      | 3               | cornerCount+0 |
| (1, 2)      | 4               | cornerCount+1 |
| (2, 0)      | 5               | cornerCount+2 |

**Quad8 node layout** `[c0, c1, c2, c3, m01, m12, m23, m30]`:

| Corner pair | Mid-edge offset | Formula       |
|-------------|-----------------|---------------|
| (0, 1)      | 4               | cornerCount+0 |
| (1, 2)      | 5               | cornerCount+1 |
| (2, 3)      | 6               | cornerCount+2 |
| (3, 0)      | 7               | cornerCount+3 |

Two caches are populated:

1. `edgeMidpointNodes[topology_edge_index]` — used by the existing split algorithm
2. `nodePairMidpoints[pack(nodeA, nodeB)]` — used by the upgrade step for
   arbitrary node pairs (including new sub-edges)

### Phase 2 — Linear Split

Route elements by `linearEquivalent()`:

- `Tri6` → `processTriangleEdges()` / `processTriangleNodes()`
- `Quad8` → `processQuadEdges()`

The existing functions access only corner nodes (`nodeLocalIds[0..cornerCount-1]`),
so they work unchanged for second-order elements. The pre-seeded cache ensures
split midpoints reuse existing mid-edge nodes.

### Phase 3 — Upgrade

After each `processXxxEdges` / `processXxxNodes` call on a second-order element,
post-process the replacement:

For each child element (Triangle or Quad):

1. For each edge `(ci, cj)` of the child:
   - Call `getOrCreateMidpointByNodes(ctx, ci, cj)`
   - This checks `nodePairMidpoints` first (returns existing mid-edge node if
     the edge existed in the original element), otherwise creates a new midpoint
2. Set `nodeLocalIds[cornerCount + k] = mid_edge_node` for the k-th edge
3. Change element type: `Triangle → Tri6`, `Quad → Quad8`

The `getOrCreateMidpointByNodes()` function uses a `getNodePosition()` helper
that resolves positions from both `entry.nodes` (original) and `result.newNodes`
(newly created, e.g., centroids).

## Mixed-Order Meshes

When a second-order element is split, its neighbor may be first-order or
second-order:

- **First-order neighbor** (Triangle/Quad): `processNeighborCut()` produces
  first-order children (unchanged behavior)
- **Second-order neighbor** (Tri6/Quad8): `processNeighborCut()` produces
  first-order children, then upgrades them

The pre-seeded cache ensures that all elements sharing an edge agree on the
midpoint node, regardless of their order.

## Guard Changes (split\_mesh\_action.cpp)

```
Current:  reject all non-Triangle/Quad
Proposed: reject Quad9, Tetra, Hexa, Prism, Pyramid
          allow  Triangle, Quad, Tri6, Quad8
```

## Helper Function Changes

### `findOppositeNode()`

Use `cornerCount()` instead of `nodeCount()` to iterate only corner nodes.
Prevents accidentally returning a mid-edge node for Tri6 elements.

### `createCentroid()`

Use `cornerCount()` instead of `nodeCount()`. For straight-edge elements the
result is identical, but the intent is clearer — the centroid of a triangle is
the average of its three corners, regardless of mid-edge nodes.

### New helper: `isSecondOrder(MeshElementType)`

Returns `true` for Tri6, Quad8, Quad9. Added to `mesh_element_type.hpp`.

## New SplitContext Members

```cpp
/// Node-pair midpoint cache (key = packed pair of 1-based localIds).
std::unordered_map<uint64_t, uint32_t> nodePairMidpoints;

/// Original node count (entry.nodes.size() at start of compute).
uint32_t originalNodeCount{};
```

## New Private Methods

| Method | Purpose |
|--------|---------|
| `packNodePair(a, b)` | Pack two 1-based node IDs into a single `uint64_t` key |
| `getNodePosition(ctx, localId, x, y, z)` | Resolve position from entry.nodes or result.newNodes |
| `getOrCreateMidpointByNodes(ctx, a, b)` | Find/create midpoint for any node pair |
| `seedMidEdgeNodes(ctx)` | Pre-seed caches with existing mid-edge nodes |
| `upgradeReplacementToSecondOrder(ctx, rep, sourceType)` | Upgrade children to Tri6/Quad8 |
| `makeTri6(c0..c2, m01, m12, m20)` | Create Tri6 MeshElement |
| `makeQuad8(c0..c3, m01..m30)` | Create Quad8 MeshElement |

## Worked Example: Tri6 1-Edge Split

Original Tri6: `[c0, c1, c2, m01, m12, m20]`, split edge `(c0, c1)`.

```
       c2                    c2
      / \                   /|\
   m20   m12     →       m20 | m12
    /     \               /  |  \
  c0--m01--c1           c0--m01--c1
```

**Pre-seed**: `edgeMidpointNodes[edge(c0,c1)] = m01`

**Linear split**: `getOrCreateMidpoint(edge(c0,c1))` → returns `m01` (cached).
Two Triangle children: `(c0, m01, c2)` and `(m01, c1, c2)`.

**Upgrade child 1** `(c0, m01, c2)` → Tri6:

| Edge       | Lookup                            | Result              |
|------------|-----------------------------------|---------------------|
| (c0, m01)  | `getOrCreateMidpointByNodes`      | NEW node at avg(c0, m01) |
| (m01, c2)  | `getOrCreateMidpointByNodes`      | NEW node at avg(m01, c2) |
| (c2, c0)   | `nodePairMidpoints[pack(c2,c0)]`  | m20 (pre-seeded)    |

**Upgrade child 2** `(m01, c1, c2)` → Tri6:

| Edge       | Lookup                            | Result              |
|------------|-----------------------------------|---------------------|
| (m01, c1)  | `getOrCreateMidpointByNodes`      | NEW node at avg(m01, c1) |
| (c1, c2)   | `nodePairMidpoints[pack(c1,c2)]`  | m12 (pre-seeded)    |
| (c2, m01)  | `nodePairMidpoints[pack(c2,m01)]` | reuse from child 1  |

**Result**: 2 Tri6 children, 3 new nodes (sub-midpoints), 0 duplicate nodes.

## Test Cases

| # | Input    | Edges | Mode             | Expected Output       |
|---|----------|-------|------------------|-----------------------|
| 1 | Tri6     | 1     | Auto             | 2 Tri6                |
| 2 | Tri6     | 2     | Auto             | 3 Tri6                |
| 3 | Tri6     | 3     | TriaFour         | 4 Tri6                |
| 4 | Tri6     | 3     | QuadThree        | 3 Quad8               |
| 5 | Tri6     | nodes | TriaThree        | 3 Tri6                |
| 6 | Quad8    | 1     | Auto             | 1 Tri6 + 1 Quad8      |
| 7 | Quad8    | 2 opp | Auto             | 2 Quad8               |
| 8 | Quad8    | 2 adj | Auto             | 2 Tri6 + 1 Quad8      |
| 9 | Quad8    | 4     | Auto             | 4 Quad8               |
| 10| Tri6+Tri | shared| Auto             | Tri6 children + Tri children |
| 11| Quad9    | any   | —                | Error: unsupported     |
| 12| Tri6 neighbor | 1  | Auto            | Neighbor also upgraded |
