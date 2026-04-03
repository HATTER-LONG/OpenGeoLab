# GeoQuery Command Protocol & Label Visibility Lifecycle — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate GeoQueryPage write operations to the command dispatch protocol, add label management commands, and implement per-panel label visibility lifecycle (default off).

**Architecture:** Add 5 new scene actions (add_label, remove_label, clear_labels, set_labels_visible, set_auto_label). Move labelsVisible/autoLabel state from SelectionService locals to LabelManager (source of truth). Wire LabelManager.isVisible() → FrameState.labelsVisible in GLViewportRenderer. Refactor GeoQueryPage to use RequestService.submitAsync() for all writes.

**Tech Stack:** C++20, Qt 6, doctest, nlohmann/json

**Build/Test commands:**
```
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `src/libs/core/include/opengeolab/core/label_colors.hpp` | MODIFY | Add `formatLabelText()` utility |
| `src/libs/scene/include/opengeolab/scene/label_manager.hpp` | MODIFY | Add visible/autoLabel state + signals |
| `src/libs/scene/src/label_manager.cpp` | MODIFY | Implement visible/autoLabel setters |
| `src/libs/scene/test/label_manager_test.cpp` | MODIFY | Tests for visible/autoLabel |
| `src/libs/scene/include/opengeolab/scene/add_label_action.hpp` | CREATE | AddLabelAction header |
| `src/libs/scene/src/add_label_action.cpp` | CREATE | AddLabelAction implementation |
| `src/libs/scene/include/opengeolab/scene/remove_label_action.hpp` | CREATE | RemoveLabelAction header |
| `src/libs/scene/src/remove_label_action.cpp` | CREATE | RemoveLabelAction implementation |
| `src/libs/scene/include/opengeolab/scene/clear_labels_action.hpp` | CREATE | ClearLabelsAction header |
| `src/libs/scene/src/clear_labels_action.cpp` | CREATE | ClearLabelsAction implementation |
| `src/libs/scene/include/opengeolab/scene/set_labels_visible_action.hpp` | CREATE | SetLabelsVisibleAction header |
| `src/libs/scene/src/set_labels_visible_action.cpp` | CREATE | SetLabelsVisibleAction implementation |
| `src/libs/scene/include/opengeolab/scene/set_auto_label_action.hpp` | CREATE | SetAutoLabelAction header |
| `src/libs/scene/src/set_auto_label_action.cpp` | CREATE | SetAutoLabelAction implementation |
| `src/libs/scene/test/label_actions_test.cpp` | CREATE | Tests for all 5 label actions |
| `src/libs/scene/src/scene_module.cpp` | MODIFY | Register 5 new actions |
| `src/libs/scene/CMakeLists.txt` | MODIFY | Add new source/header/test files |
| `src/app/src/gl_viewport_renderer.cpp` | MODIFY | Read LabelManager.isVisible() → FrameState |
| `src/libs/render/include/opengeolab/render/frame_state.hpp` | MODIFY | Default labelsVisible to false |
| `src/app/include/opengeolab/app/selection_service.hpp` | MODIFY | Delegate labelsVisible/autoLabel to LabelManager |
| `src/app/src/selection_service.cpp` | MODIFY | Bridge LabelManager signals; remove local state |
| `src/app/resource/qml/components/pages/GeoQueryPage.qml` | MODIFY | Use commands for writes + label lifecycle |

---

### Pre-task 0: Commit MSDF Label Visual Fixes

**Files:** All currently modified/staged files.

- [ ] **Step 1: Stage all changes**

```bash
git add -A
```

- [ ] **Step 2: Commit**

```bash
git commit -m "fix(render): fix MSDF label visual bugs and simplify rendering

- MSDF linear clamp fixes glyph background bleed
- Tight background sizing using ascender-descender
- Baseline centering formula for proper vertical alignment
- Glyph Y-coordinate sign fix (planeBounds Y-up matches screen Y-up)
- Atlas Y-flip via stbi_set_flip_vertically_on_load
- QML import fix for Switch component (QtQuick.Controls.Basic)
- Disabled depth test (labels always on top)
- Removed unused occlusion infrastructure
- Label text format: [shapeId]Type:localId
- Reduced font scale to 15px with tighter padding

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 1: Extend LabelManager with Visible and AutoLabel State

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/label_manager.hpp`
- Modify: `src/libs/scene/src/label_manager.cpp`
- Modify: `src/libs/scene/test/label_manager_test.cpp`
- Modify: `src/libs/core/include/opengeolab/core/label_colors.hpp`

- [ ] **Step 1: Write failing tests for visible/autoLabel**

Append to `src/libs/scene/test/label_manager_test.cpp`:

```cpp
TEST_CASE("initially not visible") {
    const LabelManager mgr;
    CHECK_FALSE(mgr.isVisible());
}

