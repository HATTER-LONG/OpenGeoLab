# Second-Order Mesh Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the mesh split algorithm to support Tri6 and Quad8 elements, producing second-order output that matches the input element order.

**Architecture:** Split-then-Upgrade in 3 phases: (1) Pre-seed caches with existing mid-edge nodes from second-order elements to prevent duplicate nodes. (2) Route via `linearEquivalent()` so existing `processTriangleEdges`/`processQuadEdges`/`processNeighborCut` produce Triangle/Quad children unchanged. (3) Post-split upgrade converts children to Tri6/Quad8 by computing sub-edge midpoints via a node-pair cache that ensures shared mid-edge nodes between adjacent children are created only once.

**Tech Stack:** C++20, doctest, OpenGeoLab Mesh library

**Design spec:** `docs/superpowers/specs/2025-07-24-second-order-mesh-split-design.md`

---

## Build & Test Commands

```bash
# MSVC environment + build mesh targets
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh opengeolab_mesh_test --parallel 4"

# Run mesh tests
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

---

## File Structure

| File | Action | Purpose |
|------|--------|---------|
| `src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp` | Modify (line 130) | Add `isSecondOrder()` constexpr helper |
| `src/libs/mesh/include/opengeolab/mesh/mesh_split_algorithm.hpp` | Modify (lines 50-98) | Add SplitContext members + 5 method declarations |
| `src/libs/mesh/src/mesh_split_algorithm.cpp` | Modify | Add 5 implementations, fix `createCentroid`/`findOppositeNode`, modify `compute()`/`processNeighborCut()` routing |
| `src/libs/mesh/src/action/split_mesh_action.cpp` | Modify (lines 216-221) | Update guard to allow Tri6/Quad8, reject Quad9 |
| `src/libs/mesh/test/mesh_split_algorithm_test.cpp` | Modify | Add 3 fixtures + 7 test cases |
| `src/libs/mesh/test/split_mesh_action_test.cpp` | Modify (lines 145-164) | Update rejection test, add Quad9 test |

---

## Element Node Layouts (reference)

| Type | Nodes | Layout | Mid-edge at indices |
|------|-------|--------|-------------------|
| Tri6 | 6 | `[c0, c1, c2, m01, m12, m20]` | 3, 4, 5 |
| Quad8 | 8 | `[c0, c1, c2, c3, m01, m12, m23, m30]` | 4, 5, 6, 7 |

---

### Task 1: Foundation — `isSecondOrder()` + cornerCount Fixes

**Files:**
- Modify: `src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp:130`
- Modify: `src/libs/mesh/src/mesh_split_algorithm.cpp:54,77`

- [ ] **Step 1: Add `isSecondOrder()` to mesh_element_type.hpp**

Open `src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp`. After `linearEquivalent()` (line 130), add:

```cpp
/// True for second-order element types (Tri6, Quad8, Quad9).
[[nodiscard]] constexpr bool isSecondOrder(MeshElementType type) noexcept {
    return type == MeshElementType::Tri6 || type == MeshElementType::Quad8 ||
           type == MeshElementType::Quad9;
}
```

- [ ] **Step 2: Fix `createCentroid` to use `cornerCount`**

In `src/libs/mesh/src/mesh_split_algorithm.cpp`, in the `createCentroid` function (line 54), change `nodeCount` to `cornerCount`:

Before (line 54):
```cpp
    const auto count = nodeCount(element.type);
```

After:
```cpp
    const auto count = cornerCount(element.type);
