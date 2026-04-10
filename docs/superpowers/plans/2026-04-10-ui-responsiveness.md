# UI Responsiveness: Batch Selection + Async Scene Bridge — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate UI freezes when box-selecting and deleting faces on large BREP models (831 faces, 43K triangles) by batching selection API calls and deferring scene bridge mutations to the render thread.

**Architecture:** Two independent improvements that together eliminate all lock contention between worker/render/main threads. Section 1 (Tasks 1–4) replaces per-entity selection signals with a batch API, reducing 831 lock acquisitions to 1. Section 2 (Tasks 5–8) moves SceneGraph write-lock mutations from the worker thread to the render thread via a deferred-update queue, so the worker never blocks the render thread.

**Tech Stack:** C++20, Kangaroo Signal (synchronous observer), Qt Quick threaded rendering, doctest, `std::shared_mutex`, `std::span`

**Design Spec:** `docs/superpowers/specs/2026-04-10-ui-responsiveness-design.md`

---

## File Map

### Section 1: Batch Selection API
| Action | File | Responsibility |
|--------|------|----------------|
| Modify | `src/libs/scene/include/opengeolab/scene/selection_state.hpp` | Add `addSelections()`/`removeSelections()`, replace per-entity signals with batch signals |
| Modify | `src/libs/scene/src/selection_state.cpp` | Batch implementation: sorted merge + single version bump |
| Modify | `src/libs/scene/test/selection_state_test.cpp` | New batch tests + migrate old signal tests |
| Modify | `src/libs/scene/src/scene_module.cpp:73-78` | Migrate to batch signals |
| Modify | `src/app/src/selection_service.cpp:179-214` | Migrate to batch signals |
| Modify | `src/app/src/gl_viewport_renderer.cpp:468-480,528-540` | Call `addSelections()`/`removeSelections()` instead of loop |

### Section 2: Async Scene Bridge
| Action | File | Responsibility |
|--------|------|----------------|
| Modify | `src/libs/scene/include/opengeolab/scene/scene_graph.hpp` | Add `registerDeferredProcessor()` + `processDeferredUpdates()` |
| Modify | `src/libs/scene/src/scene_graph.cpp` | Implementation of deferred processor registry |
| Modify | `src/libs/scene/include/opengeolab/scene/geometry_scene_bridge.hpp` | Add `PendingShapeOp`, pending queue, `processPendingUpdates()` |
| Modify | `src/libs/scene/src/geometry_scene_bridge.cpp` | Refactor handlers to enqueue; add process methods |
| Modify | `src/libs/scene/test/geometry_scene_bridge_test.cpp` | Add `processDeferredUpdates()` calls after store ops |
| Modify | `src/app/src/gl_viewport_renderer.cpp:99-102` | Call `processDeferredUpdates()` before `readLock()` |

---

## Task 1: SelectionState Batch API — Write Failing Tests

**Files:**
- Modify: `src/libs/scene/test/selection_state_test.cpp`

- [ ] **Step 1: Add batch addSelections tests**

Add the following test cases at the end of the `"SelectionState"` test suite (before the closing `}`), right after the `"selections are sorted"` test case at line 198:

```cpp
    TEST_CASE("addSelections batch — basic") {
        SelectionState state;
        const std::vector<EntityRef> batch = {FACE_1, EDGE_1, FACE_2};
        state.addSelections(batch);

        CHECK(state.selections().size() == 3);
        CHECK(state.isSelected(FACE_1));
        CHECK(state.isSelected(EDGE_1));
        CHECK(state.isSelected(FACE_2));
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("addSelections batch — empty input") {
        SelectionState state;
        state.addSelections({});
        CHECK(state.selections().empty());
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("addSelections batch — duplicates in input") {
        SelectionState state;
        const std::vector<EntityRef> batch = {FACE_1, FACE_1, FACE_2};
        state.addSelections(batch);

        CHECK(state.selections().size() == 2);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("addSelections batch — partial overlap with existing") {
        SelectionState state;
        state.addSelection(FACE_1);

        const std::vector<EntityRef> batch = {FACE_1, FACE_2, EDGE_1};
        state.addSelections(batch);

        CHECK(state.selections().size() == 3);
        CHECK(state.selectionVersion() == 2);
    }

    TEST_CASE("addSelections batch — signal emits actually added") {
        SelectionState state;
        state.addSelection(FACE_1);

        std::vector<Core::EntityRef> captured;
        auto conn =
            state.entitiesSelected.connect([&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        const std::vector<EntityRef> batch = {FACE_1, FACE_2, EDGE_1};
        state.addSelections(batch);

        REQUIRE(captured.size() == 2);
        CHECK(std::find(captured.begin(), captured.end(), FACE_2) != captured.end());
        CHECK(std::find(captured.begin(), captured.end(), EDGE_1) != captured.end());
    }

    TEST_CASE("addSelections batch — invalid entities filtered") {
        SelectionState state;
        const std::vector<EntityRef> batch = {FACE_1, EntityRef{}, EDGE_1};
        state.addSelections(batch);

        CHECK(state.selections().size() == 2);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("removeSelections batch — basic") {
        SelectionState state;
        state.addSelections({FACE_1, FACE_2, EDGE_1, VERTEX_1});

        const std::vector<EntityRef> to_remove = {FACE_1, EDGE_1};
        state.removeSelections(to_remove);

        CHECK(state.selections().size() == 2);
        CHECK(state.isSelected(FACE_2));
        CHECK(state.isSelected(VERTEX_1));
        CHECK_FALSE(state.isSelected(FACE_1));
        CHECK_FALSE(state.isSelected(EDGE_1));
    }

    TEST_CASE("removeSelections batch — empty input") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.removeSelections({});

        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("removeSelections batch — none present") {
        SelectionState state;
        state.addSelection(FACE_1);

        state.removeSelections({FACE_2, EDGE_1});
        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("removeSelections batch — signal emits actually removed") {
        SelectionState state;
        state.addSelections({FACE_1, FACE_2, EDGE_1});

        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesDeselected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        state.removeSelections({FACE_1, VERTEX_1});

        REQUIRE(captured.size() == 1);
        CHECK(captured[0] == FACE_1);
    }

    TEST_CASE("removeSelections batch — version increments once") {
        SelectionState state;
        state.addSelections({FACE_1, FACE_2, EDGE_1});
        const uint64_t before = state.selectionVersion();

        state.removeSelections({FACE_1, FACE_2});
        CHECK(state.selectionVersion() == before + 1);
    }
```