TEST_CASE("setVisible toggles state") {
    LabelManager mgr;
    mgr.setVisible(true);
    CHECK(mgr.isVisible());
    mgr.setVisible(false);
    CHECK_FALSE(mgr.isVisible());
}

TEST_CASE("setVisible emits signal on change") {
    LabelManager mgr;
    int count = 0;
    auto conn = mgr.visibleChanged.connect([&]() { ++count; });
    mgr.setVisible(true);
    CHECK(count == 1);
    mgr.setVisible(true); // no-op
    CHECK(count == 1);
    mgr.setVisible(false);
    CHECK(count == 2);
}

TEST_CASE("initially autoLabel disabled") {
    const LabelManager mgr;
    CHECK_FALSE(mgr.autoLabel());
}

TEST_CASE("setAutoLabel toggles state") {
    LabelManager mgr;
    mgr.setAutoLabel(true);
    CHECK(mgr.autoLabel());
}

TEST_CASE("setAutoLabel emits signal on change") {
    LabelManager mgr;
    int count = 0;
    auto conn = mgr.autoLabelChanged.connect([&]() { ++count; });
    mgr.setAutoLabel(true);
    CHECK(count == 1);
    mgr.setAutoLabel(true); // no-op
    CHECK(count == 1);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4`
Expected: Compile error — `isVisible`, `setVisible`, `visibleChanged`, `autoLabel`, `setAutoLabel`, `autoLabelChanged` not declared.

- [ ] **Step 3: Add visible/autoLabel state to LabelManager header**

In `src/libs/scene/include/opengeolab/scene/label_manager.hpp`, add after the existing `labelsChanged` signal:

```cpp
/// Set whether labels should be rendered. Default: false.
void setVisible(bool visible);

/// Whether labels should be rendered.
[[nodiscard]] bool isVisible() const noexcept;

/// Set whether auto-label on selection is enabled. Default: false.
void setAutoLabel(bool enabled);

/// Whether auto-label on selection is enabled.
[[nodiscard]] bool autoLabel() const noexcept;

Kangaroo::Util::Signal<> visibleChanged;   ///< Emitted when visibility changes.
Kangaroo::Util::Signal<> autoLabelChanged; ///< Emitted when auto-label changes.
```

Add private members:

```cpp
bool m_visible{false};
bool m_autoLabel{false};
```

- [ ] **Step 4: Implement in label_manager.cpp**

Append to `src/libs/scene/src/label_manager.cpp` before the closing namespace:

```cpp
void LabelManager::setVisible(bool visible) {
    if(m_visible != visible) {
        m_visible = visible;
        visibleChanged.emit();
    }
}

bool LabelManager::isVisible() const noexcept {
    return m_visible;
}

void LabelManager::setAutoLabel(bool enabled) {
    if(m_autoLabel != enabled) {
        m_autoLabel = enabled;
        autoLabelChanged.emit();
    }
}

bool LabelManager::autoLabel() const noexcept {
    return m_autoLabel;
}
```

- [ ] **Step 5: Add formatLabelText utility**

In `src/libs/core/include/opengeolab/core/label_colors.hpp`, add:

```cpp
/// Build display text for a label: "[shapeId]Prefix:localId".
[[nodiscard]] inline std::string formatLabelText(uint32_t shape_id, EntityType type,
                                                  uint32_t local_id) {
    return "[" + std::to_string(shape_id) + "]" + std::string(labelPrefix(type)) + ":" +
           std::to_string(local_id);
}
```

Also add `#include <string>` to the includes if not already present.

- [ ] **Step 6: Run tests to verify they pass**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R label_manager --output-on-failure`
Expected: All label_manager tests PASS.

- [ ] **Step 7: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/label_manager.hpp \
        src/libs/scene/src/label_manager.cpp \
        src/libs/scene/test/label_manager_test.cpp \
        src/libs/core/include/opengeolab/core/label_colors.hpp
git commit -m "feat(scene): add visible and autoLabel state to LabelManager

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: Create Label Management Actions

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/add_label_action.hpp`
- Create: `src/libs/scene/src/add_label_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/remove_label_action.hpp`
- Create: `src/libs/scene/src/remove_label_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/clear_labels_action.hpp`
- Create: `src/libs/scene/src/clear_labels_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/set_labels_visible_action.hpp`
- Create: `src/libs/scene/src/set_labels_visible_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/set_auto_label_action.hpp`
- Create: `src/libs/scene/src/set_auto_label_action.cpp`
- Create: `src/libs/scene/test/label_actions_test.cpp`
- Modify: `src/libs/scene/src/scene_module.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Write failing tests for all 5 actions**

Create `src/libs/scene/test/label_actions_test.cpp`:

```cpp
/**
 * @file label_actions_test.cpp
 * @brief Tests for label management actions
 */

#include <opengeolab/scene/add_label_action.hpp>
#include <opengeolab/scene/clear_labels_action.hpp>
#include <opengeolab/scene/remove_label_action.hpp>
#include <opengeolab/scene/set_auto_label_action.hpp>
#include <opengeolab/scene/set_labels_visible_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using OpenGeoLab::Core::ProgressCallback;
using OpenGeoLab::Scene::AddLabelAction;
using OpenGeoLab::Scene::ClearLabelsAction;
using OpenGeoLab::Scene::LabelManager;
using OpenGeoLab::Scene::RemoveLabelAction;
using OpenGeoLab::Scene::SetAutoLabelAction;
using OpenGeoLab::Scene::SetLabelsVisibleAction;

namespace {
const ProgressCallback NO_PROGRESS;
} // namespace

TEST_SUITE("AddLabelAction") {
    TEST_CASE("adds label with correct format and colors") {
        LabelManager mgr;
        AddLabelAction action(mgr);

        auto result =
            action.execute({{"shapeId", 1}, {"entityType", "GeoFace"}, {"localId", 3}}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].text == "[1]F:3");
        CHECK(labels[0].entity.shapeId == 1);
        CHECK(labels[0].entity.localId == 3);
    }

    TEST_CASE("replaces existing label for same entity") {
        LabelManager mgr;
        AddLabelAction action(mgr);

        action.execute({{"shapeId", 1}, {"entityType", "GeoEdge"}, {"localId", 2}}, NO_PROGRESS);
        action.execute({{"shapeId", 1}, {"entityType", "GeoEdge"}, {"localId", 2}}, NO_PROGRESS);

        CHECK(mgr.labels().size() == 1);
    }

    TEST_CASE("returns error for unknown entity type") {
        LabelManager mgr;
        AddLabelAction action(mgr);

        auto result =
            action.execute({{"shapeId", 1}, {"entityType", "Unknown"}, {"localId", 1}}, NO_PROGRESS);

        CHECK(result["ok"] == false);
        CHECK(mgr.labels().empty());
    }

    TEST_CASE("describe returns valid schema") {
        LabelManager mgr;
        AddLabelAction action(mgr);
        auto desc = action.describe();
        CHECK(desc["name"] == "add_label");
        CHECK(desc.contains("params"));
        CHECK(desc.contains("returns"));
    }
}