```

This ensures Tri6 centroids average only 3 corner positions (not all 6 nodes).

- [ ] **Step 3: Fix `findOppositeNode` to use `cornerCount`**

In the same file, in `findOppositeNode` (line 77), change `nodeCount` to `cornerCount`:

Before (line 77):
```cpp
    for(uint8_t i = 0; i < nodeCount(element.type); ++i) {
```

After:
```cpp
    for(uint8_t i = 0; i < cornerCount(element.type); ++i) {
```

This ensures Tri6 only searches corners 0-2 for the opposite node, not mid-edge nodes.

- [ ] **Step 4: Build and verify**

Run:
```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh opengeolab_mesh_test --parallel 4"
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

Expected: Build succeeds, all existing tests pass (no behavioral change for linear elements).

- [ ] **Step 5: Commit**

```bash
git add src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp src/libs/mesh/src/mesh_split_algorithm.cpp
git commit -m "feat(mesh): add isSecondOrder() and fix cornerCount in split helpers

createCentroid and findOppositeNode now use cornerCount instead of
nodeCount so they work correctly with second-order elements (Tri6,
Quad8) that have extra mid-edge node slots."
```

---

### Task 2: Core Implementation — SplitContext, Helpers, Routing, Upgrade

**Files:**
- Modify: `src/libs/mesh/include/opengeolab/mesh/mesh_split_algorithm.hpp:50-98`
- Modify: `src/libs/mesh/src/mesh_split_algorithm.cpp`

- [ ] **Step 1: Add SplitContext members to header**

In `src/libs/mesh/include/opengeolab/mesh/mesh_split_algorithm.hpp`, add two members to `SplitContext` after `nextNodeLocalId` (after line 62):

```cpp
        /// Node-pair midpoint cache — key is packed pair of 1-based localIds.
        /// Used by upgrade step to find/create midpoints for arbitrary node pairs.
        std::unordered_map<uint64_t, uint32_t> nodePairMidpoints;

        /// Original node count (entry.nodes.size() at start of compute).
        uint32_t originalNodeCount{};
```

- [ ] **Step 2: Add method declarations to header**

In the same file, add 5 method declarations after `makeQuad` (after line 98, before the closing `};`):

```cpp
    /// Pack two 1-based node localIds into a single uint64 key (order-independent).
    [[nodiscard]] static uint64_t packNodePair(uint32_t node_a, uint32_t node_b);

    /// Resolve node position (works for both original and newly-created nodes).
    void getNodePosition(const SplitContext& ctx,
                         uint32_t local_id,
                         double& out_x,
                         double& out_y,
                         double& out_z) const;

    /// Find or create midpoint between two nodes by 1-based localIds.
    uint32_t getOrCreateMidpointByNodes(SplitContext& ctx,
                                        uint32_t node_a,
                                        uint32_t node_b) const;

    /// Pre-seed caches with existing mid-edge nodes from second-order elements.
    void seedMidEdgeNodes(SplitContext& ctx) const;

    /// Upgrade all children in a replacement from linear to second-order types.
    void upgradeReplacementToSecondOrder(SplitContext& ctx,
                                         SplitResult::ElementReplacement& rep) const;
```

- [ ] **Step 3: Implement `packNodePair` and `getNodePosition`**

In `src/libs/mesh/src/mesh_split_algorithm.cpp`, add after `makeQuad` (after line 29):

```cpp
uint64_t MeshSplitAlgorithm::packNodePair(uint32_t node_a, uint32_t node_b) {
    const auto lo = std::min(node_a, node_b);
    const auto hi = std::max(node_a, node_b);
    return (static_cast<uint64_t>(hi) << 32U) | static_cast<uint64_t>(lo);
}

void MeshSplitAlgorithm::getNodePosition(const SplitContext& ctx,
                                          uint32_t local_id,
                                          double& out_x,
                                          double& out_y,
                                          double& out_z) const {
    if(local_id <= ctx.originalNodeCount) {
        const auto& pos = ctx.entry.nodes[local_id - 1U].position;
        out_x = static_cast<double>(pos[0]);
        out_y = static_cast<double>(pos[1]);
        out_z = static_cast<double>(pos[2]);
    } else {
        const auto& node = ctx.result.newNodes[local_id - ctx.originalNodeCount - 1U];
        out_x = node.x;
        out_y = node.y;
        out_z = node.z;
    }
}
```

- [ ] **Step 4: Implement `getOrCreateMidpointByNodes`**

Add after the functions from Step 3:

```cpp
uint32_t MeshSplitAlgorithm::getOrCreateMidpointByNodes(SplitContext& ctx,
                                                         uint32_t node_a,
                                                         uint32_t node_b) const {
    const uint64_t key = packNodePair(node_a, node_b);
    const auto existing = ctx.nodePairMidpoints.find(key);
    if(existing != ctx.nodePairMidpoints.end()) {
        return existing->second;
    }

    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;
    double bx = 0.0;
    double by = 0.0;
    double bz = 0.0;
    getNodePosition(ctx, node_a, ax, ay, az);
    getNodePosition(ctx, node_b, bx, by, bz);

    SplitResult::NewNode midpoint{};
    midpoint.x = (ax + bx) / 2.0;
    midpoint.y = (ay + by) / 2.0;
    midpoint.z = (az + bz) / 2.0;

    ctx.result.newNodes.push_back(midpoint);
    const uint32_t new_local_id = ctx.nextNodeLocalId++;
    ctx.nodePairMidpoints[key] = new_local_id;
    return new_local_id;
}
```

- [ ] **Step 5: Implement `seedMidEdgeNodes`**

Add after `getOrCreateMidpointByNodes`:

```cpp
void MeshSplitAlgorithm::seedMidEdgeNodes(SplitContext& ctx) const {
    for(uint32_t i = 0; i < ctx.entry.elements.size(); ++i) {
        const auto& elem = ctx.entry.elements[i];

        if(elem.type == MeshElementType::Tri6) {
            const uint32_t c0 = elem.nodeLocalIds[0];
            const uint32_t c1 = elem.nodeLocalIds[1];
            const uint32_t c2 = elem.nodeLocalIds[2];
            const uint32_t m01 = elem.nodeLocalIds[3];
            const uint32_t m12 = elem.nodeLocalIds[4];
            const uint32_t m20 = elem.nodeLocalIds[5];

            if(const auto e = ctx.topology.findEdgeIndex(c0, c1)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m01);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c1, c2)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m12);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c2, c0)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m20);
            }

            ctx.nodePairMidpoints.emplace(packNodePair(c0, c1), m01);
            ctx.nodePairMidpoints.emplace(packNodePair(c1, c2), m12);
            ctx.nodePairMidpoints.emplace(packNodePair(c2, c0), m20);
        } else if(elem.type == MeshElementType::Quad8) {
            const uint32_t c0 = elem.nodeLocalIds[0];
            const uint32_t c1 = elem.nodeLocalIds[1];
            const uint32_t c2 = elem.nodeLocalIds[2];
            const uint32_t c3 = elem.nodeLocalIds[3];
            const uint32_t m01 = elem.nodeLocalIds[4];
            const uint32_t m12 = elem.nodeLocalIds[5];
            const uint32_t m23 = elem.nodeLocalIds[6];
            const uint32_t m30 = elem.nodeLocalIds[7];

            if(const auto e = ctx.topology.findEdgeIndex(c0, c1)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m01);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c1, c2)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m12);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c2, c3)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m23);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c3, c0)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m30);
            }

            ctx.nodePairMidpoints.emplace(packNodePair(c0, c1), m01);
            ctx.nodePairMidpoints.emplace(packNodePair(c1, c2), m12);
            ctx.nodePairMidpoints.emplace(packNodePair(c2, c3), m23);
            ctx.nodePairMidpoints.emplace(packNodePair(c3, c0), m30);
        }
    }
}
```

- [ ] **Step 6: Implement `upgradeReplacementToSecondOrder`**

Add after `seedMidEdgeNodes`:

```cpp
void MeshSplitAlgorithm::upgradeReplacementToSecondOrder(
    SplitContext& ctx, SplitResult::ElementReplacement& rep) const {
    for(auto& child : rep.newElements) {
        if(child.type == MeshElementType::Triangle) {
            const uint32_t c0 = child.nodeLocalIds[0];
            const uint32_t c1 = child.nodeLocalIds[1];
            const uint32_t c2 = child.nodeLocalIds[2];
            child.nodeLocalIds[3] = getOrCreateMidpointByNodes(ctx, c0, c1);
            child.nodeLocalIds[4] = getOrCreateMidpointByNodes(ctx, c1, c2);
            child.nodeLocalIds[5] = getOrCreateMidpointByNodes(ctx, c2, c0);
            child.type = MeshElementType::Tri6;
        } else if(child.type == MeshElementType::Quad) {
            const uint32_t c0 = child.nodeLocalIds[0];
            const uint32_t c1 = child.nodeLocalIds[1];
            const uint32_t c2 = child.nodeLocalIds[2];
            const uint32_t c3 = child.nodeLocalIds[3];
            child.nodeLocalIds[4] = getOrCreateMidpointByNodes(ctx, c0, c1);
            child.nodeLocalIds[5] = getOrCreateMidpointByNodes(ctx, c1, c2);
            child.nodeLocalIds[6] = getOrCreateMidpointByNodes(ctx, c2, c3);
            child.nodeLocalIds[7] = getOrCreateMidpointByNodes(ctx, c3, c0);
            child.type = MeshElementType::Quad8;
        }
    }
}
```

- [ ] **Step 7: Modify `compute()` routing and SplitContext initialization**

In `compute()`, update the SplitContext aggregate initialization (line 406-408):

Before:
```cpp
    SplitContext ctx{
        entry, topology, result, {}, {}, static_cast<uint32_t>(entry.nodes.size()) + 1U,
    };
```

After:
```cpp
    SplitContext ctx{
        entry,
        topology,
        result,
        {},
        {},
        static_cast<uint32_t>(entry.nodes.size()) + 1U,
        {},
        static_cast<uint32_t>(entry.nodes.size()),
    };

    seedMidEdgeNodes(ctx);
```

Update Pass 1 routing (lines 451-456):

Before:
```cpp
        const auto& element = entry.elements[element_index];
        if(element.type == MeshElementType::Triangle) {
            processTriangleEdges(ctx, element_index, edge_indices, mode);
        } else if(element.type == MeshElementType::Quad) {
            processQuadEdges(ctx, element_index, edge_indices, mode);
        }
```

After:
```cpp
        const auto& element = entry.elements[element_index];
        const auto linear_type = linearEquivalent(element.type);
        if(linear_type == MeshElementType::Triangle) {
            processTriangleEdges(ctx, element_index, edge_indices, mode);
            if(isSecondOrder(element.type)) {
                upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
            }
        } else if(linear_type == MeshElementType::Quad) {
            processQuadEdges(ctx, element_index, edge_indices, mode);
            if(isSecondOrder(element.type)) {
                upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
            }
        }
```

Update Pass 3 node-based routing (lines 475-480):

Before:
```cpp
        const auto& element = entry.elements[element_index];
        if(element.type == MeshElementType::Triangle && node_ids.size() == 3U &&
           (static_cast<uint8_t>(mode) & static_cast<uint8_t>(SplitMode::TriaThree)) != 0U) {
            ctx.processedElements.insert(element_index);
            processTriangleNodes(ctx, element_index);
        }
```

After:
```cpp
        const auto& element = entry.elements[element_index];
        const auto linear_type = linearEquivalent(element.type);
        if(linear_type == MeshElementType::Triangle && node_ids.size() == 3U &&
           (static_cast<uint8_t>(mode) & static_cast<uint8_t>(SplitMode::TriaThree)) != 0U) {
            ctx.processedElements.insert(element_index);
            processTriangleNodes(ctx, element_index);
            if(isSecondOrder(element.type)) {
                upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
            }
        }
```

- [ ] **Step 8: Modify `processNeighborCut()` routing**

In `processNeighborCut()`, change the type checks to use `linearEquivalent()`:

Before (line 178):
```cpp
    if(element.type == MeshElementType::Triangle) {
```

After:
```cpp
    const auto linear_type = linearEquivalent(element.type);
    if(linear_type == MeshElementType::Triangle) {
```

After the `push_back(std::move(replacement))` on line 186, before `return;`, add:
```cpp
        if(isSecondOrder(element.type)) {
            upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
        }
```

Before (line 190):
```cpp
    if(element.type == MeshElementType::Quad) {
```

After:
```cpp
    if(linear_type == MeshElementType::Quad) {
```

After the `push_back(std::move(replacement))` on line 222, add:
```cpp
        if(isSecondOrder(element.type)) {
            upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
        }
```

- [ ] **Step 9: Build and verify**

Run:
```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh opengeolab_mesh_test --parallel 4"
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

Expected: Build succeeds, all existing tests still pass. The new code is not yet exercised (no second-order inputs in existing tests).

- [ ] **Step 10: Commit**

```bash
git add src/libs/mesh/include/opengeolab/mesh/mesh_split_algorithm.hpp src/libs/mesh/src/mesh_split_algorithm.cpp
git commit -m "feat(mesh): implement second-order split-then-upgrade core

Add seedMidEdgeNodes to pre-seed edge/node-pair caches with existing
mid-edge nodes from Tri6/Quad8 elements. Route compute() and
processNeighborCut() through linearEquivalent(). After each split,
upgradeReplacementToSecondOrder() converts Triangle->Tri6 and
Quad->Quad8 by computing sub-edge midpoints via the node-pair cache."
```

---

### Task 3: Tri6 Split Tests

**Files:**
- Modify: `src/libs/mesh/test/mesh_split_algorithm_test.cpp`

**Test fixture — single Tri6:**

Nodes: c0(0,0,0), c1(2,0,0), c2(1,2,0), m01(1,0,0), m12(1.5,1,0), m20(0.5,1,0)

```
    c2(3)
    / \
 m20(6)  m12(5)
  /       \
c0(1)--m01(4)--c1(2)
```

Topology edges (corner-to-corner): edge0={0,1}(c0-c1), edge1={1,2}(c1-c2), edge2={2,0}(c2-c0)

- [ ] **Step 1: Add `makeSingleTri6` fixture**

At the end of the fixture section in `mesh_split_algorithm_test.cpp` (after `makeSingleQuad`, ~line 77), add:

```cpp
static MeshEntry makeSingleTri6() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},  // localId 1 = c0
        MeshNode{{2.0F, 0.0F, 0.0F}},  // localId 2 = c1
        MeshNode{{1.0F, 2.0F, 0.0F}},  // localId 3 = c2
        MeshNode{{1.0F, 0.0F, 0.0F}},  // localId 4 = m01
        MeshNode{{1.5F, 1.0F, 0.0F}},  // localId 5 = m12
        MeshNode{{0.5F, 1.0F, 0.0F}},  // localId 6 = m20
    };
    MeshElement tri6{};
    tri6.type = MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};
    return entry;
}
```

- [ ] **Step 2: Write Tri6 1-edge test**

Select edge c0-c1: split into 2 children, each upgraded to Tri6.

Linear split produces:
- Triangle(c0=1, mid=m01=4, c2=3)
- Triangle(m01=4, c1=2, c2=3)

Mid-edge node m01 is pre-seeded, so `getOrCreateMidpoint` returns existing node 4 → **0 new split-point nodes**.

Upgrade each Triangle to Tri6 creates 3 sub-midpoints per child:
- Child 1 (c0, m01, c2): edges (1,4), (4,3), (3,1)
  - (1,4) → NEW node 7 at (0.5, 0, 0)
  - (4,3) → NEW node 8 at (1, 1, 0)
  - (3,1) → PRE-SEEDED → m20 = 6
- Child 2 (m01, c1, c2): edges (4,2), (2,3), (3,4)
  - (4,2) → NEW node 9 at (1.5, 0, 0)
  - (2,3) → PRE-SEEDED → m12 = 5
  - (3,4) → REUSE node 8

**Total: 3 new nodes** (7, 8, 9). Pre-seeded reuse: m20, m12. Cross-child shared: node 8.

Add at the end of the test file:

```cpp
TEST_CASE("MeshSplitAlgorithm: Tri6 1-edge -> 2 Tri6") {
    auto entry = makeSingleTri6();
    const auto topo = MeshTopology::build(entry);
    REQUIRE(topo.edges.size() == 3);

    const auto edge01 = topo.findEdgeIndex(1, 2);
    REQUIRE(edge01.has_value());

    const MeshSplitAlgorithm algo;
    const auto result = algo.compute(entry, topo, {edge01.value() + 1U}, {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 2);
    CHECK(result.newNodes.size() == 3);

    for(const auto& child : result.replacements[0].newElements) {
        CHECK(child.type == MeshElementType::Tri6);
    }

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 2);
    CHECK(entry.nodes.size() == 9); // 6 original + 3 new
}
```

- [ ] **Step 3: Run test and verify pass**

Run:
```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4"
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