- [ ] **Step 2: Run tests to verify they fail**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 2>&1 | Select-Object -Last 30
```

Expected: **Compilation fails** — `addSelections`, `removeSelections`, `entitiesSelected`, `entitiesDeselected` do not exist yet.

---

## Task 2: SelectionState Batch API — Implementation

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/selection_state.hpp`
- Modify: `src/libs/scene/src/selection_state.cpp`

- [ ] **Step 1: Update the header**

In `selection_state.hpp`, add `<span>` include after the existing `<vector>` include:

```cpp
#include <span>
```

Add batch methods after the existing `removeSelection` declaration (line 45):

```cpp
    void addSelections(std::span<const Core::EntityRef> entities);
    void removeSelections(std::span<const Core::EntityRef> entities);
```

Replace the two per-entity signals (lines 57–58):

```cpp
    Kangaroo::Util::Signal<Core::EntityRef> entitySelected;
    Kangaroo::Util::Signal<Core::EntityRef> entityDeselected;
```

with batch signals:

```cpp
    Kangaroo::Util::Signal<std::vector<Core::EntityRef>> entitiesSelected;
    Kangaroo::Util::Signal<std::vector<Core::EntityRef>> entitiesDeselected;
```

- [ ] **Step 2: Implement batch methods**

In `selection_state.cpp`, add `<span>` include and rewrite the four methods.

Replace the `addSelection` implementation (lines 47–61) with delegation to batch:

```cpp
void SelectionState::addSelection(const Core::EntityRef& entity) {
    addSelections(std::span<const Core::EntityRef>(&entity, 1));
}
```

Replace the `removeSelection` implementation (lines 63–74) with delegation to batch:

```cpp
void SelectionState::removeSelection(const Core::EntityRef& entity) {
    removeSelections(std::span<const Core::EntityRef>(&entity, 1));
}
```

Add the batch `addSelections` implementation after `removeSelection`:

```cpp
void SelectionState::addSelections(std::span<const Core::EntityRef> entities) {
    // 1. Filter invalid and sort + deduplicate input.
    std::vector<Core::EntityRef> sorted;
    sorted.reserve(entities.size());
    for(const auto& e : entities) {
        if(e.isValid()) {
            sorted.push_back(e);
        }
    }
    if(sorted.empty()) {
        return;
    }
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    // 2. Merge into m_selections, collecting actually-added entries.
    std::vector<Core::EntityRef> added;
    {
        std::unique_lock lock(m_mutex);
        added.reserve(sorted.size());
        std::vector<Core::EntityRef> merged;
        merged.reserve(m_selections.size() + sorted.size());

        auto sel_it = m_selections.begin();
        auto new_it = sorted.begin();
        while(sel_it != m_selections.end() && new_it != sorted.end()) {
            if(*sel_it < *new_it) {
                merged.push_back(*sel_it);
                ++sel_it;
            } else if(*new_it < *sel_it) {
                added.push_back(*new_it);
                merged.push_back(*new_it);
                ++new_it;
            } else {
                // duplicate — already selected
                merged.push_back(*sel_it);
                ++sel_it;
                ++new_it;
            }
        }
        while(sel_it != m_selections.end()) {
            merged.push_back(*sel_it);
            ++sel_it;
        }
        while(new_it != sorted.end()) {
            added.push_back(*new_it);
            merged.push_back(*new_it);
            ++new_it;
        }

        if(added.empty()) {
            return;
        }
        m_selections = std::move(merged);
        ++m_selectionVersion;
    }
    entitiesSelected.emit(std::move(added));
}
```

Add the batch `removeSelections` implementation:

```cpp
void SelectionState::removeSelections(std::span<const Core::EntityRef> entities) {
    if(entities.empty()) {
        return;
    }

    std::vector<Core::EntityRef> sorted(entities.begin(), entities.end());
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    std::vector<Core::EntityRef> removed;
    {
        std::unique_lock lock(m_mutex);
        removed.reserve(sorted.size());
        std::vector<Core::EntityRef> remaining;
        remaining.reserve(m_selections.size());

        auto sel_it = m_selections.begin();
        auto rm_it = sorted.begin();
        while(sel_it != m_selections.end() && rm_it != sorted.end()) {
            if(*sel_it < *rm_it) {
                remaining.push_back(*sel_it);
                ++sel_it;
            } else if(*rm_it < *sel_it) {
                ++rm_it;
            } else {
                removed.push_back(*sel_it);
                ++sel_it;
                ++rm_it;
            }
        }
        while(sel_it != m_selections.end()) {
            remaining.push_back(*sel_it);
            ++sel_it;
        }

        if(removed.empty()) {
            return;
        }
        m_selections = std::move(remaining);
        ++m_selectionVersion;
    }
    entitiesDeselected.emit(std::move(removed));
}
```

