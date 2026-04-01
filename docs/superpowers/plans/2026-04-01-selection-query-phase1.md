# Selection & Geometry Query — Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement entity-level selection infrastructure with GPU picking, highlight rendering, and a Geometry Query QML panel — Phase 1 (no MSDF labels).

**Architecture:** SelectionState lives as a SceneGraph member. Pick results dispatch from render thread to main thread via QMetaObject::invokeMethod. FrameState carries pre-resolved DrawRanges from SelectionState during synchronize(). GpuBufferManager builds an EntityRef→DrawRange index for O(1) lookups. GeoQueryPage QML panel orchestrates pick mode activation and displays selected entities as chips.

**Tech Stack:** C++20, OpenGL 3.3, Qt 6.9 / QML, doctest, CMake + Ninja

**Spec:** `docs/superpowers/specs/2026-03-31-selection-query-design.md`

**Build/Test commands:**
```
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

---

## File Structure

### Core layer — new files
| File | Responsibility |
|------|---------------|
| `src/libs/core/include/opengeolab/core/entity_ref.hpp` | EntityRef struct — scene-wide absolute entity address |
| `src/libs/core/include/opengeolab/core/pick_action.hpp` | PickAction enum (Add/Remove) |

### Core layer — moved files
| From | To | Responsibility |
|------|-----|---------------|
| `src/libs/render/include/opengeolab/render/pick_mask.hpp` | `src/libs/core/include/opengeolab/core/pick_mask.hpp` | PickMask + PickMode enums |

### Scene layer — new files
| File | Responsibility |
|------|---------------|
| `src/libs/scene/include/opengeolab/scene/selection_state.hpp` | SelectionState class header |
| `src/libs/scene/src/selection_state.cpp` | SelectionState implementation |
| `src/libs/scene/include/opengeolab/scene/label_manager.hpp` | LabelManager class header (data-only, Phase 2 rendering) |
| `src/libs/scene/src/label_manager.cpp` | LabelManager implementation |
| `src/libs/scene/include/opengeolab/scene/select_action.hpp` | SelectAction header |
| `src/libs/scene/src/select_action.cpp` | SelectAction implementation |
| `src/libs/scene/include/opengeolab/scene/deselect_action.hpp` | DeselectAction header |
| `src/libs/scene/src/deselect_action.cpp` | DeselectAction implementation |
| `src/libs/scene/include/opengeolab/scene/clear_selection_action.hpp` | ClearSelectionAction header |
| `src/libs/scene/src/clear_selection_action.cpp` | ClearSelectionAction implementation |
| `src/libs/scene/include/opengeolab/scene/query_selection_action.hpp` | QuerySelectionAction header |
| `src/libs/scene/src/query_selection_action.cpp` | QuerySelectionAction implementation |
| `src/libs/scene/include/opengeolab/scene/set_pick_mode_action.hpp` | SetPickModeAction header |
| `src/libs/scene/src/set_pick_mode_action.cpp` | SetPickModeAction implementation |
| `src/libs/scene/include/opengeolab/scene/set_hover_action.hpp` | SetHoverAction header |
| `src/libs/scene/src/set_hover_action.cpp` | SetHoverAction implementation |
| `src/libs/scene/test/selection_state_test.cpp` | SelectionState unit tests |
| `src/libs/scene/test/selection_actions_test.cpp` | Selection actions unit tests |
| `src/libs/scene/test/label_manager_test.cpp` | LabelManager unit tests |

### Render layer — modified files
| File | Change |
|------|--------|
| `src/libs/render/include/opengeolab/render/pick_mask.hpp` | Remove — moved to core |
| `src/libs/render/include/opengeolab/render/render_pipeline.hpp` | Add `pickRect()` method |
| `src/libs/render/src/render_pipeline.cpp` | Implement `pickRect()` |
| `src/libs/render/src/core/pick_fbo.hpp` | Add `readPickRect()` method |
| `src/libs/render/src/core/pick_fbo.cpp` | Implement `readPickRect()` |
| `src/libs/render/src/core/gpu_buffer_manager.hpp` | Add EntityRef→DrawRange index |
| `src/libs/render/src/core/gpu_buffer_manager.cpp` | Build index during rebuildBuffers |
| `src/libs/render/CMakeLists.txt` | Remove pick_mask.hpp from headers |

### App layer — new files
| File | Responsibility |
|------|---------------|
| `src/app/include/opengeolab/app/selection_service.hpp` | SelectionService QML singleton header |
| `src/app/src/selection_service.cpp` | SelectionService implementation |
| `src/app/resource/qml/components/pages/GeoQueryPage.qml` | Geometry Query panel |
| `src/app/resource/qml/components/EntityTypeSelector.qml` | Pick type toggle buttons |
| `src/app/resource/qml/components/EntityChip.qml` | Removable entity chip |

### App layer — modified files
| File | Change |
|------|--------|
| `src/app/include/opengeolab/app/gl_viewport.hpp` | Add PendingPick.action, PendingBoxSelect, box-select state |
| `src/app/src/gl_viewport.cpp` | Mouse event changes for select/deselect/box-select |
| `src/app/include/opengeolab/app/gl_viewport_renderer.hpp` | Handle PickAction, PendingBoxSelect, SelectionState resolution |
| `src/app/src/gl_viewport_renderer.cpp` | Dispatch pick with action, populate FrameState from SelectionState |
| `src/app/CMakeLists.txt` | Add SelectionService, new QML files |
| `src/app/resource/qml/MainPages.qml` | Register GeoQueryPage |
| `src/app/resource/qml/RibbonConfig.qml` | Add query action to ribbon |

---

## Task 1: EntityRef and PickAction in Core

**Files:**
- Create: `src/libs/core/include/opengeolab/core/entity_ref.hpp`
- Create: `src/libs/core/include/opengeolab/core/pick_action.hpp`
- Modify: `src/libs/core/CMakeLists.txt`
- Create: `src/libs/core/test/entity_ref_test.cpp`

- [ ] **Step 1: Create EntityRef header**

```cpp
// src/libs/core/include/opengeolab/core/entity_ref.hpp
/**
 * @file entity_ref.hpp
 * @brief EntityRef — scene-wide absolute entity address
 *
 * Combines a top-level shapeId with an EntityTag to form a globally
 * unique entity address usable across all layers.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <cstdint>
#include <functional>

namespace OpenGeoLab::Core {

/**
 * @brief Scene-wide absolute entity address.
 *
 * Extends EntityTag (shape-scoped) with a shapeId to create a globally
 * unique identifier. Zero-cost conversion to/from PickId encoding.
 */
struct EntityRef {
    uint32_t shapeId{};    ///< Owning shape/part ID
    EntityType entityType{}; ///< Entity classification
    uint32_t localId{};    ///< Type-scoped local ID within the shape

    /// True when this ref points to a real entity.
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return shapeId != 0 && localId != 0;
    }

    /// True for geometry entities (GeoVertex..GeoSolid).
    [[nodiscard]] constexpr bool isGeometry() const noexcept {
        return entityType >= EntityType::GeoVertex && entityType <= EntityType::GeoSolid;
    }

    /// True for mesh entities (MeshNode..MeshElement).
    [[nodiscard]] constexpr bool isMesh() const noexcept {
        return entityType >= EntityType::MeshNode && entityType <= EntityType::MeshElement;
    }

    /// Extract the shape-scoped EntityTag.
    [[nodiscard]] constexpr EntityTag tag() const noexcept { return {entityType, localId}; }

    bool operator==(const EntityRef&) const = default;
    auto operator<=>(const EntityRef&) const = default;
};

} // namespace OpenGeoLab::Core

/// Hash specialization for use in unordered containers.
template<>
struct std::hash<OpenGeoLab::Core::EntityRef> {
    std::size_t operator()(const OpenGeoLab::Core::EntityRef& ref) const noexcept {
        auto h1 = std::hash<uint32_t>{}(ref.shapeId);
        auto h2 = std::hash<uint8_t>{}(static_cast<uint8_t>(ref.entityType));
        auto h3 = std::hash<uint32_t>{}(ref.localId);
        h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        return h1;
    }
};
```

- [ ] **Step 2: Create PickAction header**

```cpp
// src/libs/core/include/opengeolab/core/pick_action.hpp
/**
 * @file pick_action.hpp
 * @brief PickAction — intent of a user pick gesture
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

/// Describes whether a pick gesture adds or removes from selection.
enum class PickAction : uint8_t {
    Add,    ///< Left-click: add entity to selection
    Remove, ///< Right-click: remove entity from selection
};

} // namespace OpenGeoLab::Core
```

- [ ] **Step 3: Write EntityRef tests**

```cpp
// src/libs/core/test/entity_ref_test.cpp
/**
 * @file entity_ref_test.cpp
 * @brief Unit tests for EntityRef
 */

#include <opengeolab/core/entity_ref.hpp>

#include <doctest/doctest.h>

#include <unordered_set>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;

TEST_SUITE("EntityRef") {
    TEST_CASE("default is invalid") {
        EntityRef ref;
        CHECK_FALSE(ref.isValid());
    }

    TEST_CASE("valid geometry ref") {
        EntityRef ref{1, EntityType::GeoFace, 3};
        CHECK(ref.isValid());
        CHECK(ref.isGeometry());
        CHECK_FALSE(ref.isMesh());
    }

    TEST_CASE("valid mesh ref") {
        EntityRef ref{2, EntityType::MeshNode, 5};
        CHECK(ref.isValid());
        CHECK_FALSE(ref.isGeometry());
        CHECK(ref.isMesh());
    }

    TEST_CASE("equality") {
        EntityRef a{1, EntityType::GeoEdge, 2};
        EntityRef b{1, EntityType::GeoEdge, 2};
        EntityRef c{1, EntityType::GeoEdge, 3};
        CHECK(a == b);
        CHECK(a != c);
    }

    TEST_CASE("ordering") {
        EntityRef a{1, EntityType::GeoVertex, 1};
        EntityRef b{1, EntityType::GeoEdge, 1};
        CHECK(a < b);
    }

    TEST_CASE("tag extraction") {
        EntityRef ref{5, EntityType::GeoFace, 7};
        auto tag = ref.tag();
        CHECK(tag.type == EntityType::GeoFace);
        CHECK(tag.localId == 7);
    }

    TEST_CASE("usable in unordered_set") {
        std::unordered_set<EntityRef> set;
        set.insert({1, EntityType::GeoFace, 3});
        set.insert({1, EntityType::GeoFace, 3});
        set.insert({2, EntityType::GeoEdge, 1});
        CHECK(set.size() == 2);
    }
}
```

- [ ] **Step 4: Update core CMakeLists.txt**

Add to `core_public_headers`:
```cmake
include/opengeolab/core/entity_ref.hpp
include/opengeolab/core/pick_action.hpp
```

Add test:
```cmake
opengeolab_add_doctest_test(
    opengeolab_entity_ref_test SOURCES test/entity_ref_test.cpp LINKS
    OpenGeoLab::Core)