Expected: All tests pass, including the new Tri6 1-edge test.

- [ ] **Step 4: Write Tri6 3-edge TriaFour test**

Select all 3 edges → 4 children, each upgraded to Tri6.

All 3 midpoints pre-seeded → **0 new split-point nodes**.

Linear split produces 4 Triangles:
- Tri(c0=1, m01=4, m20=6)
- Tri(m01=4, c1=2, m12=5)
- Tri(m20=6, m12=5, c2=3)
- Tri(m01=4, m12=5, m20=6) — center triangle

Upgrade creates 3 sub-midpoints per child (12 total), with 3 shared between center and outer triangles:
- **9 unique new nodes** total.

```cpp
TEST_CASE("MeshSplitAlgorithm: Tri6 3-edge TriaFour -> 4 Tri6") {
    auto entry = makeSingleTri6();
    const auto topo = MeshTopology::build(entry);

    const auto e01 = topo.findEdgeIndex(1, 2);
    const auto e12 = topo.findEdgeIndex(2, 3);
    const auto e20 = topo.findEdgeIndex(3, 1);
    REQUIRE(e01.has_value());
    REQUIRE(e12.has_value());
    REQUIRE(e20.has_value());

    const MeshSplitAlgorithm algo;
    const auto result = algo.compute(entry, topo,
                                     {e01.value() + 1U, e12.value() + 1U, e20.value() + 1U},
                                     {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 4);
    CHECK(result.newNodes.size() == 9);

    for(const auto& child : result.replacements[0].newElements) {
        CHECK(child.type == MeshElementType::Tri6);
    }

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 4);
    CHECK(entry.nodes.size() == 15); // 6 original + 9 new
}
```