- [ ] **Step 3: Build and verify new tests pass (old signal tests will fail — expected)**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4
```

Expected: Compilation may fail because old signal tests in Task 3 reference `entitySelected`/`entityDeselected` which no longer exist. That is expected — proceed to Task 3.

---

## Task 3: Migrate Signal Listeners + Update Existing Tests

**Files:**
- Modify: `src/libs/scene/test/selection_state_test.cpp:132-150`
- Modify: `src/libs/scene/src/scene_module.cpp:73-78`
- Modify: `src/app/src/selection_service.cpp:179-214`

- [ ] **Step 1: Update selection_state_test.cpp signal tests**

Replace the `"signal emitted on add"` test (lines 132–139):

```cpp
    TEST_CASE("signal emitted on add") {
        SelectionState state;
        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesSelected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        state.addSelection(FACE_1);
        REQUIRE(captured.size() == 1);
        CHECK(captured[0] == FACE_1);
    }
```

Replace the `"signal emitted on remove"` test (lines 141–150):

```cpp
    TEST_CASE("signal emitted on remove") {
        SelectionState state;
        state.addSelection(FACE_1);

        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesDeselected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        state.removeSelection(FACE_1);
        REQUIRE(captured.size() == 1);
        CHECK(captured[0] == FACE_1);
    }
```

- [ ] **Step 2: Update scene_module.cpp selection signal connections**

Replace lines 73–78 (entitySelected and entityDeselected connections):

```cpp
    m_graphConnections.push_back(sel.entitySelected.connect([this](const Core::EntityRef&) {
        dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
    }));
    m_graphConnections.push_back(sel.entityDeselected.connect([this](const Core::EntityRef&) {
        dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
    }));
```

with batch versions (single emit per batch):

```cpp
    m_graphConnections.push_back(
        sel.entitiesSelected.connect([this](const std::vector<Core::EntityRef>&) {
            dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
        }));
    m_graphConnections.push_back(
        sel.entitiesDeselected.connect([this](const std::vector<Core::EntityRef>&) {
            dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
        }));
```

- [ ] **Step 3: Update selection_service.cpp signal connections**

Replace the `entitySelected` connection (lines 179–195):

```cpp
    m_connections.push_back(m_state->entitySelected.connect([this](const Core::EntityRef& entity) {
        QMetaObject::invokeMethod(
            this,
            [this, entity]() {
                Q_EMIT entitySelected(static_cast<int>(entity.shapeId),
                                      static_cast<int>(entity.entityType),
                                      static_cast<int>(entity.localId));
                Q_EMIT selectionChanged();

                if(m_labelManager != nullptr && m_labelManager->autoLabel()) {
                    addLabelForSelection(static_cast<int>(entity.shapeId),
                                         static_cast<int>(entity.entityType),
                                         static_cast<int>(entity.localId));
                }
            },
            Qt::QueuedConnection);
    }));
```

with:

```cpp
    m_connections.push_back(
        m_state->entitiesSelected.connect([this](std::vector<Core::EntityRef> entities) {
            QMetaObject::invokeMethod(
                this,
                [this, entities = std::move(entities)]() {
                    for(const auto& entity : entities) {
                        Q_EMIT entitySelected(static_cast<int>(entity.shapeId),
                                              static_cast<int>(entity.entityType),
                                              static_cast<int>(entity.localId));
                        if(m_labelManager != nullptr && m_labelManager->autoLabel()) {
                            addLabelForSelection(static_cast<int>(entity.shapeId),
                                                 static_cast<int>(entity.entityType),
                                                 static_cast<int>(entity.localId));
                        }
                    }
                    Q_EMIT selectionChanged();
                },
                Qt::QueuedConnection);
        }));
```

Replace the `entityDeselected` connection (lines 197–214):

```cpp
    m_connections.push_back(
        m_state->entityDeselected.connect([this](const Core::EntityRef& entity) {
            QMetaObject::invokeMethod(
                this,
                [this, entity]() {
                    Q_EMIT entityDeselected(static_cast<int>(entity.shapeId),
                                            static_cast<int>(entity.entityType),
                                            static_cast<int>(entity.localId));
                    Q_EMIT selectionChanged();

                    if(m_labelManager != nullptr && m_labelManager->autoLabel()) {
                        removeLabelForSelection(static_cast<int>(entity.shapeId),
                                                static_cast<int>(entity.entityType),
                                                static_cast<int>(entity.localId));
                    }
                },
                Qt::QueuedConnection);
        }));
```

with:

```cpp
    m_connections.push_back(
        m_state->entitiesDeselected.connect([this](std::vector<Core::EntityRef> entities) {
            QMetaObject::invokeMethod(
                this,
                [this, entities = std::move(entities)]() {
                    for(const auto& entity : entities) {
                        Q_EMIT entityDeselected(static_cast<int>(entity.shapeId),
                                                static_cast<int>(entity.entityType),
                                                static_cast<int>(entity.localId));
                        if(m_labelManager != nullptr && m_labelManager->autoLabel()) {
                            removeLabelForSelection(static_cast<int>(entity.shapeId),
                                                    static_cast<int>(entity.entityType),
                                                    static_cast<int>(entity.localId));
                        }
                    }
                    Q_EMIT selectionChanged();
                },
                Qt::QueuedConnection);
        }));