TEST_SUITE("RemoveLabelAction") {
    TEST_CASE("removes existing label") {
        LabelManager mgr;
        AddLabelAction add(mgr);
        RemoveLabelAction remove(mgr);

        add.execute({{"shapeId", 1}, {"entityType", "GeoFace"}, {"localId", 3}}, NO_PROGRESS);
        auto result =
            remove.execute({{"shapeId", 1}, {"entityType", "GeoFace"}, {"localId", 3}}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["removed"] == true);
        CHECK(mgr.labels().empty());
    }

    TEST_CASE("no-op for nonexistent label") {
        LabelManager mgr;
        RemoveLabelAction remove(mgr);

        auto result =
            remove.execute({{"shapeId", 1}, {"entityType", "GeoFace"}, {"localId", 99}}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["removed"] == false);
    }
}

TEST_SUITE("ClearLabelsAction") {
    TEST_CASE("clears all labels") {
        LabelManager mgr;
        AddLabelAction add(mgr);
        ClearLabelsAction clear(mgr);

        add.execute({{"shapeId", 1}, {"entityType", "GeoFace"}, {"localId", 1}}, NO_PROGRESS);
        add.execute({{"shapeId", 1}, {"entityType", "GeoEdge"}, {"localId", 2}}, NO_PROGRESS);

        auto result = clear.execute({}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["cleared"] == 2);
        CHECK(mgr.labels().empty());
    }
}