- [ ] **Step 5: Run test and verify pass**

Same build+test command as Step 3. Expected: All pass.

- [ ] **Step 6: Write Tri6 TriaThree node-split test**

Select all 3 corner nodes with TriaThree mode → centroid-based 3-way split, each child upgraded to Tri6.

centroid = avg of corners = ((0+2+1)/3, (0+0+2)/3, 0) = (1, 2/3, 0)

New nodes: 1 centroid + 3 sub-midpoints for centroid-to-corner edges = **4 new nodes total**.
(The 3 original mid-edge nodes m01, m12, m20 are pre-seeded and reused.)

```cpp
TEST_CASE("MeshSplitAlgorithm: Tri6 TriaThree node-split -> 3 Tri6") {
    auto entry = makeSingleTri6();
    const auto topo = MeshTopology::build(entry);

    const MeshSplitAlgorithm algo;
    const auto result = algo.compute(entry, topo, {}, {1, 2, 3}, SplitMode::TriaThree);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 3);
    CHECK(result.newNodes.size() == 4); // 1 centroid + 3 sub-midpoints

    for(const auto& child : result.replacements[0].newElements) {
        CHECK(child.type == MeshElementType::Tri6);
    }

    // Verify centroid position (first new node)
    CHECK(result.newNodes[0].x == doctest::Approx(1.0));
    CHECK(result.newNodes[0].y == doctest::Approx(2.0 / 3.0));
    CHECK(result.newNodes[0].z == doctest::Approx(0.0));

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 3);
    CHECK(entry.nodes.size() == 10); // 6 original + 4 new
}
```