```

- [ ] **Step 4: Check for any remaining references to old signals**

Run:
```powershell
git grep -n "entitySelected\." --  "*.cpp" "*.hpp" | Select-String -NotMatch "Q_EMIT|entitiesSelected|entitiesDeselected"
```

and

```powershell
git grep -n "entityDeselected\." --  "*.cpp" "*.hpp" | Select-String -NotMatch "Q_EMIT|entitiesSelected|entitiesDeselected"
```

Expected: No matches for old per-entity signal connections. Qt `Q_EMIT entitySelected(...)` in SelectionService is fine (that's a Qt signal, not the Kangaroo signal).

- [ ] **Step 5: Build and run scene tests**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All tests pass including the new batch tests.

- [ ] **Step 6: Build the app to verify SelectionService compiles**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Build succeeds.

- [ ] **Step 7: Run clang-format on changed files**

Run:
```powershell
clang-format -i src/libs/scene/include/opengeolab/scene/selection_state.hpp src/libs/scene/src/selection_state.cpp src/libs/scene/test/selection_state_test.cpp src/libs/scene/src/scene_module.cpp src/app/src/selection_service.cpp
```

- [ ] **Step 8: Commit**

```powershell
git add src/libs/scene/include/opengeolab/scene/selection_state.hpp src/libs/scene/src/selection_state.cpp src/libs/scene/test/selection_state_test.cpp src/libs/scene/src/scene_module.cpp src/app/src/selection_service.cpp
git commit -m "feat(selection): add batch addSelections/removeSelections API

Replace per-entity entitySelected/entityDeselected signals with batch
entitiesSelected/entitiesDeselected signals. Single-entity methods
delegate to batch versions. Reduces 831 lock acquisitions + signal
emissions to 1 for box-select operations.

Migrate all listeners: SceneModule, SelectionService, tests.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 4: GLViewportRenderer Batch Selection Calls

**Files:**
- Modify: `src/app/src/gl_viewport_renderer.cpp:466-481` (dispatchBoxSelectResults)
- Modify: `src/app/src/gl_viewport_renderer.cpp:526-541` (dispatchPickAreaResults)

- [ ] **Step 1: Update dispatchBoxSelectResults**

Replace the per-entity loop in the QMetaObject lambda (lines 468–481):

```cpp
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            for(const auto& entity : entities) {
                if(action == Core::PickAction::Add) {
                    sel.addSelection(entity);
                } else {
                    sel.removeSelection(entity);
                }
            }
        },
        Qt::QueuedConnection);
```

with a single batch call:

```cpp
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            if(action == Core::PickAction::Add) {
                sel.addSelections(entities);
            } else {
                sel.removeSelections(entities);
            }
        },
        Qt::QueuedConnection);
```

- [ ] **Step 2: Update dispatchPickAreaResults**

Replace the per-entity loop in the QMetaObject lambda (lines 526–541):

```cpp
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            for(const auto& entity : entities) {
                if(action == Core::PickAction::Add) {
                    sel.addSelection(entity);
                } else {
                    sel.removeSelection(entity);
                }
            }
        },
        Qt::QueuedConnection);
```

with a single batch call (same pattern as Step 1):

```cpp
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            if(action == Core::PickAction::Add) {
                sel.addSelections(entities);
            } else {
                sel.removeSelections(entities);
            }
        },
        Qt::QueuedConnection);
```

- [ ] **Step 3: Build the app**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Build succeeds.

- [ ] **Step 4: Run clang-format and commit**

```powershell
clang-format -i src/app/src/gl_viewport_renderer.cpp
git add src/app/src/gl_viewport_renderer.cpp
git commit -m "feat(render): use batch selection API in viewport dispatch

Replace per-entity addSelection/removeSelection loops with single
addSelections/removeSelections calls in dispatchBoxSelectResults and
dispatchPickAreaResults. Eliminates 831 individual lock acquisitions
during box-select.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 5: SceneGraph Deferred Update Mechanism

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- Modify: `src/libs/scene/src/scene_graph.cpp`

- [ ] **Step 1: Add deferred processor interface to SceneGraph header**

In `scene_graph.hpp`, add to the public section (after `writeLock()` at line 191, before `version()`):

```cpp
    /**
     * @brief Register a callback invoked by processDeferredUpdates().
     *
     * Used by bridges (e.g. GeometrySceneBridge) to defer SceneGraph mutations
     * from the worker thread to the render thread.
     *
     * @param processor Callback invoked during processDeferredUpdates()
     */
    void registerDeferredProcessor(std::function<void()> processor);

    /**
     * @brief Execute all registered deferred processors.
     *
     * Called by the render thread during synchronize(), BEFORE acquiring
     * the read lock. Processors may call addNode/configureNode/removeNode
     * which acquire write locks internally.
     */
    void processDeferredUpdates();
```

Add private member (after `m_mutex` at line 223):

```cpp
    std::vector<std::function<void()>> m_deferredProcessors;
```

- [ ] **Step 2: Implement in scene_graph.cpp**

Add at the end of the file, before the closing namespace brace:

```cpp
void SceneGraph::registerDeferredProcessor(std::function<void()> processor) {
    m_deferredProcessors.push_back(std::move(processor));
}

void SceneGraph::processDeferredUpdates() {
    for(const auto& processor : m_deferredProcessors) {
        processor();
    }
}
```

- [ ] **Step 3: Build and run scene tests (no behavior change yet)**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All tests pass (no behavior change, just new API).

- [ ] **Step 4: Run clang-format and commit**

```powershell
clang-format -i src/libs/scene/include/opengeolab/scene/scene_graph.hpp src/libs/scene/src/scene_graph.cpp
git add src/libs/scene/include/opengeolab/scene/scene_graph.hpp src/libs/scene/src/scene_graph.cpp
git commit -m "feat(scene): add deferred-update processor registry to SceneGraph