```

- [ ] **Step 5: Build and run tests**

```
cmake --build build --config RelWithDebInfo --target opengeolab_entity_ref_test --parallel 4
ctest --test-dir build -C RelWithDebInfo -R entity_ref --output-on-failure
```
Expected: All tests pass.

- [ ] **Step 6: Commit**

```
git add src/libs/core/
git commit -m "feat(core): add EntityRef and PickAction types

EntityRef is a scene-wide absolute entity address combining shapeId +
EntityType + localId. PickAction distinguishes add vs remove pick intent.
Both are used by the selection infrastructure across all layers."
```

---

## Task 2: Move PickMask from Render to Core

**Files:**
- Move: `src/libs/render/include/opengeolab/render/pick_mask.hpp` → `src/libs/core/include/opengeolab/core/pick_mask.hpp`
- Modify: `src/libs/core/CMakeLists.txt`
- Modify: `src/libs/render/CMakeLists.txt`
- Modify: All files that `#include <opengeolab/render/pick_mask.hpp>`

- [ ] **Step 1: Create pick_mask.hpp in core**

Copy `src/libs/render/include/opengeolab/render/pick_mask.hpp` to `src/libs/core/include/opengeolab/core/pick_mask.hpp`. Change namespace from `OpenGeoLab::Render` to `OpenGeoLab::Core`.

```cpp
// src/libs/core/include/opengeolab/core/pick_mask.hpp
/**
 * @file pick_mask.hpp
 * @brief PickMode and PickMask enumerations for GPU pick filtering
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

enum class PickMode : uint8_t {
    VEF,   /**< Vertex > Edge > Face priority */
    Wire,  /**< Edge → resolve to Wire */
    Solid, /**< Face → resolve to Solid */
    Part,  /**< Any → resolve to Part (shapeId) */
};

enum class PickMask : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Edge = 1 << 1,
    Wire = 1 << 2,
    Face = 1 << 3,
    Solid = 1 << 4,
    Part = 1 << 5,
    All = 0xFFFFFFFF,
};

constexpr PickMask operator|(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr PickMask operator&(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace OpenGeoLab::Core
```

- [ ] **Step 2: Create compatibility header in render**

Replace the old `src/libs/render/include/opengeolab/render/pick_mask.hpp` with a forwarding header so downstream code keeps compiling during transition:

```cpp
// src/libs/render/include/opengeolab/render/pick_mask.hpp
/**
 * @file pick_mask.hpp
 * @brief Compatibility header — PickMask now lives in core
 */

#pragma once

#include <opengeolab/core/pick_mask.hpp>

namespace OpenGeoLab::Render {
using OpenGeoLab::Core::PickMask;
using OpenGeoLab::Core::PickMode;
} // namespace OpenGeoLab::Render
```

- [ ] **Step 3: Update core CMakeLists.txt**

Add `include/opengeolab/core/pick_mask.hpp` to `core_public_headers`.

- [ ] **Step 4: Build full project**

```
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Full build succeeds with no errors. The render compatibility header ensures all existing code compiles unchanged.

- [ ] **Step 5: Commit**

```
git add src/libs/core/ src/libs/render/include/opengeolab/render/pick_mask.hpp
git commit -m "refactor(core): move PickMask and PickMode from render to core

Scene layer needs PickMask but must not depend on render. Old header
retained as a forwarding include with using-declarations for backward
compatibility."
```

---

## Task 3: SelectionState in Scene

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/selection_state.hpp`
- Create: `src/libs/scene/src/selection_state.cpp`
- Modify: `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- Modify: `src/libs/scene/src/scene_graph.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`
- Create: `src/libs/scene/test/selection_state_test.cpp`

- [ ] **Step 1: Create SelectionState header**

```cpp
// src/libs/scene/include/opengeolab/scene/selection_state.hpp
/**
 * @file selection_state.hpp
 * @brief SelectionState — thread-safe entity-level selection manager
 *
 * Manages the set of selected and hovered entities for 3D picking.
 * Coexists with SceneGraph's node-level selection (used for tree-view).
 * Uses version-based dirty tracking so the render thread only re-resolves
 * EntityRef → DrawRange when selection actually changes.
 */