- [ ] **Step 7: Run test and verify pass**

Same build+test command. Expected: All pass.

- [ ] **Step 8: Commit**

```bash
git add src/libs/mesh/test/mesh_split_algorithm_test.cpp
git commit -m "test(mesh): add Tri6 split algorithm tests

Cover 1-edge (2 Tri6), 3-edge TriaFour (4 Tri6), and TriaThree
node-split (3 Tri6). Verify element types, new node counts,
centroid positions, and applySplitResult integrity."
```

---

### Task 4: Quad8 Split Tests

**Files:**
- Modify: `src/libs/mesh/test/mesh_split_algorithm_test.cpp`

**Test fixture — single Quad8:**

Nodes: c0(0,0,0), c1(2,0,0), c2(2,2,0), c3(0,2,0), m01(1,0,0), m12(2,1,0), m23(1,2,0), m30(0,1,0)

```
c3(4)---m23(7)---c2(3)
  |               |
m30(8)           m12(6)
  |               |
c0(1)---m01(5)---c1(2)
```

Topology edges: edge0={0,1}, edge1={1,2}, edge2={2,3}, edge3={3,0}

- [ ] **Step 1: Add `makeSingleQuad8` fixture**

After `makeSingleTri6`, add:

```cpp
static MeshEntry makeSingleQuad8() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},  // localId 1 = c0
        MeshNode{{2.0F, 0.0F, 0.0F}},  // localId 2 = c1
        MeshNode{{2.0F, 2.0F, 0.0F}},  // localId 3 = c2
        MeshNode{{0.0F, 2.0F, 0.0F}},  // localId 4 = c3
        MeshNode{{1.0F, 0.0F, 0.0F}},  // localId 5 = m01
        MeshNode{{2.0F, 1.0F, 0.0F}},  // localId 6 = m12
        MeshNode{{1.0F, 2.0F, 0.0F}},  // localId 7 = m23
        MeshNode{{0.0F, 1.0F, 0.0F}},  // localId 8 = m30
    };
    MeshElement quad8{};
    quad8.type = MeshElementType::Quad8;
    quad8.nodeLocalIds = {1, 2, 3, 4, 5, 6, 7, 8, 0};
    entry.elements = {quad8};
    return entry;
}
```