SceneGraph::registerDeferredProcessor() + processDeferredUpdates()
allow bridges to defer SceneGraph mutations to the render thread.
Called during synchronize() before readLock, keeping the renderer
decoupled from specific bridge implementations.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 6: GeometrySceneBridge Async Refactor

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/geometry_scene_bridge.hpp`
- Modify: `src/libs/scene/src/geometry_scene_bridge.cpp`

- [ ] **Step 1: Add PendingShapeOp and queue to the header**

In `geometry_scene_bridge.hpp`, add `<mutex>` include after the existing includes:

```cpp
#include <mutex>
```

Add the following **before** the class declaration (after the `ShapeEntry` forward-decl block, around line 27):

```cpp
/**
 * @brief Action type for deferred scene bridge operations.
 */
enum class PendingAction : uint8_t { Add, Update, Remove, Clear };

/**
 * @brief Lightweight data needed to apply a deferred ShapeStore change.
 *
 * Stores the fields required by buildRenderData() and node creation/update.
 * Avoids copying heavy OCC topology maps — only visualization data is captured.
 */
struct PendingShapeOp {
    PendingAction action{PendingAction::Add};
    uint32_t shapeId{0};
    std::string name;
    std::shared_ptr<Core::VisualData> visualData;
    std::vector<Core::EntityTag> triangleTags;
    std::vector<Core::EntityTag> edgeTags;
    std::vector<Core::EntityTag> vertexTags;
};
```

Add a public method after the destructor (after line 51):

```cpp
    /**
     * @brief Drain the pending-ops queue and apply deferred scene mutations.
     *
     * Called by the render thread (via SceneGraph::processDeferredUpdates())
     * during synchronize(), while the main thread is paused.
     */
    void processPendingUpdates();
```

Add private members — replace the existing private section starting at line 62 with:

```cpp
private:
    void onShapeAdded(uint32_t shape_id, const Geometry::ShapeEntry& entry);
    void onShapeRemoved(uint32_t shape_id);
    void onShapeUpdated(uint32_t shape_id, const Geometry::ShapeEntry& entry);
    void onStoreCleared();

    void processShapeAdd(PendingShapeOp& op);
    void processShapeUpdate(PendingShapeOp& op);
    void processShapeRemove(uint32_t shape_id);
    void processStoreClear();

    SceneGraph& m_scene;
    Geometry::ShapeStore& m_store;

    /** @brief Maps shapeId → NodeId for quick lookup on remove/update. */
    std::unordered_map<uint32_t, NodeId> m_shapeToNode;

    /** @brief Signal connections for cleanup on destruction. */
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;

    /** @brief Pending operations queued by worker thread, drained on render thread. */
    std::mutex m_pendingMutex;
    std::vector<PendingShapeOp> m_pendingOps;