TEST_SUITE("SetLabelsVisibleAction") {
    TEST_CASE("sets visibility") {
        LabelManager mgr;
        SetLabelsVisibleAction action(mgr);

        auto result = action.execute({{"visible", true}}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(mgr.isVisible());
    }

    TEST_CASE("returns error without visible param") {
        LabelManager mgr;
        SetLabelsVisibleAction action(mgr);

        auto result = action.execute({}, NO_PROGRESS);

        CHECK(result["ok"] == false);
    }
}

TEST_SUITE("SetAutoLabelAction") {
    TEST_CASE("enables auto-label") {
        LabelManager mgr;
        SetAutoLabelAction action(mgr);

        auto result = action.execute({{"enabled", true}}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(mgr.autoLabel());
    }

    TEST_CASE("disables auto-label") {
        LabelManager mgr;
        SetAutoLabelAction action(mgr);

        mgr.setAutoLabel(true);
        auto result = action.execute({{"enabled", false}}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK_FALSE(mgr.autoLabel());
    }
}
```

- [ ] **Step 2: Create AddLabelAction**

Header `src/libs/scene/include/opengeolab/scene/add_label_action.hpp`:

```cpp
/**
 * @file add_label_action.hpp
 * @brief AddLabelAction — add a 3D label for an entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/// Add a label for a given entity with auto-generated text/colors.
class OPENGEOLAB_SCENE_EXPORT AddLabelAction final : public Core::IAction {
public:
    explicit AddLabelAction(LabelManager& manager);
    ~AddLabelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"add_label"};

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
```

Implementation `src/libs/scene/src/add_label_action.cpp`:

```cpp
/**
 * @file add_label_action.cpp
 * @brief AddLabelAction — add a 3D label for an entity
 */

#include <opengeolab/scene/add_label_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/label_colors.hpp>

namespace OpenGeoLab::Scene {

AddLabelAction::AddLabelAction(LabelManager& manager) : m_manager(manager) {}
AddLabelAction::~AddLabelAction() = default;

nlohmann::json AddLabelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Add a 3D label for an entity with auto-generated text and colors."},
        {"params",
         {{"shapeId", {{"type", "integer"}, {"required", true}, {"description", "Shape ID"}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "GeoVertex | GeoEdge | GeoWire | GeoFace | GeoSolid"}}},
          {"localId",
           {{"type", "integer"}, {"required", true}, {"description", "Local ID within shape"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Success flag"}}},
          {"action", {{"type", "string"}, {"description", "Echo action name"}}},
          {"text", {{"type", "string"}, {"description", "Generated label text"}}}}}};
}

nlohmann::json AddLabelAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    auto shape_id = param.value("shapeId", 0u);
    auto local_id = param.value("localId", 0u);
    auto type_str = param.value("entityType", std::string{});

    auto type = parseEntityType(type_str);
    if(!type.has_value()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Unknown entityType: " + type_str}};
    }

    Label3D label;
    label.entity = {shape_id, *type, local_id};
    label.text = Core::formatLabelText(shape_id, *type, local_id);
    label.textColor = Core::labelColor(*type);
    label.bgColor = Core::K_LABEL_BG_COLOR;

    m_manager.addLabel(std::move(label));

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"text", Core::formatLabelText(shape_id, *type, local_id)}};
}

} // namespace OpenGeoLab::Scene
```

> **Note:** Each action that parses entity types should include a local `parseEntityType()` function in an anonymous namespace, matching the pattern in `select_action.cpp` / `deselect_action.cpp`. This returns `std::optional<Core::EntityType>`.

- [ ] **Step 3: Create RemoveLabelAction**

Header `src/libs/scene/include/opengeolab/scene/remove_label_action.hpp`:

```cpp
/**
 * @file remove_label_action.hpp
 * @brief RemoveLabelAction — remove a label by entity reference
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/// Remove a label for a given entity.
class OPENGEOLAB_SCENE_EXPORT RemoveLabelAction final : public Core::IAction {
public:
    explicit RemoveLabelAction(LabelManager& manager);
    ~RemoveLabelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"remove_label"};

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
```

Implementation `src/libs/scene/src/remove_label_action.cpp`:

```cpp
/**
 * @file remove_label_action.cpp
 * @brief RemoveLabelAction — remove a label by entity reference
 */

#include <opengeolab/scene/remove_label_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

#include <opengeolab/core/entity_tag.hpp>