- [ ] **Step 2: Write Quad8 1-edge test**

Select edge c0-c1 (side 0): linear split produces Triangle(c0, m01, c3) + Quad(m01, c1, c2, c3).
Mid-edge node m01 pre-seeded → 0 split nodes.

Upgrade:
- Triangle(c0=1, m01=5, c3=4) → Tri6: edges (1,5)→NEW, (5,4)→NEW, (4,1)→m30=8
- Quad(m01=5, c1=2, c2=3, c3=4) → Quad8: edges (5,2)→NEW, (2,3)→m12=6, (3,4)→m23=7, (4,5)→REUSE

**3 new nodes**. Children: 1 Tri6 + 1 Quad8.

```cpp
TEST_CASE("MeshSplitAlgorithm: Quad8 1-edge -> 1 Tri6 + 1 Quad8") {
    auto entry = makeSingleQuad8();
    const auto topo = MeshTopology::build(entry);
    REQUIRE(topo.edges.size() == 4);

    const auto edge01 = topo.findEdgeIndex(1, 2);
    REQUIRE(edge01.has_value());

    const MeshSplitAlgorithm algo;
    const auto result =
        algo.compute(entry, topo, {edge01.value() + 1U}, {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 2);
    CHECK(result.newNodes.size() == 3);

    const auto& children = result.replacements[0].newElements;
    // First child is Triangle->Tri6, second is Quad->Quad8
    CHECK(children[0].type == MeshElementType::Tri6);
    CHECK(children[1].type == MeshElementType::Quad8);

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 2);
    CHECK(entry.nodes.size() == 11); // 8 original + 3 new
}
```

- [ ] **Step 3: Write Quad8 2-opposite test**

Select edges c0-c1 (side 0) and c2-c3 (side 2): opposite sides → 2 Quad children.

Both midpoints pre-seeded → 0 split nodes.

Upgrade 2 Quads to 2 Quad8:
- Quad(c0=1, m01=5, m23=7, c3=4) → Quad8: edges (1,5)→NEW, (5,7)→NEW, (7,4)→NEW, (4,1)→m30=8
- Quad(m01=5, c1=2, c2=3, m23=7) → Quad8: edges (5,2)→NEW, (2,3)→m12=6, (3,7)→NEW, (7,5)→REUSE

**5 new nodes** (A, B, C, D, E). Pre-seeded reuse: m30, m12. Cross-child shared: B=(5,7).

```cpp
TEST_CASE("MeshSplitAlgorithm: Quad8 2-opposite -> 2 Quad8") {
    auto entry = makeSingleQuad8();
    const auto topo = MeshTopology::build(entry);

    const auto edge01 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge01.has_value());
    REQUIRE(edge23.has_value());

    const MeshSplitAlgorithm algo;
    const auto result = algo.compute(
        entry, topo, {edge01.value() + 1U, edge23.value() + 1U}, {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 2);
    CHECK(result.newNodes.size() == 5);

    for(const auto& child : result.replacements[0].newElements) {
        CHECK(child.type == MeshElementType::Quad8);
    }

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 2);
    CHECK(entry.nodes.size() == 13); // 8 original + 5 new
}
```

- [ ] **Step 4: Write Quad8 4-edge test**

Select all 4 edges → center + 4 Quad children, all upgraded to Quad8.

All 4 midpoints pre-seeded → 0 split nodes. 1 centroid at (1,1,0).
Each child Quad gets 4 mid-edge sub-nodes; adjacent children share sub-nodes.