```

Also update the includes to include `core/visual_data.hpp` and `core/entity_tag.hpp` for the PendingShapeOp fields. Add after the existing includes if not already present:

```cpp
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>
```

- [ ] **Step 2: Refactor signal handlers to enqueue**

In `geometry_scene_bridge.cpp`, replace the constructor to register with SceneGraph:

Replace the constructor body (lines 297–309):

```cpp
GeometrySceneBridge::GeometrySceneBridge(SceneGraph& scene, Geometry::ShapeStore& store)
    : m_scene(scene), m_store(store) {
    m_connections.push_back(
        store.shapeAdded.connect([this](uint32_t shape_id, const Geometry::ShapeEntry& entry) {
            onShapeAdded(shape_id, entry);
        }));
    m_connections.push_back(
        store.shapeRemoved.connect([this](uint32_t shape_id) { onShapeRemoved(shape_id); }));
    m_connections.push_back(
        store.shapeUpdated.connect([this](uint32_t shape_id, const Geometry::ShapeEntry& entry) {
            onShapeUpdated(shape_id, entry);
        }));
    m_connections.push_back(store.storeCleared.connect([this]() { onStoreCleared(); }));
}
```

with:

```cpp
GeometrySceneBridge::GeometrySceneBridge(SceneGraph& scene, Geometry::ShapeStore& store)
    : m_scene(scene), m_store(store) {
    m_connections.push_back(
        store.shapeAdded.connect([this](uint32_t shape_id, const Geometry::ShapeEntry& entry) {
            onShapeAdded(shape_id, entry);
        }));
    m_connections.push_back(
        store.shapeRemoved.connect([this](uint32_t shape_id) { onShapeRemoved(shape_id); }));
    m_connections.push_back(
        store.shapeUpdated.connect([this](uint32_t shape_id, const Geometry::ShapeEntry& entry) {
            onShapeUpdated(shape_id, entry);
        }));
    m_connections.push_back(store.storeCleared.connect([this]() { onStoreCleared(); }));

    m_scene.registerDeferredProcessor([this]() { processPendingUpdates(); });
}
```

- [ ] **Step 3: Rewrite signal handlers to enqueue**

Replace `onShapeAdded` (lines 343–357):

```cpp
void GeometrySceneBridge::onShapeAdded(uint32_t shape_id, const Geometry::ShapeEntry& entry) {
    m_scene.topologyIndex().buildForShape(shape_id, entry);
    std::lock_guard lock(m_pendingMutex);
    m_pendingOps.push_back({
        .action = PendingAction::Add,
        .shapeId = shape_id,
        .name = entry.name,
        .visualData = entry.visualData,
        .triangleTags = entry.triangleTags,
        .edgeTags = entry.edgeTags,
        .vertexTags = entry.vertexTags,
    });
}
```

Replace `onShapeRemoved` (lines 359–366):

```cpp
void GeometrySceneBridge::onShapeRemoved(uint32_t shape_id) {
    {
        std::lock_guard lock(m_pendingMutex);
        m_pendingOps.push_back({.action = PendingAction::Remove, .shapeId = shape_id});
    }
    m_scene.topologyIndex().removeShape(shape_id);
}
```

Replace `onStoreCleared` (line 368):

```cpp
void GeometrySceneBridge::onStoreCleared() {
    std::lock_guard lock(m_pendingMutex);
    m_pendingOps.clear();
    m_pendingOps.push_back({.action = PendingAction::Clear});
}
```

Replace `onShapeUpdated` (lines 370–426):

```cpp
void GeometrySceneBridge::onShapeUpdated(uint32_t shape_id, const Geometry::ShapeEntry& entry) {
    m_scene.topologyIndex().buildForShape(shape_id, entry);
    std::lock_guard lock(m_pendingMutex);
    m_pendingOps.push_back({
        .action = PendingAction::Update,
        .shapeId = shape_id,
        .name = entry.name,
        .visualData = entry.visualData,
        .triangleTags = entry.triangleTags,
        .edgeTags = entry.edgeTags,
        .vertexTags = entry.vertexTags,
    });
}
```

- [ ] **Step 4: Add processPendingUpdates and process methods**

Add `processPendingUpdates()` implementation:

```cpp
void GeometrySceneBridge::processPendingUpdates() {
    std::vector<PendingShapeOp> ops;
    {
        std::lock_guard lock(m_pendingMutex);
        ops.swap(m_pendingOps);
    }

    for(auto& op : ops) {
        switch(op.action) {
        case PendingAction::Add: processShapeAdd(op); break;
        case PendingAction::Update: processShapeUpdate(op); break;
        case PendingAction::Remove: processShapeRemove(op.shapeId); break;
        case PendingAction::Clear: processStoreClear(); break;
        }
    }
}
```

Add `processShapeAdd()` — the original `onShapeAdded` body minus topologyIndex:

```cpp
void GeometrySceneBridge::processShapeAdd(PendingShapeOp& op) {
    if(op.visualData == nullptr || m_shapeToNode.contains(op.shapeId)) {
        return;
    }

    const NodeId node_id = m_scene.addNode(op.name);
    if(node_id == 0) {
        return;
    }

    m_shapeToNode[op.shapeId] = node_id;
    m_scene.setNodeSource(node_id, "geometry", op.shapeId);

    // Build a temporary ShapeEntry for buildRenderData (only vis fields used).
    Geometry::ShapeEntry temp_entry;
    temp_entry.id = op.shapeId;
    temp_entry.name = std::move(op.name);
    temp_entry.visualData = std::move(op.visualData);
    temp_entry.triangleTags = std::move(op.triangleTags);
    temp_entry.edgeTags = std::move(op.edgeTags);
    temp_entry.vertexTags = std::move(op.vertexTags);

    RenderMeshData mesh_data = buildRenderData(op.shapeId, temp_entry);
    m_scene.configureNode(node_id, [&](SceneNode& node) {
        node.setLocalBounds(mesh_data.bounds);
        auto render_component = std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
        ShapeRenderComponent const* render_component_ptr = render_component.get();
        node.setRenderComponent(std::move(render_component));
        node.setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
    });
}
```

> **Note:** `ShapeRenderComponent` and `ShapePickComponent` are defined in the anonymous namespace at the top of `geometry_scene_bridge.cpp`. The `attachComponents` free function can be kept or inlined — the above inlines it for clarity.

Add `processShapeUpdate()` — the original `onShapeUpdated` body minus topologyIndex:

```cpp
void GeometrySceneBridge::processShapeUpdate(PendingShapeOp& op) {
    NodeId node_id = 0;
    if(const auto iterator = m_shapeToNode.find(op.shapeId); iterator != m_shapeToNode.end()) {
        if(m_scene.findNode(iterator->second) != nullptr) {
            node_id = iterator->second;
        } else {
            m_shapeToNode.erase(iterator);
        }
    }

    // Build temp entry for buildRenderData.
    Geometry::ShapeEntry temp_entry;
    temp_entry.id = op.shapeId;
    temp_entry.name = std::move(op.name);
    temp_entry.visualData = std::move(op.visualData);
    temp_entry.triangleTags = std::move(op.triangleTags);
    temp_entry.edgeTags = std::move(op.edgeTags);
    temp_entry.vertexTags = std::move(op.vertexTags);

    if(node_id == 0) {
        if(temp_entry.visualData == nullptr) {
            return;
        }

        node_id = m_scene.addNode(temp_entry.name);
        if(node_id == 0) {
            return;
        }
        m_shapeToNode[op.shapeId] = node_id;
        m_scene.setNodeSource(node_id, "geometry", op.shapeId);

        RenderMeshData mesh_data = buildRenderData(op.shapeId, temp_entry);
        m_scene.configureNode(node_id, [&](SceneNode& node) {
            node.setLocalBounds(mesh_data.bounds);
            auto render_component = std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
            ShapeRenderComponent const* render_component_ptr = render_component.get();
            node.setRenderComponent(std::move(render_component));
            node.setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
        });
        return;
    }

    if(temp_entry.visualData == nullptr) {
        m_scene.configureNode(node_id,
                              [&](SceneNode& node) { node.setName(temp_entry.name); });
        return;
    }

    RenderMeshData mesh_data = buildRenderData(op.shapeId, temp_entry);
    m_scene.configureNode(node_id, [&](SceneNode& node) {
        node.setName(temp_entry.name);
        node.setLocalBounds(mesh_data.bounds);

        if(auto* render_component = dynamic_cast<ShapeRenderComponent*>(node.renderComponent());
           render_component != nullptr) {
            render_component->updateData(std::move(mesh_data));
        } else {
            auto new_render_component =
                std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
            ShapeRenderComponent const* render_component_ptr = new_render_component.get();
            node.setPickComponent(nullptr);
            node.setRenderComponent(std::move(new_render_component));
            node.setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
        }

        if(node.pickComponent() == nullptr) {
            if(auto* render_component =
                   dynamic_cast<ShapeRenderComponent*>(node.renderComponent());
               render_component != nullptr) {
                node.setPickComponent(std::make_unique<ShapePickComponent>(render_component));
            }
        }
    });
}
```

Add `processShapeRemove()`:

```cpp
void GeometrySceneBridge::processShapeRemove(uint32_t shape_id) {
    if(const auto iterator = m_shapeToNode.find(shape_id); iterator != m_shapeToNode.end()) {
        m_scene.removeNode(iterator->second);
        m_shapeToNode.erase(iterator);
    }
}
```

Add `processStoreClear()`:

```cpp
void GeometrySceneBridge::processStoreClear() { m_shapeToNode.clear(); }
```

- [ ] **Step 5: Build scene library**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```