#pragma once

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/pick_mask.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <kangaroo/util/signal.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace OpenGeoLab::Scene {

/**
 * @brief Thread-safe entity-level selection state.
 *
 * Writers (main thread: pick dispatch, Python actions, QML commands)
 * increment version atomically. Readers (render synchronize, QML reads)
 * use the version to detect changes without locking during render.
 */
class OPENGEOLAB_SCENE_EXPORT SelectionState final {
public:
    SelectionState();
    ~SelectionState();

    // ── Pick configuration ──

    void setPickEnabled(bool enabled);
    [[nodiscard]] bool pickEnabled() const;

    void setPickMask(Core::PickMask mask);
    [[nodiscard]] Core::PickMask pickMask() const;

    // ── Selection management ──

    void addSelection(const Core::EntityRef& entity);
    void removeSelection(const Core::EntityRef& entity);
    void clearSelection();
    [[nodiscard]] std::vector<Core::EntityRef> selections() const;
    [[nodiscard]] bool isSelected(const Core::EntityRef& entity) const;

    // ── Hover management ──

    void setHovered(const Core::EntityRef& entity);
    void clearHover();
    [[nodiscard]] std::optional<Core::EntityRef> hovered() const;

    // ── Version tracking (for render dirty-check) ──

    [[nodiscard]] uint64_t selectionVersion() const noexcept;
    [[nodiscard]] uint64_t hoverVersion() const noexcept;

    // ── Signals ──

    Kangaroo::Util::Signal<Core::EntityRef> entitySelected;
    Kangaroo::Util::Signal<Core::EntityRef> entityDeselected;
    Kangaroo::Util::Signal<> selectionCleared;
    Kangaroo::Util::Signal<Core::EntityRef> hoverChanged;
    Kangaroo::Util::Signal<> pickConfigChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Core::EntityRef> m_selections; ///< Sorted for O(log n) lookup
    std::optional<Core::EntityRef> m_hovered;
    Core::PickMask m_pickMask{Core::PickMask::None};
    bool m_pickEnabled{false};
    std::atomic<uint64_t> m_selectionVersion{0};
    std::atomic<uint64_t> m_hoverVersion{0};
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Implement SelectionState**

```cpp
// src/libs/scene/src/selection_state.cpp
/**
 * @file selection_state.cpp
 * @brief SelectionState implementation
 */

#include <opengeolab/scene/selection_state.hpp>

#include <algorithm>

namespace OpenGeoLab::Scene {

SelectionState::SelectionState() = default;
SelectionState::~SelectionState() = default;

void SelectionState::setPickEnabled(bool enabled) {
    {
        std::unique_lock lock(m_mutex);
        if(m_pickEnabled == enabled) {
            return;
        }
        m_pickEnabled = enabled;
    }
    pickConfigChanged.emit();
}

bool SelectionState::pickEnabled() const {
    std::shared_lock lock(m_mutex);
    return m_pickEnabled;
}

void SelectionState::setPickMask(Core::PickMask mask) {
    {
        std::unique_lock lock(m_mutex);
        if(m_pickMask == mask) {
            return;
        }
        m_pickMask = mask;
    }
    pickConfigChanged.emit();
}

Core::PickMask SelectionState::pickMask() const {
    std::shared_lock lock(m_mutex);
    return m_pickMask;
}

void SelectionState::addSelection(const Core::EntityRef& entity) {
    if(!entity.isValid()) {
        return;
    }
    {
        std::unique_lock lock(m_mutex);
        auto it = std::lower_bound(m_selections.begin(), m_selections.end(), entity);
        if(it != m_selections.end() && *it == entity) {
            return; // Already selected
        }
        m_selections.insert(it, entity);
        ++m_selectionVersion;
    }
    entitySelected.emit(entity);
}

void SelectionState::removeSelection(const Core::EntityRef& entity) {
    {
        std::unique_lock lock(m_mutex);
        auto it = std::lower_bound(m_selections.begin(), m_selections.end(), entity);
        if(it == m_selections.end() || *it != entity) {
            return; // Not selected
        }
        m_selections.erase(it);
        ++m_selectionVersion;
    }
    entityDeselected.emit(entity);
}

void SelectionState::clearSelection() {
    {
        std::unique_lock lock(m_mutex);
        if(m_selections.empty()) {
            return;
        }
        m_selections.clear();
        ++m_selectionVersion;
    }
    selectionCleared.emit();
}

std::vector<Core::EntityRef> SelectionState::selections() const {
    std::shared_lock lock(m_mutex);
    return m_selections;
}

bool SelectionState::isSelected(const Core::EntityRef& entity) const {
    std::shared_lock lock(m_mutex);
    return std::binary_search(m_selections.begin(), m_selections.end(), entity);
}

void SelectionState::setHovered(const Core::EntityRef& entity) {
    {
        std::unique_lock lock(m_mutex);
        if(m_hovered.has_value() && *m_hovered == entity) {
            return;
        }
        m_hovered = entity;
        ++m_hoverVersion;
    }
    hoverChanged.emit(entity);
}

void SelectionState::clearHover() {
    {
        std::unique_lock lock(m_mutex);
        if(!m_hovered.has_value()) {
            return;
        }
        m_hovered.reset();
        ++m_hoverVersion;
    }
    hoverChanged.emit(Core::EntityRef{});
}

std::optional<Core::EntityRef> SelectionState::hovered() const {
    std::shared_lock lock(m_mutex);
    return m_hovered;
}

uint64_t SelectionState::selectionVersion() const noexcept {
    return m_selectionVersion.load(std::memory_order_acquire);
}

uint64_t SelectionState::hoverVersion() const noexcept {
    return m_hoverVersion.load(std::memory_order_acquire);
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Add SelectionState to SceneGraph**

In `scene_graph.hpp`, add after existing includes:
```cpp
#include <opengeolab/scene/selection_state.hpp>
```

Add public accessors after `setHoveredNode`:
```cpp
/** @brief Entity-level selection state for 3D picking. */
[[nodiscard]] SelectionState& selectionState() { return m_selectionState; }
[[nodiscard]] const SelectionState& selectionState() const { return m_selectionState; }
```

Add private member after `m_hoveredNode`:
```cpp
SelectionState m_selectionState;
```

- [ ] **Step 4: Write SelectionState tests**

```cpp
// src/libs/scene/test/selection_state_test.cpp
/**
 * @file selection_state_test.cpp
 * @brief Unit tests for SelectionState
 */

#include <opengeolab/scene/selection_state.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Core::PickMask;
using OpenGeoLab::Scene::SelectionState;

namespace {
constexpr EntityRef FACE_1{1, EntityType::GeoFace, 3};
constexpr EntityRef FACE_2{1, EntityType::GeoFace, 5};
constexpr EntityRef EDGE_1{1, EntityType::GeoEdge, 2};
constexpr EntityRef VERTEX_1{1, EntityType::GeoVertex, 1};
} // namespace

TEST_SUITE("SelectionState") {
    TEST_CASE("initially empty") {
        SelectionState state;
        CHECK(state.selections().empty());
        CHECK_FALSE(state.hovered().has_value());
        CHECK(state.selectionVersion() == 0);
        CHECK(state.hoverVersion() == 0);
    }

    TEST_CASE("add and query selection") {
        SelectionState state;
        state.addSelection(FACE_1);

        CHECK(state.isSelected(FACE_1));
        CHECK_FALSE(state.isSelected(FACE_2));
        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("duplicate add is idempotent") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(FACE_1);

        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("remove selection") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(FACE_2);
        state.removeSelection(FACE_1);

        CHECK_FALSE(state.isSelected(FACE_1));
        CHECK(state.isSelected(FACE_2));
        CHECK(state.selectionVersion() == 3);
    }

    TEST_CASE("remove nonexistent is no-op") {
        SelectionState state;
        state.removeSelection(FACE_1);
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("clear selection") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(EDGE_1);
        state.clearSelection();

        CHECK(state.selections().empty());
        CHECK(state.selectionVersion() == 3);
    }

    TEST_CASE("clear empty is no-op") {
        SelectionState state;
        state.clearSelection();
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("invalid entity rejected") {
        SelectionState state;
        state.addSelection(EntityRef{});
        CHECK(state.selections().empty());
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("hover set and clear") {
        SelectionState state;
        state.setHovered(FACE_1);
        CHECK(state.hovered().has_value());
        CHECK(*state.hovered() == FACE_1);
        CHECK(state.hoverVersion() == 1);

        state.clearHover();
        CHECK_FALSE(state.hovered().has_value());
        CHECK(state.hoverVersion() == 2);
    }

    TEST_CASE("hover same entity is idempotent") {
        SelectionState state;
        state.setHovered(FACE_1);
        state.setHovered(FACE_1);
        CHECK(state.hoverVersion() == 1);
    }

    TEST_CASE("pick configuration") {
        SelectionState state;
        CHECK_FALSE(state.pickEnabled());
        CHECK(state.pickMask() == PickMask::None);

        state.setPickEnabled(true);
        state.setPickMask(PickMask::Vertex | PickMask::Edge | PickMask::Face);

        CHECK(state.pickEnabled());
        CHECK((state.pickMask() & PickMask::Vertex) != PickMask::None);
    }

    TEST_CASE("signal emitted on add") {
        SelectionState state;
        EntityRef captured;
        auto conn = state.entitySelected.connect([&](EntityRef ref) { captured = ref; });

        state.addSelection(FACE_1);
        CHECK(captured == FACE_1);
    }

    TEST_CASE("signal emitted on remove") {
        SelectionState state;
        state.addSelection(FACE_1);

        EntityRef captured;
        auto conn = state.entityDeselected.connect([&](EntityRef ref) { captured = ref; });

        state.removeSelection(FACE_1);
        CHECK(captured == FACE_1);
    }

    TEST_CASE("signal emitted on clear") {
        SelectionState state;
        state.addSelection(FACE_1);

        int clear_count = 0;
        auto conn = state.selectionCleared.connect([&]() { ++clear_count; });

        state.clearSelection();
        CHECK(clear_count == 1);
    }

    TEST_CASE("selections are sorted") {
        SelectionState state;
        state.addSelection(FACE_2);
        state.addSelection(VERTEX_1);
        state.addSelection(EDGE_1);

        auto sels = state.selections();
        REQUIRE(sels.size() == 3);
        CHECK(sels[0] < sels[1]);
        CHECK(sels[1] < sels[2]);
    }
}

TEST_SUITE("SceneGraph::selectionState") {
    TEST_CASE("accessible from SceneGraph") {
        OpenGeoLab::Scene::SceneGraph graph;
        auto& sel = graph.selectionState();

        sel.addSelection(FACE_1);
        CHECK(graph.selectionState().isSelected(FACE_1));
    }
}
```

- [ ] **Step 5: Update scene CMakeLists.txt**

Add to `scene_public_headers`:
```
include/opengeolab/scene/selection_state.hpp
```

Add to `scene_sources`:
```
src/selection_state.cpp
```

Add test:
```cmake
opengeolab_add_doctest_test(
    opengeolab_selection_state_test
    SOURCES test/selection_state_test.cpp
    LINKS OpenGeoLab::Scene)
```

- [ ] **Step 6: Build and run tests**

```
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo -R selection_state --output-on-failure
```
Expected: All tests pass.

- [ ] **Step 7: Commit**

```
git add src/libs/scene/
git commit -m "feat(scene): add SelectionState as SceneGraph member

Thread-safe entity-level selection manager with version-based dirty
tracking. Sorted vector storage for O(log n) lookup. Signals for
add/remove/clear/hover. Accessible via sceneGraph.selectionState()."
```

---

## Task 4: Selection Actions in SceneModule

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/select_action.hpp`
- Create: `src/libs/scene/src/select_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/deselect_action.hpp`
- Create: `src/libs/scene/src/deselect_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/clear_selection_action.hpp`
- Create: `src/libs/scene/src/clear_selection_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/query_selection_action.hpp`
- Create: `src/libs/scene/src/query_selection_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/set_pick_mode_action.hpp`
- Create: `src/libs/scene/src/set_pick_mode_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/set_hover_action.hpp`
- Create: `src/libs/scene/src/set_hover_action.cpp`
- Modify: `src/libs/scene/src/scene_module.cpp`
- Modify: `src/libs/scene/include/opengeolab/scene/scene_module.hpp`
- Modify: `src/libs/scene/CMakeLists.txt`
- Create: `src/libs/scene/test/selection_actions_test.cpp`

This task creates 6 actions. Each follows the established IAction pattern from SetVisibilityAction. Actions receive `SelectionState&` from SceneModule.

- [ ] **Step 1: Create SelectAction**

```cpp
// src/libs/scene/include/opengeolab/scene/select_action.hpp
/**
 * @file select_action.hpp
 * @brief SelectAction — add entities to selection
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {
class SelectionState;

class OPENGEOLAB_SCENE_EXPORT SelectAction final : public Core::IAction {
public:
    static constexpr const char* ACTION_NAME = "select";

    explicit SelectAction(SelectionState& state);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

private:
    SelectionState& m_state;
};
} // namespace OpenGeoLab::Scene
```

```cpp
// src/libs/scene/src/select_action.cpp
#include <opengeolab/scene/select_action.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <opengeolab/core/entity_ref.hpp>

namespace OpenGeoLab::Scene {

SelectAction::SelectAction(SelectionState& state) : m_state(state) {}

nlohmann::json SelectAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Add entities to selection."},
            {"params",
             {{"entities",
               {{"type", "array"},
                {"description", "Array of {shapeId, type, localId} objects."}}},
              {"append",
               {{"type", "boolean"},
                {"description",
                 "If false, clears existing selection first. Default true."}}}}},
            {"returns",
             {{"ok", {{"description", "true when the action completes successfully."}}},
              {"selected", {{"description", "Number of entities added."}}}}}};
}

nlohmann::json SelectAction::execute(const nlohmann::json& param, const Core::ProgressCallback& /*progress*/) {
    bool append = true;
    if(param.contains("append") && param["append"].is_boolean()) {
        append = param["append"].get<bool>();
    }
    if(!append) {
        m_state.clearSelection();
    }

    int selected = 0;
    if(param.contains("entities") && param["entities"].is_array()) {
        for(const auto& e : param["entities"]) {
            if(!e.contains("shapeId") || !e.contains("localId")) {
                continue;
            }
            Core::EntityRef ref;
            ref.shapeId = e["shapeId"].get<uint32_t>();
            ref.localId = e["localId"].get<uint32_t>();
            if(e.contains("type") && e["type"].is_string()) {
                const auto& type_str = e["type"].get<std::string>();
                if(type_str == "GeoVertex") ref.entityType = Core::EntityType::GeoVertex;
                else if(type_str == "GeoEdge") ref.entityType = Core::EntityType::GeoEdge;
                else if(type_str == "GeoWire") ref.entityType = Core::EntityType::GeoWire;
                else if(type_str == "GeoFace") ref.entityType = Core::EntityType::GeoFace;
                else if(type_str == "GeoSolid") ref.entityType = Core::EntityType::GeoSolid;
                else continue;
            }
            m_state.addSelection(ref);
            ++selected;
        }
    }
    return {{"ok", true}, {"action", ACTION_NAME}, {"selected", selected}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Create DeselectAction**

```cpp
// src/libs/scene/include/opengeolab/scene/deselect_action.hpp
/**
 * @file deselect_action.hpp
 * @brief DeselectAction — remove entities from selection
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {
class SelectionState;

class OPENGEOLAB_SCENE_EXPORT DeselectAction final : public Core::IAction {
public:
    static constexpr const char* ACTION_NAME = "deselect";

    explicit DeselectAction(SelectionState& state);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

private:
    SelectionState& m_state;
};
} // namespace OpenGeoLab::Scene
```

```cpp
// src/libs/scene/src/deselect_action.cpp
#include <opengeolab/scene/deselect_action.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <opengeolab/core/entity_ref.hpp>

namespace OpenGeoLab::Scene {

DeselectAction::DeselectAction(SelectionState& state) : m_state(state) {}

nlohmann::json DeselectAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Remove entities from selection."},
            {"params",
             {{"entities",
               {{"type", "array"},
                {"description", "Array of {shapeId, type, localId} objects."}}}}},
            {"returns",
             {{"ok", {{"description", "true when the action completes successfully."}}},
              {"removed", {{"description", "Number of entities removed."}}}}}};
}

nlohmann::json DeselectAction::execute(const nlohmann::json& param, const Core::ProgressCallback& /*progress*/) {
    int removed = 0;
    if(param.contains("entities") && param["entities"].is_array()) {
        for(const auto& e : param["entities"]) {
            if(!e.contains("shapeId") || !e.contains("localId")) {
                continue;
            }
            Core::EntityRef ref;
            ref.shapeId = e["shapeId"].get<uint32_t>();
            ref.localId = e["localId"].get<uint32_t>();
            if(e.contains("type") && e["type"].is_string()) {
                const auto& type_str = e["type"].get<std::string>();
                if(type_str == "GeoVertex") ref.entityType = Core::EntityType::GeoVertex;
                else if(type_str == "GeoEdge") ref.entityType = Core::EntityType::GeoEdge;
                else if(type_str == "GeoWire") ref.entityType = Core::EntityType::GeoWire;
                else if(type_str == "GeoFace") ref.entityType = Core::EntityType::GeoFace;
                else if(type_str == "GeoSolid") ref.entityType = Core::EntityType::GeoSolid;
                else continue;
            }
            m_state.removeSelection(ref);
            ++removed;
        }
    }
    return {{"ok", true}, {"action", ACTION_NAME}, {"removed", removed}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Create ClearSelectionAction**

```cpp
// src/libs/scene/include/opengeolab/scene/clear_selection_action.hpp
/**
 * @file clear_selection_action.hpp
 * @brief ClearSelectionAction — clear all selections
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {
class SelectionState;

class OPENGEOLAB_SCENE_EXPORT ClearSelectionAction final : public Core::IAction {
public:
    static constexpr const char* ACTION_NAME = "clear_selection";

    explicit ClearSelectionAction(SelectionState& state);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

private:
    SelectionState& m_state;
};
} // namespace OpenGeoLab::Scene
```

```cpp
// src/libs/scene/src/clear_selection_action.cpp
#include <opengeolab/scene/clear_selection_action.hpp>
#include <opengeolab/scene/selection_state.hpp>

namespace OpenGeoLab::Scene {

ClearSelectionAction::ClearSelectionAction(SelectionState& state) : m_state(state) {}

nlohmann::json ClearSelectionAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Clear all selections."},
            {"params", nlohmann::json::object()},
            {"returns",
             {{"ok", {{"description", "true when the action completes successfully."}}}}}};
}

nlohmann::json ClearSelectionAction::execute(const nlohmann::json& /*param*/,
                                             const Core::ProgressCallback& /*progress*/) {
    m_state.clearSelection();
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Create QuerySelectionAction**

```cpp
// src/libs/scene/include/opengeolab/scene/query_selection_action.hpp
/**
 * @file query_selection_action.hpp
 * @brief QuerySelectionAction — return current selections
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {
class SelectionState;

class OPENGEOLAB_SCENE_EXPORT QuerySelectionAction final : public Core::IAction {
public:
    static constexpr const char* ACTION_NAME = "query_selection";

    explicit QuerySelectionAction(const SelectionState& state);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

private:
    const SelectionState& m_state;
};
} // namespace OpenGeoLab::Scene
```

```cpp
// src/libs/scene/src/query_selection_action.cpp
#include <opengeolab/scene/query_selection_action.hpp>
#include <opengeolab/scene/selection_state.hpp>

namespace OpenGeoLab::Scene {

namespace {

std::string entityTypeName(Core::EntityType type) {
    switch(type) {
    case Core::EntityType::GeoVertex: return "GeoVertex";
    case Core::EntityType::GeoEdge: return "GeoEdge";
    case Core::EntityType::GeoWire: return "GeoWire";
    case Core::EntityType::GeoFace: return "GeoFace";
    case Core::EntityType::GeoSolid: return "GeoSolid";
    case Core::EntityType::MeshNode: return "MeshNode";
    case Core::EntityType::MeshEdge: return "MeshEdge";
    case Core::EntityType::MeshElement: return "MeshElement";
    default: return "Unknown";
    }
}

} // namespace

QuerySelectionAction::QuerySelectionAction(const SelectionState& state) : m_state(state) {}

nlohmann::json QuerySelectionAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Return current selections."},
            {"params", nlohmann::json::object()},
            {"returns",
             {{"ok", {{"description", "true when the action completes successfully."}}},
              {"selections",
               {{"description", "Array of {shapeId, type, localId} objects."}}}}}};
}

nlohmann::json QuerySelectionAction::execute(const nlohmann::json& /*param*/,
                                             const Core::ProgressCallback& /*progress*/) {
    auto selections = m_state.selections();
    auto json_selections = nlohmann::json::array();
    for(const auto& ref : selections) {
        json_selections.push_back({{"shapeId", ref.shapeId},
                                   {"type", entityTypeName(ref.entityType)},
                                   {"localId", ref.localId}});
    }
    return {{"ok", true}, {"action", ACTION_NAME}, {"selections", json_selections}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 5: Create SetPickModeAction**

```cpp
// src/libs/scene/include/opengeolab/scene/set_pick_mode_action.hpp
/**
 * @file set_pick_mode_action.hpp
 * @brief SetPickModeAction — configure pick mode and mask
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {
class SelectionState;

class OPENGEOLAB_SCENE_EXPORT SetPickModeAction final : public Core::IAction {
public:
    static constexpr const char* ACTION_NAME = "set_pick_mode";

    explicit SetPickModeAction(SelectionState& state);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

private:
    SelectionState& m_state;
};
} // namespace OpenGeoLab::Scene
```

```cpp
// src/libs/scene/src/set_pick_mode_action.cpp
#include <opengeolab/scene/set_pick_mode_action.hpp>
#include <opengeolab/scene/selection_state.hpp>

namespace OpenGeoLab::Scene {

SetPickModeAction::SetPickModeAction(SelectionState& state) : m_state(state) {}

nlohmann::json SetPickModeAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Configure pick mode mask and enabled state."},
        {"params",
         {{"pickMask", {{"type", "integer"}, {"description", "Bitmask of entity types."}}},
          {"enabled", {{"type", "boolean"}, {"description", "Enable or disable picking."}}}}},
        {"returns",
         {{"ok", {{"description", "true when the action completes successfully."}}}}}};
}

nlohmann::json SetPickModeAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& /*progress*/) {
    if(param.contains("pickMask") && param["pickMask"].is_number_integer()) {
        m_state.setPickMask(
            static_cast<Core::PickMask>(param["pickMask"].get<uint32_t>()));
    }
    if(param.contains("enabled") && param["enabled"].is_boolean()) {
        m_state.setPickEnabled(param["enabled"].get<bool>());
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 6: Create SetHoverAction**

```cpp
// src/libs/scene/include/opengeolab/scene/set_hover_action.hpp
/**
 * @file set_hover_action.hpp
 * @brief SetHoverAction — set hover entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {
class SelectionState;

class OPENGEOLAB_SCENE_EXPORT SetHoverAction final : public Core::IAction {
public:
    static constexpr const char* ACTION_NAME = "set_hover";

    explicit SetHoverAction(SelectionState& state);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

private:
    SelectionState& m_state;
};
} // namespace OpenGeoLab::Scene
```

```cpp
// src/libs/scene/src/set_hover_action.cpp
#include <opengeolab/scene/set_hover_action.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <opengeolab/core/entity_ref.hpp>

namespace OpenGeoLab::Scene {

SetHoverAction::SetHoverAction(SelectionState& state) : m_state(state) {}

nlohmann::json SetHoverAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Set hover entity for highlight preview."},
        {"params",
         {{"entity",
           {{"type", "object"},
            {"description",
             "{shapeId, type, localId}. Omit or null to clear hover."}}}}},
        {"returns",
         {{"ok", {{"description", "true when the action completes successfully."}}}}}};
}

nlohmann::json SetHoverAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& /*progress*/) {
    if(!param.contains("entity") || param["entity"].is_null()) {
        m_state.clearHover();
        return {{"ok", true}, {"action", ACTION_NAME}};
    }

    const auto& e = param["entity"];
    if(!e.contains("shapeId") || !e.contains("localId")) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing shapeId or localId"}};
    }

    Core::EntityRef ref;
    ref.shapeId = e["shapeId"].get<uint32_t>();
    ref.localId = e["localId"].get<uint32_t>();
    if(e.contains("type") && e["type"].is_string()) {
        const auto& type_str = e["type"].get<std::string>();
        if(type_str == "GeoVertex") ref.entityType = Core::EntityType::GeoVertex;
        else if(type_str == "GeoEdge") ref.entityType = Core::EntityType::GeoEdge;
        else if(type_str == "GeoWire") ref.entityType = Core::EntityType::GeoWire;
        else if(type_str == "GeoFace") ref.entityType = Core::EntityType::GeoFace;
        else if(type_str == "GeoSolid") ref.entityType = Core::EntityType::GeoSolid;
        else return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown entity type"}};
    }

    m_state.setHovered(ref);
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 7: Register actions in SceneModule**

Modify `scene_module.cpp` to register all 6 new actions. Add includes for all action headers.

```cpp
// In SceneModule constructor, after existing registerAction calls:
#include <opengeolab/scene/select_action.hpp>
#include <opengeolab/scene/deselect_action.hpp>
#include <opengeolab/scene/clear_selection_action.hpp>
#include <opengeolab/scene/query_selection_action.hpp>
#include <opengeolab/scene/set_pick_mode_action.hpp>
#include <opengeolab/scene/set_hover_action.hpp>

// In constructor body:
registerAction<SelectAction>(std::ref(m_sceneGraph.selectionState()));
registerAction<DeselectAction>(std::ref(m_sceneGraph.selectionState()));
registerAction<ClearSelectionAction>(std::ref(m_sceneGraph.selectionState()));
registerAction<QuerySelectionAction>(std::cref(m_sceneGraph.selectionState()));
registerAction<SetPickModeAction>(std::ref(m_sceneGraph.selectionState()));
registerAction<SetHoverAction>(std::ref(m_sceneGraph.selectionState()));
```

- [ ] **Step 8: Write selection actions tests**

```cpp
// src/libs/scene/test/selection_actions_test.cpp
/**
 * @file selection_actions_test.cpp
 * @brief Unit tests for selection actions
 */

#include <opengeolab/scene/clear_selection_action.hpp>
#include <opengeolab/scene/deselect_action.hpp>
#include <opengeolab/scene/query_selection_action.hpp>
#include <opengeolab/scene/scene_module.hpp>
#include <opengeolab/scene/select_action.hpp>
#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/scene/set_hover_action.hpp>
#include <opengeolab/scene/set_pick_mode_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Scene::ClearSelectionAction;
using OpenGeoLab::Scene::DeselectAction;
using OpenGeoLab::Scene::QuerySelectionAction;
using OpenGeoLab::Scene::SelectAction;
using OpenGeoLab::Scene::SelectionState;
using OpenGeoLab::Scene::SetHoverAction;
using OpenGeoLab::Scene::SetPickModeAction;

TEST_SUITE("SelectAction") {
    TEST_CASE("select single entity") {
        SelectionState state;
        SelectAction action(state);

        auto result = action.execute(
            {{"entities", {{{"shapeId", 1}, {"type", "GeoFace"}, {"localId", 3}}}},
             {"append", true}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["selected"] == 1);
        CHECK(state.isSelected({1, EntityType::GeoFace, 3}));
    }

    TEST_CASE("select with append=false clears first") {
        SelectionState state;
        state.addSelection({1, EntityType::GeoEdge, 2});

        SelectAction action(state);
        auto result = action.execute(
            {{"entities", {{{"shapeId", 1}, {"type", "GeoFace"}, {"localId", 3}}}},
             {"append", false}},
            nullptr);

        CHECK(result["selected"] == 1);
        CHECK_FALSE(state.isSelected({1, EntityType::GeoEdge, 2}));
        CHECK(state.isSelected({1, EntityType::GeoFace, 3}));
    }

    TEST_CASE("describe returns valid schema") {
        SelectionState state;
        SelectAction action(state);
        auto desc = action.describe();
        CHECK(desc["name"] == "select");
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("entities"));
    }
}

TEST_SUITE("DeselectAction") {
    TEST_CASE("deselect entity") {
        SelectionState state;
        state.addSelection({1, EntityType::GeoFace, 3});

        DeselectAction action(state);
        auto result = action.execute(
            {{"entities", {{{"shapeId", 1}, {"type", "GeoFace"}, {"localId", 3}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["removed"] == 1);
        CHECK_FALSE(state.isSelected({1, EntityType::GeoFace, 3}));
    }
}

TEST_SUITE("ClearSelectionAction") {
    TEST_CASE("clear all") {
        SelectionState state;
        state.addSelection({1, EntityType::GeoFace, 3});
        state.addSelection({1, EntityType::GeoEdge, 2});

        ClearSelectionAction action(state);
        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(state.selections().empty());
    }
}

TEST_SUITE("QuerySelectionAction") {
    TEST_CASE("query returns selections") {
        SelectionState state;
        state.addSelection({1, EntityType::GeoFace, 3});
        state.addSelection({2, EntityType::GeoVertex, 1});

        QuerySelectionAction action(state);
        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["selections"].size() == 2);
    }
}

TEST_SUITE("SetPickModeAction") {
    TEST_CASE("set pick mask and enabled") {
        SelectionState state;
        SetPickModeAction action(state);
        auto result = action.execute({{"pickMask", 0x0B}, {"enabled", true}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(state.pickEnabled());
        CHECK(static_cast<uint32_t>(state.pickMask()) == 0x0B);
    }
}

TEST_SUITE("SetHoverAction") {
    TEST_CASE("set hover entity") {
        SelectionState state;
        SetHoverAction action(state);
        auto result = action.execute(
            {{"entity", {{"shapeId", 1}, {"type", "GeoFace"}, {"localId", 5}}}}, nullptr);

        CHECK(result["ok"] == true);
        REQUIRE(state.hovered().has_value());
        CHECK(state.hovered()->shapeId == 1);
        CHECK(state.hovered()->localId == 5);
    }

    TEST_CASE("clear hover with null entity") {
        SelectionState state;
        state.setHovered({1, EntityType::GeoFace, 5});

        SetHoverAction action(state);
        auto result = action.execute({{"entity", nullptr}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK_FALSE(state.hovered().has_value());
    }
}

TEST_SUITE("SceneModule selection dispatch") {
    TEST_CASE("select dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        OpenGeoLab::Scene::SceneModule mod(factory);

        auto result = mod.process(
            {{"action", "select"},
             {"param",
              {{"entities", {{{"shapeId", 1}, {"type", "GeoFace"}, {"localId", 3}}}},
               {"append", true}}}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(mod.sceneGraph().selectionState().isSelected({1, EntityType::GeoFace, 3}));
    }

    TEST_CASE("query_selection dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        OpenGeoLab::Scene::SceneModule mod(factory);
        mod.sceneGraph().selectionState().addSelection({1, EntityType::GeoFace, 3});

        auto result = mod.process({{"action", "query_selection"}, {"param", {}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["selections"].size() == 1);
    }
}
```

- [ ] **Step 9: Update scene CMakeLists.txt**

Add all 6 new headers to `scene_public_headers`:
```
include/opengeolab/scene/select_action.hpp
include/opengeolab/scene/deselect_action.hpp
include/opengeolab/scene/clear_selection_action.hpp
include/opengeolab/scene/query_selection_action.hpp
include/opengeolab/scene/set_pick_mode_action.hpp
include/opengeolab/scene/set_hover_action.hpp
```

Add 6 new .cpp to `scene_sources`:
```
src/select_action.cpp
src/deselect_action.cpp
src/clear_selection_action.cpp
src/query_selection_action.cpp
src/set_pick_mode_action.cpp
src/set_hover_action.cpp
```

Add test target:
```cmake
opengeolab_add_doctest_test(
    opengeolab_selection_actions_test
    SOURCES test/selection_actions_test.cpp
    LINKS OpenGeoLab::Scene)
```

- [ ] **Step 10: Build and run tests**

```
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo -R selection_actions --output-on-failure
ctest --test-dir build -C RelWithDebInfo -R scene_module --output-on-failure
```

- [ ] **Step 11: Commit**

```
git add src/libs/scene/
git commit -m "feat(scene): add selection actions to SceneModule

Six new actions registered under scene module:
  select, deselect, clear_selection, query_selection, set_pick_mode, set_hover
All accessible via JSON protocol: {module: scene, action: <name>}."
```

---

## Task 5: LabelManager in Scene (Data-Only)

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/label_manager.hpp`
- Create: `src/libs/scene/src/label_manager.cpp`
- Modify: `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- Modify: `src/libs/scene/CMakeLists.txt`
- Create: `src/libs/scene/test/label_manager_test.cpp`

- [ ] **Step 1: Create LabelManager header**

```cpp
// src/libs/scene/include/opengeolab/scene/label_manager.hpp
/**
 * @file label_manager.hpp
 * @brief LabelManager — 3D annotation label storage
 *
 * Phase 1: data-only storage. Phase 2 adds MSDF rendering via LabelPass.
 * Independent of SelectionState — any tool can add labels.
 */

#pragma once

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <glm/vec4.hpp>
#include <kangaroo/util/signal.hpp>

#include <atomic>
#include <shared_mutex>
#include <span>
#include <string>
#include <vector>

namespace OpenGeoLab::Scene {

/// A 3D annotation label attached to an entity.
struct Label3D {
    Core::EntityRef entity;                              ///< Which entity to attach to
    std::string text;                                    ///< Display text ("F:3", "V:1")
    glm::vec4 textColor{1.0F, 1.0F, 1.0F, 1.0F};       ///< White default
    glm::vec4 bgColor{0.0F, 0.0F, 0.0F, 0.7F};         ///< Semi-transparent black
};

/**
 * @brief Thread-safe label storage with version-based dirty tracking.
 *
 * LabelPass (Phase 2) reads labels and computes 3D anchor positions
 * from the entity's geometry data in GpuBufferManager.
 */
class OPENGEOLAB_SCENE_EXPORT LabelManager final {
public:
    LabelManager();
    ~LabelManager();

    void addLabel(Label3D label);
    void removeByEntity(const Core::EntityRef& entity);
    void clearLabels();

    [[nodiscard]] std::vector<Label3D> labels() const;
    [[nodiscard]] uint64_t version() const noexcept;

    Kangaroo::Util::Signal<> labelsChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Label3D> m_labels;
    std::atomic<uint64_t> m_version{0};
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Implement LabelManager**

```cpp
// src/libs/scene/src/label_manager.cpp
#include <opengeolab/scene/label_manager.hpp>

#include <algorithm>

namespace OpenGeoLab::Scene {

LabelManager::LabelManager() = default;
LabelManager::~LabelManager() = default;

void LabelManager::addLabel(Label3D label) {
    if(!label.entity.isValid()) {
        return;
    }
    {
        std::unique_lock lock(m_mutex);
        // Replace existing label for same entity, or append
        auto it = std::find_if(m_labels.begin(), m_labels.end(),
                               [&](const Label3D& l) { return l.entity == label.entity; });
        if(it != m_labels.end()) {
            *it = std::move(label);
        } else {
            m_labels.push_back(std::move(label));
        }
        ++m_version;
    }
    labelsChanged.emit();
}

void LabelManager::removeByEntity(const Core::EntityRef& entity) {
    {
        std::unique_lock lock(m_mutex);
        auto it = std::remove_if(m_labels.begin(), m_labels.end(),
                                 [&](const Label3D& l) { return l.entity == entity; });
        if(it == m_labels.end()) {
            return; // Nothing to remove
        }
        m_labels.erase(it, m_labels.end());
        ++m_version;
    }
    labelsChanged.emit();
}

void LabelManager::clearLabels() {
    {
        std::unique_lock lock(m_mutex);
        if(m_labels.empty()) {
            return;
        }
        m_labels.clear();
        ++m_version;
    }
    labelsChanged.emit();
}

std::vector<Label3D> LabelManager::labels() const {
    std::shared_lock lock(m_mutex);
    return m_labels;
}

uint64_t LabelManager::version() const noexcept {
    return m_version.load(std::memory_order_acquire);
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Add LabelManager to SceneGraph**

In `scene_graph.hpp`, add include:
```cpp
#include <opengeolab/scene/label_manager.hpp>
```

Add public accessors:
```cpp
/** @brief 3D annotation label manager. */
[[nodiscard]] LabelManager& labelManager() { return m_labelManager; }
[[nodiscard]] const LabelManager& labelManager() const { return m_labelManager; }
```

Add private member:
```cpp
LabelManager m_labelManager;
```

- [ ] **Step 4: Write tests**

```cpp
// src/libs/scene/test/label_manager_test.cpp
/**
 * @file label_manager_test.cpp
 * @brief Unit tests for LabelManager
 */

#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Scene::Label3D;
using OpenGeoLab::Scene::LabelManager;

namespace {
constexpr EntityRef FACE_1{1, EntityType::GeoFace, 3};
constexpr EntityRef EDGE_1{1, EntityType::GeoEdge, 2};
} // namespace

TEST_SUITE("LabelManager") {
    TEST_CASE("initially empty") {
        LabelManager mgr;
        CHECK(mgr.labels().empty());
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("add label") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});

        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].entity == FACE_1);
        CHECK(labels[0].text == "F:3");
        CHECK(mgr.version() == 1);
    }

    TEST_CASE("add label for same entity replaces") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});
        mgr.addLabel({FACE_1, "Face 3"});

        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].text == "Face 3");
        CHECK(mgr.version() == 2);
    }

    TEST_CASE("remove by entity") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});
        mgr.addLabel({EDGE_1, "E:2"});
        mgr.removeByEntity(FACE_1);

        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].entity == EDGE_1);
    }

    TEST_CASE("remove nonexistent is no-op") {
        LabelManager mgr;
        mgr.removeByEntity(FACE_1);
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("clear labels") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});
        mgr.addLabel({EDGE_1, "E:2"});
        mgr.clearLabels();

        CHECK(mgr.labels().empty());
        CHECK(mgr.version() == 3);
    }

    TEST_CASE("clear empty is no-op") {
        LabelManager mgr;
        mgr.clearLabels();
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("invalid entity rejected") {
        LabelManager mgr;
        mgr.addLabel({EntityRef{}, "bad"});
        CHECK(mgr.labels().empty());
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("signal emitted on add") {
        LabelManager mgr;
        int signal_count = 0;
        auto conn = mgr.labelsChanged.connect([&]() { ++signal_count; });

        mgr.addLabel({FACE_1, "F:3"});
        CHECK(signal_count == 1);
    }
}

TEST_SUITE("SceneGraph::labelManager") {
    TEST_CASE("accessible from SceneGraph") {
        OpenGeoLab::Scene::SceneGraph graph;
        graph.labelManager().addLabel({FACE_1, "F:3"});
        CHECK(graph.labelManager().labels().size() == 1);
    }
}
```

- [ ] **Step 5: Update CMakeLists.txt, build, test**

Add to `scene_public_headers`:
```
include/opengeolab/scene/label_manager.hpp
```

Add to `scene_sources`:
```
src/label_manager.cpp
```

Add test:
```cmake
opengeolab_add_doctest_test(
    opengeolab_label_manager_test
    SOURCES test/label_manager_test.cpp
    LINKS OpenGeoLab::Scene)
```

Build and test:
```
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo -R label_manager --output-on-failure
```

- [ ] **Step 6: Commit**

```
git commit -m "feat(scene): add LabelManager data structure

Phase 1: data-only label storage in SceneGraph. Phase 2 will add
MSDF rendering via LabelPass. Stores Label3D{entity, text, colors}
with version-based dirty tracking."
```

---

## Task 6: EntityRef → DrawRange Index in GpuBufferManager

**Files:**
- Modify: `src/libs/render/src/core/gpu_buffer_manager.hpp`
- Modify: `src/libs/render/src/core/gpu_buffer_manager.cpp`

- [ ] **Step 1: Add index structure to GpuBufferManager header**

Add to private members:
```cpp
struct EntityRefKey {
    uint32_t shapeId{};
    Core::EntityType entityType{};
    uint32_t localId{};
    bool operator==(const EntityRefKey&) const = default;
};

struct EntityRefKeyHash {
    std::size_t operator()(const EntityRefKey& k) const noexcept {
        auto h = std::hash<uint32_t>{}(k.shapeId);
        h ^= std::hash<uint8_t>{}(static_cast<uint8_t>(k.entityType)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(k.localId) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

std::unordered_map<EntityRefKey, std::vector<Scene::DrawRange>, EntityRefKeyHash> m_entityIndex;
```

Add public method:
```cpp
/**
 * @brief Look up all DrawRanges for an entity.
 * @return Span of DrawRanges, empty if entity not found.
 */
[[nodiscard]] std::span<const Scene::DrawRange> lookupEntity(uint32_t shape_id,
                                                              Core::EntityType entity_type,
                                                              uint32_t local_id) const;
```

- [ ] **Step 2: Build index in rebuildBuffers**

After `adjust_and_append` calls, add index building:
```cpp
auto index_ranges = [this](const std::vector<Scene::DrawRange>& ranges) {
    for(const auto& range : ranges) {
        EntityRefKey key{range.shapeId, range.entityType, range.localId};
        m_entityIndex[key].push_back(range);
    }
};

m_entityIndex.clear();
index_ranges(m_triangleRanges);
index_ranges(m_lineRanges);
index_ranges(m_pointRanges);
```

- [ ] **Step 3: Implement lookupEntity**

```cpp
std::span<const Scene::DrawRange> GpuBufferManager::lookupEntity(
    uint32_t shape_id, Core::EntityType entity_type, uint32_t local_id) const {
    EntityRefKey key{shape_id, entity_type, local_id};
    auto it = m_entityIndex.find(key);
    if(it == m_entityIndex.end()) {
        return {};
    }
    return it->second;
}
```

- [ ] **Step 4: Clear index in cleanup()**

Add `m_entityIndex.clear();` to the `cleanup()` method.

- [ ] **Step 5: Build**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4
```

- [ ] **Step 6: Commit**

```
git add src/libs/render/
git commit -m "feat(render): add EntityRef → DrawRange index in GpuBufferManager

Built during rebuildBuffers alongside DrawRange vectors. Provides O(1)
lookupEntity() for resolving selected entities to renderable draw ranges.
Invalidated and rebuilt when scene version changes."
```

---

## Task 7: PickFbo Rectangular Readback

**Files:**
- Modify: `src/libs/render/src/core/pick_fbo.hpp`
- Modify: `src/libs/render/src/core/pick_fbo.cpp`
- Modify: `src/libs/render/include/opengeolab/render/render_pipeline.hpp`
- Modify: `src/libs/render/src/render_pipeline.cpp`

- [ ] **Step 1: Add readPickRect to PickFbo**

Add to `pick_fbo.hpp`:
```cpp
/**
 * @brief Read all pickIds in an arbitrary rectangle.
 * @param x0, y0 Top-left corner (item space, top-left origin).
 * @param x1, y1 Bottom-right corner.
 * @return Non-zero pickIds found in the rectangle (unsorted).
 */
[[nodiscard]] std::vector<uint64_t> readPickRect(int x0, int y0, int x1, int y1) const;
```

Implement in `pick_fbo.cpp`:
```cpp
std::vector<uint64_t> PickFbo::readPickRect(int x0, int y0, int x1, int y1) const {
    // Normalize coordinates (ensure x0 <= x1, y0 <= y1)
    if(x0 > x1) std::swap(x0, x1);
    if(y0 > y1) std::swap(y0, y1);

    // Clamp to FBO bounds
    x0 = std::clamp(x0, 0, m_width - 1);
    x1 = std::clamp(x1, 0, m_width - 1);
    // Flip Y: item-space top-left to GL bottom-left
    int const gl_y0 = std::clamp(m_height - 1 - y1, 0, m_height - 1);
    int const gl_y1 = std::clamp(m_height - 1 - y0, 0, m_height - 1);

    int const w = x1 - x0 + 1;
    int const h = gl_y1 - gl_y0 + 1;
    if(w <= 0 || h <= 0) {
        return {};
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    std::vector<uint32_t> data(static_cast<size_t>(w * h * 2));
    glReadPixels(x0, gl_y0, w, h, GL_RG_INTEGER, GL_UNSIGNED_INT, data.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    std::vector<uint64_t> result;
    int const pixel_count = w * h;
    for(int i = 0; i < pixel_count; ++i) {
        uint32_t const lo = data[static_cast<size_t>(i) * 2];
        uint32_t const hi = data[static_cast<size_t>(i) * 2 + 1];
        uint64_t const pick_id = (static_cast<uint64_t>(hi) << 32U) | lo;
        if(pick_id != 0) {
            result.push_back(pick_id);
        }
    }
    return result;
}
```

- [ ] **Step 2: Add pickRect to RenderPipeline**

Add to `render_pipeline.hpp`:
```cpp
/**
 * @brief Resolve all picks within an arbitrary rectangle (box-select).
 * @param x0, y0, x1, y1 Rectangle in item-space pixels.
 * @param mask Bitmask controlling which entity types are considered.
 * @return All unique resolved results in the rectangle.
 */
[[nodiscard]] std::vector<PickResult>
pickRect(int x0, int y0, int x1, int y1, PickMask mask) const;
```

Implement in `render_pipeline.cpp`:
```cpp
std::vector<PickResult>
RenderPipeline::pickRect(int x0, int y0, int x1, int y1, PickMask mask) const {
    if(!m_impl->pickResolver) {
        return {};
    }
    const auto raw_pick_ids = m_impl->selectionPass.pickFbo().readPickRect(x0, y0, x1, y1);
    return m_impl->pickResolver->resolveAll(raw_pick_ids, Detail::pickModeFromMask(mask));
}
```

- [ ] **Step 3: Build**

```
cmake --build build --config RelWithDebInfo --target opengeolab_render --parallel 4
```

- [ ] **Step 4: Commit**

```
git add src/libs/render/
git commit -m "feat(render): add rectangular pick readback for box selection

PickFbo::readPickRect reads an arbitrary rectangle from the pick FBO.
RenderPipeline::pickRect wraps it with PickResolver for box select."
```

---

## Task 8: GLViewport Mouse Interaction Changes

**Files:**
- Modify: `src/app/include/opengeolab/app/gl_viewport.hpp`
- Modify: `src/app/src/gl_viewport.cpp`

- [ ] **Step 1: Extend PendingPick and add PendingBoxSelect**

In `gl_viewport.hpp`, update PendingPick and add PendingBoxSelect:
```cpp
struct PendingPick {
    bool active{false};
    float x{0.0F};
    float y{0.0F};
    Core::PickAction action{Core::PickAction::Add};
};

struct PendingBoxSelect {
    bool active{false};
    float x1{0.0F}, y1{0.0F};
    float x2{0.0F}, y2{0.0F};
    Core::PickAction action{Core::PickAction::Add};
};
```

Add consume method and member:
```cpp
[[nodiscard]] PendingBoxSelect consumePendingBoxSelect();

// In Q_PROPERTY section:
Q_PROPERTY(bool boxSelectActive READ boxSelectActive NOTIFY boxSelectActiveChanged)
Q_PROPERTY(QRectF boxSelectRect READ boxSelectRect NOTIFY boxSelectRectChanged)

// Public:
[[nodiscard]] bool boxSelectActive() const { return m_boxSelectActive; }
[[nodiscard]] QRectF boxSelectRect() const { return m_boxSelectRect; }

Q_SIGNALS:
    // ... existing signals ...
    void boxSelectActiveChanged();
    void boxSelectRectChanged();
```

Add private members:
```cpp
PendingBoxSelect m_pendingBoxSelect;
bool m_boxSelectActive{false};
QRectF m_boxSelectRect;
bool m_selectionActive{false};  ///< Synced from SelectionState::pickEnabled()
```

- [ ] **Step 2: Modify mouse event handlers**

In `gl_viewport.cpp`:

**mouseReleaseEvent** — handle left-click (add) and right-click (remove):
```cpp
void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    if(m_trackball.isActive()) {
        m_trackball.end();
        update();
    } else if(!m_movedSincePress && m_pickingEnabled) {
        if(event->button() == Qt::LeftButton) {
            m_pendingPick = {true, static_cast<float>(event->position().x()),
                             static_cast<float>(event->position().y()),
                             Core::PickAction::Add};
            update();
        } else if(event->button() == Qt::RightButton && m_selectionActive) {
            // Right-click deselect only when selection mode is active
            m_pendingPick = {true, static_cast<float>(event->position().x()),
                             static_cast<float>(event->position().y()),
                             Core::PickAction::Remove};
            update();
        }
    } else if(m_movedSincePress && m_pickingEnabled && m_boxSelectActive) {
        // Box select complete
        m_pendingBoxSelect = {true,
                              static_cast<float>(m_pressPos.x()),
                              static_cast<float>(m_pressPos.y()),
                              static_cast<float>(event->position().x()),
                              static_cast<float>(event->position().y()),
                              (m_pressedButtons & Qt::LeftButton) ? Core::PickAction::Add
                                                                  : Core::PickAction::Remove};
        m_boxSelectActive = false;
        m_boxSelectRect = {};
        Q_EMIT boxSelectActiveChanged();
        Q_EMIT boxSelectRectChanged();
        update();
    }

    m_pressedButtons &= ~event->button();
    if(m_pressedButtons == Qt::NoButton) {
        m_pressedModifiers = Qt::NoModifier;
    }
    event->accept();
}
```

**mouseMoveEvent** — box-select rubber-band when selectionActive + left/right drag with no modifier:
```cpp
void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if(m_trackball.isActive()) {
        m_trackball.update(...);
        update();
        event->accept();
        return;
    }

    if(m_pressedButtons == Qt::NoButton) {
        event->ignore();
        return;
    }

    const QLineF drag_line(m_pressPos, event->position());
    if(!m_movedSincePress && drag_line.length() >= DRAG_THRESHOLD_PIXELS) {
        m_movedSincePress = true;

        // Determine if this is a camera gesture or box-select
        const auto mode = mapMouseMode(m_pressedButtons, m_pressedModifiers);
        if(mode != TrackballController::Mode::None) {
            // Camera gesture
            m_trackball.setViewportSize(...);
            m_trackball.begin(..., mode, m_camera);
            m_trackball.update(...);
            update();
        } else if(m_selectionActive) {
            // Box select start — only when selection mode is active
            m_boxSelectActive = true;
            Q_EMIT boxSelectActiveChanged();
        }
    }

    if(m_boxSelectActive) {
        m_boxSelectRect = QRectF(m_pressPos, event->position()).normalized();
        Q_EMIT boxSelectRectChanged();
        update();
    }

    event->accept();
}
```

**mapMouseMode** — in selection mode, right-drag no longer maps to zoom:
```cpp
TrackballController::Mode GLViewport::mapMouseMode(Qt::MouseButtons buttons,
                                                    Qt::KeyboardModifiers modifiers) const {
    if((modifiers & Qt::ControlModifier) && (buttons & Qt::LeftButton)) {
        return TrackballController::Mode::Orbit;
    }
    if(((modifiers & Qt::ShiftModifier) && (buttons & Qt::LeftButton)) ||
       (buttons & Qt::MiddleButton)) {
        return TrackballController::Mode::Pan;
    }
    // Right-drag is zoom ONLY when selection mode is NOT active.
    // m_selectionActive is synced from SelectionState::pickEnabled()
    // during synchronize(). m_pickingEnabled (hover picking) is unaffected.
    if((buttons & Qt::RightButton) && !m_selectionActive) {
        return TrackballController::Mode::Zoom;
    }
    return TrackballController::Mode::None;
}
```

- [ ] **Step 3: Add include for PickAction**

Add `#include <opengeolab/core/pick_action.hpp>` to `gl_viewport.hpp`.

- [ ] **Step 4: Build**

```
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] **Step 5: Commit**

```
git add src/app/
git commit -m "feat(app): extend GLViewport for select/deselect/box-select

PendingPick now carries PickAction (Add/Remove). New PendingBoxSelect
for drag-based box selection. Right-click = deselect, right-drag =
box-deselect (when pickEnabled). Camera zoom via right-drag disabled
in pick mode — use Ctrl+Wheel instead."
```

---

## Task 9: GLViewportRenderer — FrameState Population from SelectionState

**Files:**
- Modify: `src/app/include/opengeolab/app/gl_viewport_renderer.hpp`
- Modify: `src/app/src/gl_viewport_renderer.cpp`

- [ ] **Step 1: Add SelectionState resolution to synchronize()**

In `gl_viewport_renderer.hpp`, add members:
```cpp
GLViewport::PendingBoxSelect m_pendingBoxSelect;
uint64_t m_cachedSelectionVersion{0};
uint64_t m_cachedHoverVersion{0};
std::vector<Scene::DrawRange> m_resolvedSelectedRanges;
std::vector<Scene::DrawRange> m_resolvedHoveredRanges;
```

In `gl_viewport_renderer.cpp`, update `synchronize()`:
```cpp
// After existing FrameState setup, before m_pickingEnabled:
m_pendingBoxSelect = viewport->consumePendingBoxSelect();

// Resolve SelectionState → DrawRanges (only when version changes)
if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
    const auto& sel = scene->selectionState();
    const uint64_t sel_ver = sel.selectionVersion();
    const uint64_t hov_ver = sel.hoverVersion();

    if(sel_ver != m_cachedSelectionVersion) {
        m_resolvedSelectedRanges.clear();
        for(const auto& entity : sel.selections()) {
            auto ranges = m_pipeline.resolveEntityDrawRanges(
                entity.shapeId, entity.entityType, entity.localId);
            m_resolvedSelectedRanges.insert(
                m_resolvedSelectedRanges.end(), ranges.begin(), ranges.end());
        }
        m_cachedSelectionVersion = sel_ver;
    }

    if(hov_ver != m_cachedHoverVersion) {
        m_resolvedHoveredRanges.clear();
        if(auto hovered = sel.hovered(); hovered.has_value()) {
            auto ranges = m_pipeline.resolveEntityDrawRanges(
                hovered->shapeId, hovered->entityType, hovered->localId);
            m_resolvedHoveredRanges.insert(
                m_resolvedHoveredRanges.end(), ranges.begin(), ranges.end());
        }
        m_cachedHoverVersion = hov_ver;
    }

    // Sync selection-active flag for mouse mode mapping
    m_selectionActive = sel.pickEnabled();
}

m_frameState.selectedDrawRanges = m_resolvedSelectedRanges;
m_frameState.hoveredDrawRanges = m_resolvedHoveredRanges;
```

Note: GpuBufferManager is a private implementation detail of the render library.
Add a public forwarding method to `RenderPipeline` instead of exposing the internal type:
```cpp
// render_pipeline.hpp
/**
 * @brief Resolve an entity to its DrawRanges via the internal entity index.
 * @return DrawRanges for the entity (may span multiple primitive types). Empty if not found.
 */
[[nodiscard]] std::vector<Scene::DrawRange> resolveEntityDrawRanges(
    uint32_t shapeId, Core::EntityType entityType, uint32_t localId) const;
```

- [ ] **Step 2: Handle pick dispatch with PickAction**

Update `render()` to pass PickAction and handle box select:
```cpp
void GLViewportRenderer::render() {
    // ... existing pipeline render ...

    if(m_pickingEnabled) {
        if(m_hoverPick.active) {
            dispatchHoverResult(pickAtItemPosition(m_hoverPick.x, m_hoverPick.y));
        }

        if(m_pendingPick.active) {
            auto result = pickAtItemPosition(m_pendingPick.x, m_pendingPick.y);
            dispatchPickResult(result, m_pendingPick.action);
            m_pendingPick = {};
        }

        if(m_pendingBoxSelect.active) {
            dispatchBoxSelectResults(m_pendingBoxSelect);
            m_pendingBoxSelect = {};
        }
    }
    // ...
}
```

Update `dispatchPickResult` to include PickAction. Preserves existing signal behavior
AND adds SelectionState modification when selection mode is active:
```cpp
void GLViewportRenderer::dispatchPickResult(const Render::PickResult& result,
                                            Core::PickAction action) const {
    if(m_viewport.isNull() || !result.valid) {
        return;
    }
    const Core::EntityRef entity{result.shapeId, result.entityType, result.localId};
    const QPointer<GLViewport> viewport = m_viewport;

    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entity, action, result]() {
            if(viewport.isNull()) return;
            // Preserve existing signal behavior (always emitted)
            viewport->notifyPickResult(result);
            // Selection behavior (only when selection mode is active)
            if(viewport->sceneGraph() == nullptr) return;
            auto& sel = viewport->sceneGraph()->selectionState();
            if(!sel.pickEnabled()) return;
            if(action == Core::PickAction::Add) {
                sel.addSelection(entity);
            } else {
                sel.removeSelection(entity);
            }
        },
        Qt::QueuedConnection);
}
```

Update `dispatchHoverResult` to update SelectionState hover alongside existing signals:
```cpp
void GLViewportRenderer::dispatchHoverResult(const Render::PickResult& result) const {
    if(m_viewport.isNull()) return;
    const QPointer<GLViewport> viewport = m_viewport;

    if(!result.valid) {
        QMetaObject::invokeMethod(viewport.data(), [viewport]() {
            if(viewport.isNull()) return;
            viewport->notifyHoverResult(Render::PickResult{});
            if(viewport->sceneGraph() && viewport->sceneGraph()->selectionState().pickEnabled())
                viewport->sceneGraph()->selectionState().clearHover();
        }, Qt::QueuedConnection);
        return;
    }

    const Core::EntityRef entity{result.shapeId, result.entityType, result.localId};
    QMetaObject::invokeMethod(viewport.data(), [viewport, entity, result]() {
        if(viewport.isNull()) return;
        viewport->notifyHoverResult(result);
        if(viewport->sceneGraph() && viewport->sceneGraph()->selectionState().pickEnabled())
            viewport->sceneGraph()->selectionState().setHovered(entity);
    }, Qt::QueuedConnection);
}
```

Add `dispatchBoxSelectResults`:
```cpp
void GLViewportRenderer::dispatchBoxSelectResults(
    const GLViewport::PendingBoxSelect& box) const {
    if(m_viewport.isNull()) return;

    const float dpr = m_frameState.devicePixelRatio;
    const int px0 = static_cast<int>(box.x1 * dpr);
    const int py0 = static_cast<int>(box.y1 * dpr);
    const int px1 = static_cast<int>(box.x2 * dpr);
    const int py1 = static_cast<int>(box.y2 * dpr);

    auto results = m_pipeline.pickRect(px0, py0, px1, py1, pickMask());
    if(results.empty()) return;

    std::vector<Core::EntityRef> entities;
    entities.reserve(results.size());
    for(const auto& r : results) {
        if(r.valid) {
            entities.push_back({r.shapeId, r.entityType, r.localId});
        }
    }

    const QPointer<GLViewport> viewport = m_viewport;
    const Core::PickAction action = box.action;
    QMetaObject::invokeMethod(viewport.data(), [viewport, entities, action]() {
        if(viewport.isNull() || !viewport->sceneGraph()) return;
        auto& sel = viewport->sceneGraph()->selectionState();
        for(const auto& entity : entities) {
            if(action == Core::PickAction::Add) {
                sel.addSelection(entity);
            } else {
                sel.removeSelection(entity);
            }
        }
    }, Qt::QueuedConnection);
}
```

- [ ] **Step 3: Build**

```
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] **Step 4: Commit**

```
git add src/app/ src/libs/render/
git commit -m "feat(app): populate FrameState from SelectionState

GLViewportRenderer resolves EntityRef → DrawRange via GpuBufferManager
index during synchronize(). Version-based dirty tracking avoids
redundant resolution. Pick dispatch uses PickAction to add/remove.
Box select dispatches batch results to main thread."
```

---

## Task 10: SelectionService QML Bridge

**Files:**
- Create: `src/app/include/opengeolab/app/selection_service.hpp`
- Create: `src/app/src/selection_service.cpp`
- Modify: `src/app/CMakeLists.txt`

- [ ] **Step 1: Create SelectionService header**

QML singleton that bridges SelectionState signals to QML. Follows the pattern of existing QML singletons (TranslationManager, RequestService).

```cpp
// src/app/include/opengeolab/app/selection_service.hpp
/**
 * @file selection_service.hpp
 * @brief QML singleton bridging SelectionState to QML
 */

#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <kangaroo/util/signal.hpp>

#include <vector>

namespace OpenGeoLab::Scene {
class SelectionState;
} // namespace OpenGeoLab::Scene

namespace OpenGeoLab::App {

class SelectionService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool pickEnabled READ pickEnabled WRITE setPickEnabled NOTIFY pickEnabledChanged)
    Q_PROPERTY(int pickMask READ pickMask WRITE setPickMask NOTIFY pickMaskChanged)
    Q_PROPERTY(QVariantList selections READ selections NOTIFY selectionChanged)

public:
    explicit SelectionService(QObject* parent = nullptr);
    ~SelectionService() override;

    void setSelectionState(Scene::SelectionState* state);

    [[nodiscard]] bool pickEnabled() const;
    void setPickEnabled(bool enabled);

    [[nodiscard]] int pickMask() const;
    void setPickMask(int mask);

    [[nodiscard]] QVariantList selections() const;

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

private:
    Scene::SelectionState* m_state{nullptr};
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::App
```

- [ ] **Step 2: Implement SelectionService**

Connects to SelectionState signals, bridges to QML via Q_SIGNALS. Uses QMetaObject::invokeMethod for thread-safe signal emission.

- [ ] **Step 3: Wire SelectionService in main.cpp**

After SceneModule creation, set SelectionState on SelectionService singleton.

- [ ] **Step 4: Update CMakeLists.txt**

Add `selection_service.cpp` to app sources and `selection_service.hpp` to SOURCES in qt_add_qml_module.

- [ ] **Step 5: Build**

```
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] **Step 6: Commit**

```
git add src/app/
git commit -m "feat(app): add SelectionService QML singleton

Bridges SelectionState signals to QML. Provides activatePickMode,
deactivatePickMode, clearSelection, removeSelection Q_INVOKABLE
methods and selections Q_PROPERTY for the query panel."
```

---

## Task 11: GeoQueryPage QML Panel

**Files:**
- Create: `src/app/resource/qml/components/EntityTypeSelector.qml`
- Create: `src/app/resource/qml/components/EntityChip.qml`
- Create: `src/app/resource/qml/components/pages/GeoQueryPage.qml`
- Modify: `src/app/resource/qml/MainPages.qml`
- Modify: `src/app/CMakeLists.txt`

- [ ] **Step 1: Create EntityTypeSelector.qml**

Row of toggle buttons for V/E/F/Solid. V/E/F combinable via bitmask OR. Solid mutually exclusive with V/E/F. Emits `maskChanged(int)`.

- [ ] **Step 2: Create EntityChip.qml**

Removable chip showing entity info (e.g. "F:3 [Shape 1]"). Has close button. Emits `removeRequested(shapeId, entityType, localId)`.

- [ ] **Step 3: Create GeoQueryPage.qml**

Extends `FunctionPageBase.qml`. Contains:
- EntityTypeSelector
- Pick mode indicator
- Flow layout of EntityChip components
- Clear All button
- Connects to SelectionService signals

On open: calls `SelectionService.activatePickMode(mask)`.
On close: calls `SelectionService.deactivatePickMode()` and `SelectionService.clearSelection()`.

- [ ] **Step 4: Register in MainPages.qml**

Add `"geoQuery": "components/pages/GeoQueryPage.qml"` to componentMap.

- [ ] **Step 5: Add query action to RibbonConfig.qml**

Add a "Query" button that triggers the `geoQuery` action.

- [ ] **Step 6: Update CMakeLists.txt**

Add new QML files to `qt_add_qml_module QML_FILES` list.

- [ ] **Step 7: Build and test UI**

```
cmake --build build --config RelWithDebInfo --parallel 4
```

Run the app and verify the Query panel opens and closes.

- [ ] **Step 8: Commit**

```
git add src/app/
git commit -m "feat(app): add GeoQueryPage with entity type selector

Geometry Query panel with V/E/F/Solid type toggles, pick mode
activation, entity chip display, and clear-all button. Integrates
with SelectionService for pick mode control."
```

---

## Task 12: Box Selection Rubber-Band Overlay

**Files:**
- Modify: `src/app/resource/qml/sections/ViewportPanel.qml`

- [ ] **Step 1: Add rubber-band rectangle overlay**

In `ViewportPanel.qml`, add a `Rectangle` overlay on top of the GLViewport that renders when `glViewport.boxSelectActive` is true:

```qml
Rectangle {
    id: rubberBand
    visible: glViewport.boxSelectActive
    x: glViewport.boxSelectRect.x
    y: glViewport.boxSelectRect.y
    width: glViewport.boxSelectRect.width
    height: glViewport.boxSelectRect.height
    color: "transparent"
    border.width: 1
    // Blue for left-drag (add), red for right-drag (remove)
    border.color: /* ... detect based on mouse button */
    opacity: 0.3
    Rectangle {
        anchors.fill: parent
        color: parent.border.color
        opacity: 0.15
    }
}
```

- [ ] **Step 2: Build and verify**

```
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] **Step 3: Commit**

```
git add src/app/resource/qml/
git commit -m "feat(app): add box selection rubber-band overlay

Blue rectangle for left-drag (add selection), rendered as a QML
overlay on top of the viewport. Uses GLViewport's boxSelectActive
and boxSelectRect properties."
```

---

## Task 13: Integration Testing and Final Verification

- [ ] **Step 1: Full build**

```
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Clean build, zero errors.

- [ ] **Step 2: Run all tests**

```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: All tests pass including new selection_state, entity_ref, selection_actions, label_manager tests.

- [ ] **Step 3: Verify existing tests unbroken**

Check that scene_module_test, pick_resolver_test, scene_graph_test all still pass.

- [ ] **Step 4: Manual smoke test**

Launch the app, import a BREP model, open the Query panel:
1. Click on faces → highlights appear, chips show in panel
2. Right-click on selected face → deselects
3. Box-select with left-drag → multiple selections
4. Toggle entity types (V/E/F)
5. Close panel → selection clears

- [ ] **Step 5: Final commit**

```
git add -A
git commit -m "test(selection): integration verification

All unit tests pass. Manual smoke test confirms click select,
right-click deselect, box select, entity type toggle, and
panel lifecycle all working correctly."
```