**13 new nodes** (1 center + 12 sub-midpoints, 4 shared between adjacent children → 16 - 4 + 1 = 13).

```cpp
TEST_CASE("MeshSplitAlgorithm: Quad8 4-edge -> 4 Quad8") {
    auto entry = makeSingleQuad8();
    const auto topo = MeshTopology::build(entry);

    const auto e01 = topo.findEdgeIndex(1, 2);
    const auto e12 = topo.findEdgeIndex(2, 3);
    const auto e23 = topo.findEdgeIndex(3, 4);
    const auto e30 = topo.findEdgeIndex(4, 1);
    REQUIRE(e01.has_value());
    REQUIRE(e12.has_value());
    REQUIRE(e23.has_value());
    REQUIRE(e30.has_value());

    const MeshSplitAlgorithm algo;
    const auto result =
        algo.compute(entry, topo,
                     {e01.value() + 1U, e12.value() + 1U, e23.value() + 1U, e30.value() + 1U},
                     {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 4);
    CHECK(result.newNodes.size() == 13);

    for(const auto& child : result.replacements[0].newElements) {
        CHECK(child.type == MeshElementType::Quad8);
    }

    // Verify center position (first new node)
    CHECK(result.newNodes[0].x == doctest::Approx(1.0));
    CHECK(result.newNodes[0].y == doctest::Approx(1.0));
    CHECK(result.newNodes[0].z == doctest::Approx(0.0));

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 4);
    CHECK(entry.nodes.size() == 21); // 8 original + 13 new
}
```

- [ ] **Step 5: Run all tests and verify pass**

Run:
```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4"
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 6: Commit**

```bash
git add src/libs/mesh/test/mesh_split_algorithm_test.cpp
git commit -m "test(mesh): add Quad8 split algorithm tests

Cover 1-edge (Tri6+Quad8), 2-opposite (2 Quad8), and 4-edge (4 Quad8).
Verify element types, new node counts, center position, and
applySplitResult integrity."
```

---

### Task 5: Guard Update + Mixed-Order Tests

**Files:**
- Modify: `src/libs/mesh/src/action/split_mesh_action.cpp:216-221`
- Modify: `src/libs/mesh/test/split_mesh_action_test.cpp:145-164`
- Modify: `src/libs/mesh/test/mesh_split_algorithm_test.cpp`

- [ ] **Step 1: Update guard in `split_mesh_action.cpp`**

Replace lines 216-221:

Before:
```cpp
    for(const auto& element : entry.elements) {
        if(element.type != MeshElementType::Triangle && element.type != MeshElementType::Quad) {
            return makeFailure("Mesh split is not supported for second-order elements. "
                               "Convert to linear elements first.");
        }
    }
```

After:
```cpp
    for(const auto& element : entry.elements) {
        if(element.type != MeshElementType::Triangle && element.type != MeshElementType::Quad &&
           element.type != MeshElementType::Tri6 && element.type != MeshElementType::Quad8) {
            const auto* message = element.type == MeshElementType::Quad9
                                      ? "Quad9 element splitting is not supported."
                                      : "Mesh split is not supported for this element type.";
            return makeFailure(message);
        }
    }