Expected: Build succeeds. Tests will need update (next task).

- [ ] **Step 6: Run clang-format**

```powershell
clang-format -i src/libs/scene/include/opengeolab/scene/geometry_scene_bridge.hpp src/libs/scene/src/geometry_scene_bridge.cpp
```

---

## Task 7: Update GeometrySceneBridge Tests

**Files:**
- Modify: `src/libs/scene/test/geometry_scene_bridge_test.cpp`

After the async refactor, store operations queue pending ops instead of immediately
updating the scene. Tests must call `fixture.scene.processDeferredUpdates()` to flush.

- [ ] **Step 1: Update "creates renderable node after tessellation" test**

In the test at line 36, add the flush call after `tessellate`:

```cpp
TEST_CASE("GeometrySceneBridge creates renderable node after tessellation") {
    BridgeFixture fixture;
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = fixture.store.add("TestBox", boxMaker.Shape());
    fixture.store.tessellate(shapeId);
    fixture.scene.processDeferredUpdates();

    SceneNode* node = firstChild(fixture.scene);
    REQUIRE(node != nullptr);
    CHECK(std::string{node->name()} == "TestBox");
    REQUIRE(node->renderComponent() != nullptr);
    REQUIRE(node->pickComponent() != nullptr);

    const RenderMeshData& meshData = node->renderComponent()->meshData();
    CHECK_FALSE(meshData.vertices.empty());
    CHECK(meshData.pickIds.size() == meshData.vertices.size());
    CHECK_FALSE(meshData.triangleRanges.empty());
    CHECK_FALSE(meshData.lineRanges.empty());
    CHECK(meshData.bounds.isValid());
    CHECK(fixture.scene.topologyIndex().edgeToWire(shapeId, 1).has_value());
    CHECK(node->sourceType() == "geometry");
    CHECK(node->sourceId() == shapeId);
}
```

- [ ] **Step 2: Update "removes scene node and topology data" test**

```cpp
TEST_CASE("GeometrySceneBridge removes scene node and topology data") {
    BridgeFixture fixture;
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = fixture.store.add("TestBox", boxMaker.Shape());
    fixture.store.tessellate(shapeId);
    fixture.scene.processDeferredUpdates();
    REQUIRE(firstChild(fixture.scene) != nullptr);
    REQUIRE(fixture.scene.topologyIndex().edgeToWire(shapeId, 1).has_value());

    fixture.store.remove(shapeId);
    fixture.scene.processDeferredUpdates();

    CHECK(firstChild(fixture.scene) == nullptr);
    CHECK_FALSE(fixture.scene.topologyIndex().edgeToWire(shapeId, 1).has_value());
}
```

- [ ] **Step 3: Update "refreshes mesh data on retessellation" test**

```cpp
TEST_CASE("GeometrySceneBridge refreshes mesh data on retessellation") {
    BridgeFixture fixture;
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = fixture.store.add("TestBox", boxMaker.Shape());
    fixture.store.tessellate(shapeId);
    fixture.scene.processDeferredUpdates();

    SceneNode* node = firstChild(fixture.scene);
    REQUIRE(node != nullptr);
    REQUIRE(node->renderComponent() != nullptr);

    const uint64_t originalVersion = node->renderComponent()->dataVersion();
    const std::size_t originalVertexCount = node->renderComponent()->meshData().vertices.size();

    fixture.store.tessellate(shapeId, Geometry::TessellationParams{0.05, 0.25});
    fixture.scene.processDeferredUpdates();

    node = firstChild(fixture.scene);
    REQUIRE(node != nullptr);
    REQUIRE(node->renderComponent() != nullptr);
    CHECK(node->renderComponent()->dataVersion() > originalVersion);
    CHECK(node->renderComponent()->meshData().vertices.size() >= originalVertexCount);
    CHECK(node->renderComponent()->meshData().bounds.isValid());
}
```

- [ ] **Step 4: Update "buildRenderData keeps pickIds aligned" test**

```cpp
TEST_CASE("GeometrySceneBridge buildRenderData keeps pickIds aligned with vertices") {
    Geometry::ShapeStore store;
    SceneGraph scene;
    GeometrySceneBridge bridge(scene, store);
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = store.add("TestBox", boxMaker.Shape());
    store.tessellate(shapeId);
    const Geometry::ShapeEntry* entry = store.find(shapeId);
    REQUIRE(entry != nullptr);

    const RenderMeshData data = GeometrySceneBridge::buildRenderData(shapeId, *entry);

    CHECK(data.pickIds.size() == data.vertices.size());
    CHECK(data.bounds.isValid());
}
```

