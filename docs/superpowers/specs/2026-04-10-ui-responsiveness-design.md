# UI Responsiveness: Batch Selection + Async Scene Bridge

## Problem Statement

Three layers of UI responsiveness issues occur when working with large BREP models
(831 faces, 43K triangles):

1. **Box selection lag**: Highlighting appears after noticeable delay when box-selecting
   many faces. Root cause: 831 individual `addSelection()` calls, each acquiring a
   unique_lock, incrementing the version counter, and emitting a synchronous signal.
   The next render frame re-resolves all 831 entities' DrawRanges from scratch.

2. **Delete operation freeze**: UI freezes (rotation sluggish, progress inaccurate)
   during `DeleteEntityAction`. Root cause: `GeometrySceneBridge::onShapeUpdated()`
   runs synchronously on the worker thread, calling CPU-heavy `buildRenderData()` and
   then `configureNode()` which acquires SceneGraph's write lock. The render thread's
   `synchronize()` blocks waiting for the read lock, which in turn blocks the main
   thread (Qt Quick threaded rendering model).

3. **Post-delete visual delay**: After the operation completes, faces don't disappear
   immediately. Root cause: full GPU buffer rebuild triggered by scene version change.

## Scope

This design addresses problems (1) and (2). Problem (3) is deferred as the GPU buffer
rebuild itself is fast (few milliseconds for 43K triangles); the perceived delay is
primarily caused by the lock contention that problem (2) addresses.

## Architecture Overview

```
BEFORE:
  Worker thread: action → store.replaceShape()
    → [Kangaroo sync signal] → onShapeUpdated()
      → buildRenderData() [CPU heavy, 10-50ms]
      → configureNode() [SceneGraph WRITE lock]      ← blocks render thread
    → store.tessellate()
      → [same path, even heavier]

AFTER:
  Worker thread: action → store.replaceShape()
    → [Kangaroo sync signal] → onShapeUpdated()
      → enqueue PendingShapeUpdate                    ← microseconds, no locks
    → store.tessellate()
      → [same lightweight enqueue]

  Render thread: synchronize()
    → processPendingUpdates()
      → buildRenderData() + configureNode()           ← safe, main thread paused
    → pipeline.synchronize()
```

## Design

### Section 1: SelectionState Batch API

**Goal**: Reduce 831 lock acquisitions + signal emissions to 1.

#### New Interface

```cpp
// selection_state.hpp
void addSelections(std::span<const Core::EntityRef> entities);
void removeSelections(std::span<const Core::EntityRef> entities);

// Replace old per-entity signals with batch versions:
Kangaroo::Util::Signal<std::vector<Core::EntityRef>> entitiesSelected;
Kangaroo::Util::Signal<std::vector<Core::EntityRef>> entitiesDeselected;
Kangaroo::Util::Signal<> selectionCleared;  // unchanged
```

#### Implementation

`addSelections()`:
1. Filter invalid entities.
2. Acquire `unique_lock` once.
3. Sort the input, merge into `m_selections` (set-union), collecting actually-added
   entries.
4. Increment `m_selectionVersion` once.
5. Release lock.
6. Emit `entitiesSelected` with the actually-added vector.

`removeSelections()`:
1. Acquire `unique_lock` once.
2. Set-difference from `m_selections`, collecting actually-removed entries.
3. Increment `m_selectionVersion` once.
4. Release lock.
5. Emit `entitiesDeselected` with the actually-removed vector.

Single-entity `addSelection()` / `removeSelection()` delegate to the batch versions.

Old signals `entitySelected` / `entityDeselected` are removed. All listeners migrate
to the batch signals (single-select produces a 1-element vector).

#### Callers to Update

- `gl_viewport_renderer.cpp:468-480` — change loop to `sel.addSelections(entities)`
- `gl_viewport_renderer.cpp:502-520` — same for `dispatchPickAreaResults()`
- All signal listeners connected to `entitySelected` / `entityDeselected`

### Section 2: Async Scene Bridge

**Goal**: Move CPU-heavy scene building off the worker thread so it does not hold
SceneGraph write locks and does not block the render thread.

All three ShapeStore signal handlers (`onShapeAdded`, `onShapeUpdated`, `onShapeRemoved`)
currently perform SceneGraph mutations on the worker thread. All three are deferred.

#### New Data Structure

```cpp
// geometry_scene_bridge.hpp (private)
enum class PendingAction : uint8_t { Add, Update, Remove };

struct PendingShapeOp {
    PendingAction action;
    uint32_t shapeId;
    Geometry::ShapeEntry entry;  // shared_ptr<VisualData> keeps data alive; empty for Remove
};

std::mutex m_pendingMutex;
std::vector<PendingShapeOp> m_pendingOps;
```

#### Modified Signal Handlers

All three handlers become lightweight enqueue operations:

```cpp
void GeometrySceneBridge::onShapeAdded(uint32_t shape_id,
                                        const Geometry::ShapeEntry& entry) {
    m_scene.topologyIndex().buildForShape(shape_id, entry);
    std::lock_guard lock(m_pendingMutex);
    m_pendingOps.push_back({PendingAction::Add, shape_id, entry});
}

void GeometrySceneBridge::onShapeUpdated(uint32_t shape_id,
                                          const Geometry::ShapeEntry& entry) {
    m_scene.topologyIndex().buildForShape(shape_id, entry);
    std::lock_guard lock(m_pendingMutex);
    m_pendingOps.push_back({PendingAction::Update, shape_id, entry});
}

void GeometrySceneBridge::onShapeRemoved(uint32_t shape_id) {
    std::lock_guard lock(m_pendingMutex);
    m_pendingOps.push_back({PendingAction::Remove, shape_id, {}});
}
```

#### New `processPendingUpdates()`

```cpp
// Called by render thread during synchronize(), BEFORE scene readLock
void GeometrySceneBridge::processPendingUpdates() {
    std::vector<PendingShapeOp> ops;
    {
        std::lock_guard lock(m_pendingMutex);
        ops.swap(m_pendingOps);
    }

    for (auto& op : ops) {
        switch (op.action) {
        case PendingAction::Add:    processShapeAdd(op.shapeId, op.entry); break;
        case PendingAction::Update: processShapeUpdate(op.shapeId, op.entry); break;
        case PendingAction::Remove: processShapeRemove(op.shapeId); break;
        }
    }
}
```

- `processShapeAdd()` = current `onShapeAdded()` body minus topologyIndex call
- `processShapeUpdate()` = current `onShapeUpdated()` body minus topologyIndex call
- `processShapeRemove()` = current `onShapeRemoved()` body minus topologyIndex call

#### GLViewportRenderer Integration

```cpp
// gl_viewport_renderer.cpp::synchronize()
if (const auto* scene = viewport->sceneGraph(); scene != nullptr) {
    // Process pending scene updates first (acquires write lock internally)
    m_sceneBridge->processPendingUpdates();

    // Then read-lock for pipeline sync
    [[maybe_unused]] auto lock = scene->readLock();
    m_pipeline.synchronize(*scene);
}
```

#### Thread Safety

- `m_pendingMutex` protects the queue (worker writes, render thread reads)
- `processPendingUpdates()` runs on render thread during `synchronize()`, while main
  thread is paused (Qt Quick model) — no contention with QML
- SceneGraph write lock in `configureNode()` is held briefly on render thread, not
  competing with itself
- Worker thread is free to continue processing the next action while scene updates are
  queued

#### Access to GeometrySceneBridge from GLViewportRenderer

GLViewportRenderer needs access to the bridge's `processPendingUpdates()`. Options:
- **Via SceneGraph**: Add a `processDeferred()` method to SceneGraph that delegates
  to registered bridges. SceneGraph already owns the bridge registration concept.
- **Direct reference**: Pass a `GeometrySceneBridge*` to GLViewportRenderer at
  construction. Simpler but couples app layer to scene internals.

Recommended: Route through SceneGraph with a generic deferred-update mechanism:
```cpp
// scene_graph.hpp
void processDeferredUpdates();  // calls all registered deferred processors
```

This keeps GLViewportRenderer decoupled from specific bridge implementations.

### Signal Migration Checklist

All current listeners of `entitySelected` / `entityDeselected` must migrate:

| Listener | File:Line | Current Signature | Change |
|----------|-----------|-------------------|--------|
| SelectionService | `selection_service.cpp:179` | `entitySelected(EntityRef)` | Loop over vector, emit per-entity Qt signal + auto-label |
| SelectionService | `selection_service.cpp:198` | `entityDeselected(EntityRef)` | Loop over vector, emit per-entity Qt signal + remove label |
| SceneModule | `scene_module.cpp:73` | `entitySelected(EntityRef&)` | Emit `ViewportChanged` once per batch (not per entity) |
| SceneModule | `scene_module.cpp:76` | `entityDeselected(EntityRef&)` | Emit `ViewportChanged` once per batch (not per entity) |
| Tests | `selection_state_test.cpp:135,146` | Both signals | Update to batch signal, verify single emission |

## Testing Strategy

1. **Unit tests** for `addSelections()` / `removeSelections()`:
   - Empty input
   - Duplicates in input
   - Partial overlap with existing selection
   - Signal emission count verification

2. **Integration test**: Verify `processPendingUpdates()` correctly builds render data
   when called from a non-worker thread context.

3. **Manual test**: Load large BREP model, box-select many faces, verify immediate
   highlight. Delete selected faces, verify UI stays responsive during operation.

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Pending updates accumulate if render thread is slow | Queue is bounded by action rate; typical: 1-2 updates per delete operation |
| Signal migration breaks existing listeners | Audit all `entitySelected.connect()` calls; compile-time breakage (signal type change) |
| Race between topology index (worker) and scene update (render) | TopologyIndex is independent of SceneGraph; no shared state |
| `processPendingUpdates()` makes `synchronize()` slower | Only processes queued items; cost is same as before, just on a different thread |

## Out of Scope (Future Work)

- Incremental GPU buffer upload (per-node dirty tracking)
- Async GPU readback for pick pass (PBO ping-pong)
- Progress callback granularity for multi-face defeaturing
- Lock-free SceneGraph with versioned snapshots