```

- [ ] **Step 2: Update existing Tri6 rejection test**

In `split_mesh_action_test.cpp`, the test "SplitMeshAction: rejects second-order elements" (lines 145-164) currently expects Tri6 to be rejected. Change it to expect **success**:

Replace the entire test case (lines 145-164):

```cpp
TEST_CASE("SplitMeshAction: accepts Tri6 elements") {
    MeshStore store;
    MeshEntry entry;
    entry.shapeId = 99;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 0.0F, 0.0F}}, MeshNode{{1.0F, 2.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}}, MeshNode{{1.5F, 1.0F, 0.0F}}, MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    MeshElement tri6{};
    tri6.type = MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};
    store.setMesh(99, std::move(entry));

    SplitMeshAction action(store);
    const auto result = action.execute(
        {{"shapeId", 99}, {"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);
    CHECK(result["ok"].get<bool>() == true);
}
```

- [ ] **Step 3: Add Quad9 rejection test**

After the updated test, add:

```cpp
TEST_CASE("SplitMeshAction: rejects Quad9 elements") {
    MeshStore store;
    MeshEntry entry;
    entry.shapeId = 100;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 2.0F, 0.0F}},
        MeshNode{{0.0F, 2.0F, 0.0F}}, MeshNode{{1.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 1.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}}, MeshNode{{0.0F, 1.0F, 0.0F}}, MeshNode{{1.0F, 1.0F, 0.0F}},
    };
    MeshElement quad9{};
    quad9.type = MeshElementType::Quad9;
    quad9.nodeLocalIds = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    entry.elements = {quad9};
    store.setMesh(100, std::move(entry));

    SplitMeshAction action(store);
    const auto result = action.execute(
        {{"shapeId", 100}, {"selections", {{{"type", "edge"}, {"localId", 1}}}}}, nullptr);
    CHECK(result["ok"].get<bool>() == false);
    CHECK(result["summary"].get<std::string>().find("Quad9") != std::string::npos);
}
```

- [ ] **Step 4: Add mixed-order neighbor test**

This tests a Tri6 element adjacent to a linear Triangle, sharing edge c1-c2. Splitting the shared edge should produce:
- Tri6 → 2 Tri6 children (upgraded)
- Triangle → 2 Triangle children (not upgraded)

Add fixture after `makeSingleQuad8`:

```cpp
static MeshEntry makeTri6AndTriangleSharedEdge() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},   // localId 1 = c0 (Tri6 only)
        MeshNode{{2.0F, 0.0F, 0.0F}},   // localId 2 = c1 (shared corner)
        MeshNode{{1.0F, 2.0F, 0.0F}},   // localId 3 = c2 (shared corner)
        MeshNode{{1.0F, 0.0F, 0.0F}},   // localId 4 = m01 (Tri6 mid-edge)
        MeshNode{{1.5F, 1.0F, 0.0F}},   // localId 5 = m12 (Tri6 mid-edge)
        MeshNode{{0.5F, 1.0F, 0.0F}},   // localId 6 = m20 (Tri6 mid-edge)
        MeshNode{{3.0F, 2.0F, 0.0F}},   // localId 7 = extra corner (Triangle only)
    };
    MeshElement tri6{};
    tri6.type = MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};

    MeshElement tri{};
    tri.type = MeshElementType::Triangle;
    tri.nodeLocalIds = {2, 7, 3, 0, 0, 0, 0, 0, 0};

    entry.elements = {tri6, tri};
    return entry;
}
```

Add test:

```cpp
TEST_CASE("MeshSplitAlgorithm: mixed Tri6 + Triangle shared edge") {
    auto entry = makeTri6AndTriangleSharedEdge();
    const auto topo = MeshTopology::build(entry);

    // Select the shared edge c1-c2 (localIds 2-3)
    const auto shared_edge = topo.findEdgeIndex(2, 3);
    REQUIRE(shared_edge.has_value());

    const MeshSplitAlgorithm algo;
    const auto result =
        algo.compute(entry, topo, {shared_edge.value() + 1U}, {}, SplitMode::TriaFour);

    // Both elements split: Tri6 → 2 children, Triangle → 2 children (neighbor cut)
    CHECK(result.replacements.size() == 2);

    // Find which replacement is for which element
    const auto& rep0 = result.replacements[0];
    const auto& rep1 = result.replacements[1];

    // Count Tri6 and Triangle children across all replacements
    uint32_t tri6_count = 0;
    uint32_t tri_count = 0;
    for(const auto& rep : result.replacements) {
        for(const auto& child : rep.newElements) {
            if(child.type == MeshElementType::Tri6) {
                ++tri6_count;
            } else if(child.type == MeshElementType::Triangle) {
                ++tri_count;
            }
        }
    }
    CHECK(tri6_count == 2);  // Tri6 element produces 2 Tri6 children
    CHECK(tri_count == 2);   // Triangle element produces 2 Triangle children

    applySplitResult(entry, result);
    CHECK(entry.elements.size() == 4);
}
```

- [ ] **Step 5: Run all tests and verify pass**

Run:
```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh opengeolab_mesh_test --parallel 4"
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 6: Commit**

```bash
git add src/libs/mesh/src/action/split_mesh_action.cpp src/libs/mesh/test/split_mesh_action_test.cpp src/libs/mesh/test/mesh_split_algorithm_test.cpp
git commit -m "feat(mesh): update guard to allow Tri6/Quad8, reject Quad9

Allow Tri6 and Quad8 elements through the split action guard.
Quad9 and 3D element types are rejected with specific messages.
Add Quad9 rejection test and mixed-order Tri6+Triangle neighbor test."
```

---

### Task 6: Build Verification + clang-format

**Files:** All modified files from Tasks 1-5.

- [ ] **Step 1: Full build**

```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --parallel 4"
```

Expected: Full project builds with no errors.

- [ ] **Step 2: Full test suite**

```bash
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: All mesh tests pass. Pre-existing camera test failure in `opengeolab_app_test` is unrelated.

- [ ] **Step 3: clang-format check**

```bash
clang-format -i src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp src/libs/mesh/include/opengeolab/mesh/mesh_split_algorithm.hpp src/libs/mesh/src/mesh_split_algorithm.cpp src/libs/mesh/src/action/split_mesh_action.cpp src/libs/mesh/test/mesh_split_algorithm_test.cpp src/libs/mesh/test/split_mesh_action_test.cpp
```

If any files changed, commit:
```bash
git add -u
git commit -m "style(mesh): apply clang-format to second-order split code"
```

- [ ] **Step 4: Final verification**

Rebuild and retest after formatting:
```bash
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat\" x64 >nul 2>&1 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh opengeolab_mesh_test --parallel 4"
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

Expected: All pass. Phase 4 complete.