> **Note:** This test only exercises the static `buildRenderData` method. No `processDeferredUpdates()` needed.

- [ ] **Step 5: Update "destructor disconnects from ShapeStore" test**

```cpp
TEST_CASE("GeometrySceneBridge destructor disconnects from ShapeStore") {
    Geometry::ShapeStore store;
    SceneGraph scene;

    {
        GeometrySceneBridge bridge(scene, store);
    }

    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);
    const uint32_t shapeId = store.add("DetachedBox", boxMaker.Shape());
    store.tessellate(shapeId);
    scene.processDeferredUpdates();

    CHECK(firstChild(scene) == nullptr);
    CHECK_FALSE(scene.topologyIndex().edgeToWire(shapeId, 1).has_value());
}
```

> **Note:** After bridge destruction, the deferred processor registered by the bridge still exists in `m_deferredProcessors` but the lambda captures `this` which is now dangling. The bridge must deregister on destruction. **Important fix needed in Task 6 Step 2** — see note below.

**IMPORTANT**: The current `registerDeferredProcessor` has no deregistration mechanism. When the bridge is destroyed, its lambda becomes dangling. Two options:
1. Add `unregisterDeferredProcessor` with an ID/token system.
2. Have the bridge use a shared flag to skip processing after destruction.

**Simplest fix**: Use a `std::shared_ptr<bool>` alive flag:

In `geometry_scene_bridge.hpp` private section, add:

```cpp
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
```

In the constructor, change the registration to:

```cpp
    auto alive = m_alive;
    m_scene.registerDeferredProcessor([this, alive]() {
        if(*alive) {
            processPendingUpdates();
        }
    });
```

In the destructor implementation, change from defaulted to:

```cpp
GeometrySceneBridge::~GeometrySceneBridge() { *m_alive = false; }
```

This ensures `processDeferredUpdates()` safely skips the destroyed bridge's callback.

- [ ] **Step 6: Build and run scene tests**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All scene tests pass.

- [ ] **Step 7: Run clang-format and commit**

```powershell
clang-format -i src/libs/scene/include/opengeolab/scene/geometry_scene_bridge.hpp src/libs/scene/src/geometry_scene_bridge.cpp src/libs/scene/test/geometry_scene_bridge_test.cpp
git add src/libs/scene/include/opengeolab/scene/geometry_scene_bridge.hpp src/libs/scene/src/geometry_scene_bridge.cpp src/libs/scene/test/geometry_scene_bridge_test.cpp
git commit -m "refactor(scene): defer GeometrySceneBridge mutations to render thread

ShapeStore signal handlers now enqueue lightweight PendingShapeOp
structs instead of mutating SceneGraph directly. processPendingUpdates()
drains the queue on the render thread during synchronize(), eliminating
worker-thread write-lock contention that blocked the render pipeline.

TopologyIndex building stays on the worker thread (no lock needed).
Alive-flag guards deferred callback after bridge destruction.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 8: GLViewportRenderer Deferred Integration

**Files:**
- Modify: `src/app/src/gl_viewport_renderer.cpp:99-102`

- [ ] **Step 1: Call processDeferredUpdates before readLock**

Replace lines 99–102 in `synchronize()`:

```cpp
    if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
        [[maybe_unused]] auto lock = scene->readLock();
        m_pipeline.synchronize(*scene);
    }
```

with:

```cpp
    if(auto* scene = viewport->sceneGraph(); scene != nullptr) {
        scene->processDeferredUpdates();
        [[maybe_unused]] auto lock = scene->readLock();
        m_pipeline.synchronize(*scene);
    }
```

Key changes:
- `const auto*` → `auto*` (non-const, because `processDeferredUpdates()` is non-const)
- Call `processDeferredUpdates()` before acquiring the read lock

- [ ] **Step 2: Build the full app**

Run:
```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Build succeeds.

- [ ] **Step 3: Run all tests**

Run:
```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 4: Run clang-format and commit**

```powershell
clang-format -i src/app/src/gl_viewport_renderer.cpp
git add src/app/src/gl_viewport_renderer.cpp
git commit -m "feat(render): call processDeferredUpdates in synchronize

GLViewportRenderer now flushes deferred scene bridge mutations before
acquiring the readLock in synchronize(). Deferred updates execute on
the render thread while the main thread is paused (Qt Quick model),
so no lock contention occurs.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 9: Final Verification

- [ ] **Step 1: Full build**

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] **Step 2: Full test suite**

```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

- [ ] **Step 3: clang-tidy on changed files**

```powershell
clang-tidy --config-file=.clang-tidy -p build src/libs/scene/include/opengeolab/scene/selection_state.hpp src/libs/scene/src/selection_state.cpp src/libs/scene/include/opengeolab/scene/geometry_scene_bridge.hpp src/libs/scene/src/geometry_scene_bridge.cpp src/libs/scene/include/opengeolab/scene/scene_graph.hpp src/libs/scene/src/scene_graph.cpp src/app/src/gl_viewport_renderer.cpp src/app/src/selection_service.cpp src/libs/scene/src/scene_module.cpp
```

Fix any issues and re-commit if needed.

- [ ] **Step 4: Manual test**

```
1. Start app: build\src\app\RelWithDebInfo\opengeolab_app.exe --start-http-server
2. Import BREP model via HTTP
3. Box-select many faces → verify immediate highlight (no lag)
4. Delete selected faces → verify UI stays responsive during operation
5. Verify faces disappear after operation completes
```