namespace OpenGeoLab::Scene {

RemoveLabelAction::RemoveLabelAction(LabelManager& manager) : m_manager(manager) {}
RemoveLabelAction::~RemoveLabelAction() = default;

nlohmann::json RemoveLabelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Remove a label for the specified entity."},
        {"params",
         {{"shapeId", {{"type", "integer"}, {"required", true}, {"description", "Shape ID"}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "GeoVertex | GeoEdge | GeoWire | GeoFace | GeoSolid"}}},
          {"localId",
           {{"type", "integer"}, {"required", true}, {"description", "Local ID within shape"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Success flag"}}},
          {"action", {{"type", "string"}, {"description", "Echo action name"}}},
          {"removed", {{"type", "boolean"}, {"description", "Whether a label was removed"}}}}}};
}

nlohmann::json RemoveLabelAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& progress) {
    auto shape_id = param.value("shapeId", 0u);
    auto local_id = param.value("localId", 0u);
    auto type_str = param.value("entityType", std::string{});

    auto type = parseEntityType(type_str);
    if(!type.has_value()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Unknown entityType: " + type_str},
                {"removed", false}};
    }

    Core::EntityRef entity{shape_id, *type, local_id};
    auto version_before = m_manager.version();
    m_manager.removeByEntity(entity);
    bool was_removed = (m_manager.version() != version_before);

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"removed", was_removed}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Create ClearLabelsAction**

Header `src/libs/scene/include/opengeolab/scene/clear_labels_action.hpp`:

```cpp
/**
 * @file clear_labels_action.hpp
 * @brief ClearLabelsAction — remove all labels
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/// Remove all labels from the label manager.
class OPENGEOLAB_SCENE_EXPORT ClearLabelsAction final : public Core::IAction {
public:
    explicit ClearLabelsAction(LabelManager& manager);
    ~ClearLabelsAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"clear_labels"};

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
```

Implementation `src/libs/scene/src/clear_labels_action.cpp`:

```cpp
/**
 * @file clear_labels_action.cpp
 * @brief ClearLabelsAction — remove all labels
 */

#include <opengeolab/scene/clear_labels_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

ClearLabelsAction::ClearLabelsAction(LabelManager& manager) : m_manager(manager) {}
ClearLabelsAction::~ClearLabelsAction() = default;

nlohmann::json ClearLabelsAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Remove all active labels."},
            {"params", nlohmann::json::object()},
            {"returns",
             {{"ok", {{"type", "boolean"}, {"description", "Success flag"}}},
              {"action", {{"type", "string"}, {"description", "Echo action name"}}},
              {"cleared", {{"type", "integer"}, {"description", "Number of labels removed"}}}}}};
}

nlohmann::json ClearLabelsAction::execute(const nlohmann::json& /*param*/,
                                          const Core::ProgressCallback& progress) {
    auto count = m_manager.labels().size();
    m_manager.clearLabels();

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"cleared", count}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 5: Create SetLabelsVisibleAction**

Header `src/libs/scene/include/opengeolab/scene/set_labels_visible_action.hpp`:

```cpp
/**
 * @file set_labels_visible_action.hpp
 * @brief SetLabelsVisibleAction — toggle label rendering visibility
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/// Set whether labels are rendered in the viewport.
class OPENGEOLAB_SCENE_EXPORT SetLabelsVisibleAction final : public Core::IAction {
public:
    explicit SetLabelsVisibleAction(LabelManager& manager);
    ~SetLabelsVisibleAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"set_labels_visible"};

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
```

Implementation `src/libs/scene/src/set_labels_visible_action.cpp`:

```cpp
/**
 * @file set_labels_visible_action.cpp
 * @brief SetLabelsVisibleAction — toggle label rendering visibility
 */

#include <opengeolab/scene/set_labels_visible_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

SetLabelsVisibleAction::SetLabelsVisibleAction(LabelManager& manager) : m_manager(manager) {}
SetLabelsVisibleAction::~SetLabelsVisibleAction() = default;

nlohmann::json SetLabelsVisibleAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Set whether labels are rendered in the viewport."},
        {"params",
         {{"visible",
           {{"type", "boolean"}, {"required", true}, {"description", "true to show labels"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Success flag"}}},
          {"action", {{"type", "string"}, {"description", "Echo action name"}}}}}};
}

nlohmann::json SetLabelsVisibleAction::execute(const nlohmann::json& param,
                                               const Core::ProgressCallback& progress) {
    if(!param.contains("visible") || !param["visible"].is_boolean()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing required param: visible"}};
    }

    m_manager.setVisible(param["visible"].get<bool>());

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 6: Create SetAutoLabelAction**

Header `src/libs/scene/include/opengeolab/scene/set_auto_label_action.hpp`:

```cpp
/**
 * @file set_auto_label_action.hpp
 * @brief SetAutoLabelAction — toggle automatic labeling on selection
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/// Enable or disable automatic label creation when entities are selected.
class OPENGEOLAB_SCENE_EXPORT SetAutoLabelAction final : public Core::IAction {
public:
    explicit SetAutoLabelAction(LabelManager& manager);
    ~SetAutoLabelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"set_auto_label"};

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
```

Implementation `src/libs/scene/src/set_auto_label_action.cpp`:

```cpp
/**
 * @file set_auto_label_action.cpp
 * @brief SetAutoLabelAction — toggle automatic labeling on selection
 */

#include <opengeolab/scene/set_auto_label_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

SetAutoLabelAction::SetAutoLabelAction(LabelManager& manager) : m_manager(manager) {}
SetAutoLabelAction::~SetAutoLabelAction() = default;

nlohmann::json SetAutoLabelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Enable or disable automatic label creation on entity selection."},
        {"params",
         {{"enabled",
           {{"type", "boolean"},
            {"required", true},
            {"description", "true to auto-label on select"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Success flag"}}},
          {"action", {{"type", "string"}, {"description", "Echo action name"}}}}}};
}

nlohmann::json SetAutoLabelAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    if(!param.contains("enabled") || !param["enabled"].is_boolean()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Missing required param: enabled"}};
    }

    m_manager.setAutoLabel(param["enabled"].get<bool>());

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 7: Register actions in SceneModule**

In `src/libs/scene/src/scene_module.cpp`, add includes:

```cpp
#include <opengeolab/scene/add_label_action.hpp>
#include <opengeolab/scene/clear_labels_action.hpp>
#include <opengeolab/scene/remove_label_action.hpp>
#include <opengeolab/scene/set_auto_label_action.hpp>
#include <opengeolab/scene/set_labels_visible_action.hpp>
```

Add registrations in the constructor after the existing `DescribeLabelsAction` registration:

```cpp
registerAction<AddLabelAction>(std::ref(m_sceneGraph.labelManager()));
registerAction<RemoveLabelAction>(std::ref(m_sceneGraph.labelManager()));
registerAction<ClearLabelsAction>(std::ref(m_sceneGraph.labelManager()));
registerAction<SetLabelsVisibleAction>(std::ref(m_sceneGraph.labelManager()));
registerAction<SetAutoLabelAction>(std::ref(m_sceneGraph.labelManager()));
```

- [ ] **Step 8: Update CMakeLists.txt**

In `src/libs/scene/CMakeLists.txt`, add to `scene_public_headers`:

```cmake
include/opengeolab/scene/add_label_action.hpp
include/opengeolab/scene/remove_label_action.hpp
include/opengeolab/scene/clear_labels_action.hpp
include/opengeolab/scene/set_labels_visible_action.hpp
include/opengeolab/scene/set_auto_label_action.hpp
```

Add to `scene_sources`:

```cmake
src/add_label_action.cpp
src/remove_label_action.cpp
src/clear_labels_action.cpp
src/set_labels_visible_action.cpp
src/set_auto_label_action.cpp
```

Add to test section:

```cmake
test/label_actions_test.cpp
```

- [ ] **Step 9: Build and run tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 4 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS including new label_actions tests.

- [ ] **Step 10: Commit**

```bash
git add src/libs/scene/
git commit -m "feat(scene): add label management actions for command protocol

Five new scene actions:
- scene.add_label: create label with auto text/color from entity ref
- scene.remove_label: remove label by entity ref
- scene.clear_labels: remove all labels
- scene.set_labels_visible: toggle label rendering
- scene.set_auto_label: toggle auto-label on selection

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: Wire labelsVisible Through Render Pipeline

**Files:**
- Modify: `src/libs/render/include/opengeolab/render/frame_state.hpp`
- Modify: `src/app/src/gl_viewport_renderer.cpp`

- [ ] **Step 1: Change FrameState default**

In `src/libs/render/include/opengeolab/render/frame_state.hpp`, change:

```cpp
// OLD:
bool labelsVisible{true};
// NEW:
bool labelsVisible{false};  ///< Default off; enabled per-panel via command.
```

- [ ] **Step 2: Read LabelManager.isVisible() in synchronize()**

In `src/app/src/gl_viewport_renderer.cpp`, in the `synchronize()` method, find where labels are resolved (around the `lbl_mgr` usage). Add after reading labels:

```cpp
m_frameState.labelsVisible = lbl_mgr.isVisible();
```

This should go near line 179 where `const auto& lbl_mgr = scene->labelManager();` is used.

- [ ] **Step 3: Build and verify**

Run: `cmake --build build --config RelWithDebInfo --parallel 4 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS. Labels now hidden by default (LabelManager.isVisible() starts as false).

- [ ] **Step 4: Commit**

```bash
git add src/libs/render/include/opengeolab/render/frame_state.hpp \
        src/app/src/gl_viewport_renderer.cpp
git commit -m "feat(render): wire LabelManager.isVisible to FrameState

Labels are now hidden by default. Visibility is controlled via
scene.set_labels_visible command which sets LabelManager state,
read by GLViewportRenderer in synchronize().

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: Refactor SelectionService to Use LabelManager State

**Files:**
- Modify: `src/app/include/opengeolab/app/selection_service.hpp`
- Modify: `src/app/src/selection_service.cpp`

- [ ] **Step 1: Update header — delegate reads to LabelManager**

In `src/app/include/opengeolab/app/selection_service.hpp`:

Remove private members:

```cpp
// REMOVE:
bool m_labelsVisible{true};
bool m_autoLabel{true};
```

- [ ] **Step 2: Update implementation — bridge LabelManager signals**

In `src/app/src/selection_service.cpp`:

Update `setLabelManager()` to connect signals:

```cpp
void SelectionService::setLabelManager(Scene::LabelManager* manager) {
    m_labelManager = manager;
    if(m_labelManager != nullptr) {
        m_connections.push_back(m_labelManager->visibleChanged.connect([this]() {
            QMetaObject::invokeMethod(
                this, [this]() { Q_EMIT labelsVisibleChanged(); }, Qt::QueuedConnection);
        }));
        m_connections.push_back(m_labelManager->autoLabelChanged.connect([this]() {
            QMetaObject::invokeMethod(
                this, [this]() { Q_EMIT autoLabelChanged(); }, Qt::QueuedConnection);
        }));
    }
}
```

Update `labelsVisible()`:

```cpp
bool SelectionService::labelsVisible() const {
    return m_labelManager != nullptr && m_labelManager->isVisible();
}
```

Update `setLabelsVisible()`:

```cpp
void SelectionService::setLabelsVisible(bool visible) {
    if(m_labelManager != nullptr) {
        m_labelManager->setVisible(visible);
    }
}
```

Update `autoLabel()`:

```cpp
bool SelectionService::autoLabel() const {
    return m_labelManager != nullptr && m_labelManager->autoLabel();
}
```

Update `setAutoLabel()`:

```cpp
void SelectionService::setAutoLabel(bool enabled) {
    if(m_labelManager != nullptr) {
        m_labelManager->setAutoLabel(enabled);
    }
}
```

Update auto-label check in `connectSignals()` — change `m_autoLabel` to read from LabelManager:

```cpp
// In entitySelected handler, change:
if(m_autoLabel && m_labelManager != nullptr) {
// To:
if(m_labelManager != nullptr && m_labelManager->autoLabel()) {
```

Same for `entityDeselected` handler.

Update `addLabelForSelection` to use shared utility:

```cpp
void SelectionService::addLabelForSelection(int shapeId, int entityType, int localId) {
    if(m_labelManager == nullptr) {
        return;
    }
    auto type = static_cast<Core::EntityType>(entityType);
    Scene::Label3D label;
    label.entity = {static_cast<uint32_t>(shapeId), type, static_cast<uint32_t>(localId)};
    label.text = Core::formatLabelText(static_cast<uint32_t>(shapeId), type,
                                        static_cast<uint32_t>(localId));
    label.textColor = Core::labelColor(type);
    label.bgColor = Core::K_LABEL_BG_COLOR;
    m_labelManager->addLabel(std::move(label));
}
```

- [ ] **Step 3: Build and run all tests**

Run: `cmake --build build --config RelWithDebInfo --parallel 4 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 4: Commit**

```bash
git add src/app/include/opengeolab/app/selection_service.hpp \
        src/app/src/selection_service.cpp
git commit -m "refactor(app): delegate labelsVisible/autoLabel to LabelManager

SelectionService no longer owns label visibility or auto-label state.
Both are read from LabelManager (source of truth set via commands).
LabelManager signals are bridged to Qt signals for QML binding updates.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: Refactor GeoQueryPage to Command Protocol and Label Lifecycle

**Files:**
- Modify: `src/app/resource/qml/components/pages/GeoQueryPage.qml`

- [ ] **Step 1: Add command helper functions**

Add at the top of the GeoQueryPage component (after the `function execute()` block):

```qml
/** @brief Send a scene command via the command protocol. */
function sceneCommand(action, param) {
    RequestService.submitAsync(JSON.stringify({
        module: "scene",
        action: action,
        param: param ?? {},
        mute: true
    }));
}

/** @brief Map entity type integer to command protocol string. */
function entityTypeTag(typeInt) {
    var map = {
        1: "GeoVertex",
        2: "GeoEdge",
        3: "GeoWire",
        4: "GeoFace",
        5: "GeoSolid"
    };
    return map[typeInt] ?? "GeoFace";
}
```

> **Note:** Verify the EntityType integer values match the C++ enum. Check `entity_ref.hpp` for exact values.

- [ ] **Step 2: Refactor open() to use commands + enable labels**

```qml
function open(payload) {
    root.x = 292;
    root.y = 0;
    root.pageVisible = true;
    root.forceActiveFocus();
    sceneCommand("set_pick_mode", {
        pickMask: typeSelector.mask,
        enabled: true
    });
    sceneCommand("set_labels_visible", { visible: true });
    sceneCommand("set_auto_label", { enabled: true });
}
```

- [ ] **Step 3: Refactor close() to use commands + restore defaults**

```qml
function close() {
    sceneCommand("set_pick_mode", { enabled: false });
    sceneCommand("clear_selection", {});
    sceneCommand("set_labels_visible", { visible: false });
    sceneCommand("set_auto_label", { enabled: false });
    sceneCommand("clear_labels", {});
    root.pageVisible = false;
    if (MainPages.currentOpenPage === root.actionId) {
        MainPages.currentOpenPage = "";
    }
}
```

- [ ] **Step 4: Refactor mask change handler**

```qml
onMaskChanged: {
    sceneCommand("set_pick_mode", { pickMask: typeSelector.mask });
}
```

- [ ] **Step 5: Refactor auto-label toggle**

```qml
Switch {
    id: labelToggle
    checked: SelectionService.autoLabel
    onCheckedChanged: sceneCommand("set_auto_label", { enabled: checked })
}
```

- [ ] **Step 6: Refactor entity removal**

```qml
onRemoveRequested: function(sid, etype, lid) {
    sceneCommand("deselect", {
        entities: [{ shapeId: sid, type: root.entityTypeTag(etype), localId: lid }]
    });
}
```

- [ ] **Step 7: Refactor Clear All button**

```qml
onClicked: sceneCommand("clear_selection", {})
```

- [ ] **Step 8: Build and verify**

Run: `cmake --build build --config RelWithDebInfo --parallel 4`
Expected: Build succeeds. QML changes don't need compilation but the app should be tested manually.

- [ ] **Step 9: Commit**

```bash
git add src/app/resource/qml/components/pages/GeoQueryPage.qml
git commit -m "refactor(app): migrate GeoQueryPage writes to command protocol

All state-mutating operations now go through RequestService/scene module:
- set_pick_mode for pick activation/deactivation/mask changes
- clear_selection / deselect for selection management
- set_labels_visible / set_auto_label / clear_labels for label lifecycle

Panel open: enables labels + auto-label via commands.
Panel close: disables labels + auto-label, clears labels via commands.
Read-only Q_PROPERTY bindings retained for UI efficiency.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Notes

- **Existing commands reused:** `scene.set_pick_mode`, `scene.clear_selection`, `scene.deselect` — no changes needed.
- **Q_PROPERTY reads remain:** `SelectionService.pickEnabled`, `.pickMask`, `.selections`, `.labelsVisible`, `.autoLabel` — still read directly for QML binding efficiency.
- **Auto-label behavior stays in SelectionService:** The signal handler that auto-creates labels when entities are selected is internal logic, not a QML-initiated operation. It reads `autoLabel()` from LabelManager.
- **Thread safety:** LabelManager state uses `std::shared_mutex` for data members. The simple bool `m_visible`/`m_autoLabel` don't need mutex since they're only written from the command dispatch thread and read from the render thread (single-word reads are atomic on x86).
- **parseEntityType():** Each action that parses entity type strings should include a local `parseEntityType()` in an anonymous namespace, matching the existing pattern in `select_action.cpp`.
