# Scene Module Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a SceneModule that exposes visibility control through the Command/Action system, enabling unified QML + Python access.

**Architecture:** SceneModule owns SceneGraph and registers two actions (`set_visibility`, `list_nodes`) via the standard ModuleBase/IAction pattern. QML calls go through the existing RequestService singleton. ModuleDataNotifier bridges scene events to QML for reactive UI updates. SidebarPanel drops local visibility state and queries `scene.list_nodes` for truth.

**Tech Stack:** C++20, nlohmann/json, doctest, Qt 6 (QML), CMake

**Build:** `cmake --build build --config RelWithDebInfo --parallel 8`
**Test:** `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

**Spec:** `docs/superpowers/specs/2026-03-31-scene-module-design.md`

---

## File Map

### New Files

| File | Responsibility |
|------|---------------|
| `src/libs/scene/include/opengeolab/scene/scene_module.hpp` | SceneModule class — owns SceneGraph, registers actions |
| `src/libs/scene/src/scene_module.cpp` | SceneModule implementation |
| `src/libs/scene/include/opengeolab/scene/set_visibility_action.hpp` | SetVisibilityAction header |
| `src/libs/scene/src/set_visibility_action.cpp` | SetVisibilityAction — batch set node visibility |
| `src/libs/scene/include/opengeolab/scene/list_nodes_action.hpp` | ListNodesAction header |
| `src/libs/scene/src/list_nodes_action.cpp` | ListNodesAction — query all scene nodes |
| `src/libs/scene/test/scene_module_test.cpp` | Unit tests for both actions + dataChanged signal |

### Modified Files

| File | Change |
|------|--------|
| `src/libs/scene/CMakeLists.txt` | Add new source files + test target |
| `src/libs/command/CMakeLists.txt` | Add `OpenGeoLab::Scene` link dependency |
| `src/libs/command/src/module_registry.cpp` | Register SceneModule |
| `src/app/include/opengeolab/app/module_data_notifier.h` | Add `sceneDataChanged()` signal |
| `src/app/src/module_data_notifier.cpp` | Subscribe to `"scene"` module events |
| `src/app/include/opengeolab/app/gl_viewport.hpp` | Remove `setShapeVisible()` |
| `src/app/src/gl_viewport.cpp` | Remove `setShapeVisible()` implementation |
| `src/app/src/main.cpp` | Get SceneGraph from SceneModule; connect sceneDataChanged |
| `src/app/resource/qml/sections/SidebarPanel.qml` | Remove local visibilityState; use command for toggle; query list_nodes |
| `src/app/resource/qml/Main.qml` | Remove `viewport` property from SidebarPanel |

---

### Task 1: SetVisibilityAction (TDD)

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/set_visibility_action.hpp`
- Create: `src/libs/scene/src/set_visibility_action.cpp`
- Create: `src/libs/scene/test/scene_module_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Write the test file with set_visibility tests**

Create `src/libs/scene/test/scene_module_test.cpp`:

```cpp
/**
 * @file scene_module_test.cpp
 * @brief Unit tests for SceneModule actions
 */

#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/set_visibility_action.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Scene::Tests {

TEST_SUITE("SetVisibilityAction") {

TEST_CASE("single node set invisible") {
    SceneGraph graph;
    auto* node = graph.addNode("Box_1");
    REQUIRE(node != nullptr);
    REQUIRE(node->isVisible());

    SetVisibilityAction action(graph);
    nlohmann::json param = {
        {"nodes", {{{"nodeId", node->id()}, {"visible", false}}}}
    };
    auto result = action.execute(param, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "set_visibility");
    CHECK(result["updated"] == 1);
    CHECK(result["skipped"] == 0);
    CHECK_FALSE(node->isVisible());
}

TEST_CASE("batch set visibility") {
    SceneGraph graph;
    auto* a = graph.addNode("A");
    auto* b = graph.addNode("B");
    auto* c = graph.addNode("C");

    SetVisibilityAction action(graph);
    nlohmann::json param = {
        {"nodes", {
            {{"nodeId", a->id()}, {"visible", false}},
            {{"nodeId", b->id()}, {"visible", false}},
            {{"nodeId", c->id()}, {"visible", true}}
        }}
    };
    auto result = action.execute(param, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["updated"] == 2);
    CHECK(result["skipped"] == 0);
    CHECK_FALSE(a->isVisible());
    CHECK_FALSE(b->isVisible());
    CHECK(c->isVisible());
}

TEST_CASE("node not found increments skipped") {
    SceneGraph graph;
    SetVisibilityAction action(graph);
    nlohmann::json param = {
        {"nodes", {{{"nodeId", 999}, {"visible", false}}}}
    };
    auto result = action.execute(param, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["updated"] == 0);
    CHECK(result["skipped"] == 1);
}

TEST_CASE("empty nodes array succeeds") {
    SceneGraph graph;
    SetVisibilityAction action(graph);
    nlohmann::json param = {{"nodes", nlohmann::json::array()}};
    auto result = action.execute(param, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["updated"] == 0);
    CHECK(result["skipped"] == 0);
}

TEST_CASE("no actual change yields updated=0") {
    SceneGraph graph;
    auto* node = graph.addNode("Box_1");
    REQUIRE(node->isVisible());

    SetVisibilityAction action(graph);
    nlohmann::json param = {
        {"nodes", {{{"nodeId", node->id()}, {"visible", true}}}}
    };
    auto result = action.execute(param, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["updated"] == 0);
    CHECK(result["skipped"] == 0);
}

TEST_CASE("describe returns valid schema") {
    SceneGraph graph;
    SetVisibilityAction action(graph);
    auto desc = action.describe();

    CHECK(desc["name"] == "set_visibility");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc.contains("returns"));
}

} // TEST_SUITE

} // namespace OpenGeoLab::Scene::Tests
```

- [ ] **Step 2: Write the SetVisibilityAction header**

Create `src/libs/scene/include/opengeolab/scene/set_visibility_action.hpp`:

```cpp
/**
 * @file set_visibility_action.hpp
 * @brief SetVisibilityAction — batch set scene node visibility
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Batch-set visibility of scene nodes.
 *
 * Param: {"nodes": [{"nodeId": <int>, "visible": <bool>}, ...]}
 * Nodes not found are counted in "skipped" (no failure).
 */
class OPENGEOLAB_SCENE_EXPORT SetVisibilityAction final : public Core::IAction {
public:
    explicit SetVisibilityAction(SceneGraph& graph);
    ~SetVisibilityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"set_visibility"};

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Write the SetVisibilityAction implementation**

Create `src/libs/scene/src/set_visibility_action.cpp`:

```cpp
/**
 * @file set_visibility_action.cpp
 * @brief SetVisibilityAction — batch set scene node visibility
 */

#include <opengeolab/scene/set_visibility_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>

namespace OpenGeoLab::Scene {

SetVisibilityAction::SetVisibilityAction(SceneGraph& graph) : m_graph(graph) {}
SetVisibilityAction::~SetVisibilityAction() = default;

nlohmann::json SetVisibilityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Batch-set visibility of scene nodes."},
        {"params",
         {{"nodes",
           {{"type", "array"},
            {"required", true},
            {"description",
             "Array of {nodeId: int, visible: bool} pairs."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Always true."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"updated", {{"type", "integer"}, {"description", "Nodes whose visibility changed."}}},
          {"skipped", {{"type", "integer"}, {"description", "Node IDs not found."}}}}}};
}

nlohmann::json SetVisibilityAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto& nodes = param.value("nodes", nlohmann::json::array());
    int updated = 0;
    int skipped = 0;

    for (const auto& entry : nodes) {
        const auto node_id = entry.value("nodeId", static_cast<NodeId>(0));
        const auto visible = entry.value("visible", true);

        auto* node = m_graph.findNode(node_id);
        if (node == nullptr) {
            ++skipped;
            continue;
        }
        if (node->isVisible() != visible) {
            m_graph.setNodeVisible(node_id, visible);
            ++updated;
        }
    }

    if (progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", "set_visibility"},
            {"updated", updated},
            {"skipped", skipped}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Add sources and test to scene CMakeLists.txt**

In `src/libs/scene/CMakeLists.txt`, add to the header list, source list, and add a test:

Add `include/opengeolab/scene/set_visibility_action.hpp` to `scene_public_headers`.

Add `src/set_visibility_action.cpp` to `scene_sources`.

Add test block:

```cmake
    opengeolab_add_doctest_test(
        opengeolab_scene_module_test
        SOURCES test/scene_module_test.cpp
        LINKS OpenGeoLab::Scene)
```

- [ ] **Step 5: Build and run the test**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Then: `cmake --build build --config RelWithDebInfo --target opengeolab_scene_module_test --parallel 8`
Then: `ctest --test-dir build -C RelWithDebInfo -R scene_module --output-on-failure`

Expected: All 6 set_visibility tests pass.

- [ ] **Step 6: Commit**

```
git add src/libs/scene/include/opengeolab/scene/set_visibility_action.hpp \
        src/libs/scene/src/set_visibility_action.cpp \
        src/libs/scene/test/scene_module_test.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): add SetVisibilityAction with batch support

Implements scene.set_visibility action that batch-sets node visibility.
Nodes not found are counted in 'skipped' without failing.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: ListNodesAction (TDD)

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/list_nodes_action.hpp`
- Create: `src/libs/scene/src/list_nodes_action.cpp`
- Modify: `src/libs/scene/test/scene_module_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Add list_nodes tests to the existing test file**

Append to `src/libs/scene/test/scene_module_test.cpp`, inside the namespace but after the `SetVisibilityAction` test suite closing brace:

```cpp
#include <opengeolab/scene/list_nodes_action.hpp>

TEST_SUITE("ListNodesAction") {

TEST_CASE("empty scene returns no nodes") {
    SceneGraph graph;
    ListNodesAction action(graph);
    auto result = action.execute({}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "list_nodes");
    CHECK(result["nodes"].size() == 0);
}

TEST_CASE("lists multiple nodes with correct fields") {
    SceneGraph graph;
    auto* a = graph.addNode("Box_1");
    auto* b = graph.addNode("Cyl_1");

    ListNodesAction action(graph);
    auto result = action.execute({}, nullptr);

    CHECK(result["ok"] == true);
    auto nodes = result["nodes"];
    REQUIRE(nodes.size() == 2);

    // Find the node entries by nodeId
    bool found_a = false;
    bool found_b = false;
    for (const auto& n : nodes) {
        CHECK(n.contains("nodeId"));
        CHECK(n.contains("name"));
        CHECK(n.contains("visible"));
        CHECK(n.contains("parentId"));
        if (n["nodeId"] == a->id()) {
            CHECK(n["name"] == "Box_1");
            CHECK(n["visible"] == true);
            CHECK(n["parentId"] == 0);
            found_a = true;
        }
        if (n["nodeId"] == b->id()) {
            CHECK(n["name"] == "Cyl_1");
            CHECK(n["visible"] == true);
            CHECK(n["parentId"] == 0);
            found_b = true;
        }
    }
    CHECK(found_a);
    CHECK(found_b);
}

TEST_CASE("visibility reflected in list_nodes") {
    SceneGraph graph;
    auto* node = graph.addNode("Box_1");
    graph.setNodeVisible(node->id(), false);

    ListNodesAction action(graph);
    auto result = action.execute({}, nullptr);

    REQUIRE(result["nodes"].size() == 1);
    CHECK(result["nodes"][0]["visible"] == false);
}

TEST_CASE("describe returns valid schema") {
    SceneGraph graph;
    ListNodesAction action(graph);
    auto desc = action.describe();

    CHECK(desc["name"] == "list_nodes");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc.contains("returns"));
}

} // TEST_SUITE
```

- [ ] **Step 2: Write the ListNodesAction header**

Create `src/libs/scene/include/opengeolab/scene/list_nodes_action.hpp`:

```cpp
/**
 * @file list_nodes_action.hpp
 * @brief ListNodesAction — query all scene nodes with visibility state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Query all scene nodes (excluding root) with id, name, visibility, parentId.
 */
class OPENGEOLAB_SCENE_EXPORT ListNodesAction final : public Core::IAction {
public:
    explicit ListNodesAction(const SceneGraph& graph);
    ~ListNodesAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"list_nodes"};

private:
    const SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Write the ListNodesAction implementation**

Create `src/libs/scene/src/list_nodes_action.cpp`:

```cpp
/**
 * @file list_nodes_action.cpp
 * @brief ListNodesAction — query all scene nodes with visibility state
 */

#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <functional>

namespace OpenGeoLab::Scene {

ListNodesAction::ListNodesAction(const SceneGraph& graph) : m_graph(graph) {}
ListNodesAction::~ListNodesAction() = default;

nlohmann::json ListNodesAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "List all scene nodes with visibility state."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Always true."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"nodes",
           {{"type", "array"},
            {"description",
             "Array of {nodeId, name, visible, parentId} objects."}}}}}};
}

nlohmann::json ListNodesAction::execute(const nlohmann::json& /*param*/,
                                        const Core::ProgressCallback& progress) {
    auto lock = m_graph.readLock();
    nlohmann::json nodes = nlohmann::json::array();

    std::function<void(const SceneNode&)> collect = [&](const SceneNode& node) {
        nodes.push_back({
            {"nodeId", node.id()},
            {"name", std::string(node.name())},
            {"visible", node.isVisible()},
            {"parentId", node.parent() ? node.parent()->id() : 0}
        });
        for (const auto& child : node.children()) {
            collect(*child);
        }
    };

    for (const auto& child : m_graph.root()->children()) {
        collect(*child);
    }

    if (progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "list_nodes"}, {"nodes", nodes}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Add new files to scene CMakeLists.txt**

Add `include/opengeolab/scene/list_nodes_action.hpp` to `scene_public_headers`.
Add `src/list_nodes_action.cpp` to `scene_sources`.

- [ ] **Step 5: Build and run all scene module tests**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Then: `cmake --build build --config RelWithDebInfo --target opengeolab_scene_module_test --parallel 8`
Then: `ctest --test-dir build -C RelWithDebInfo -R scene_module --output-on-failure`

Expected: All 10 tests pass (6 set_visibility + 4 list_nodes).

- [ ] **Step 6: Commit**

```
git add src/libs/scene/include/opengeolab/scene/list_nodes_action.hpp \
        src/libs/scene/src/list_nodes_action.cpp \
        src/libs/scene/test/scene_module_test.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): add ListNodesAction for querying scene state

Returns nodeId, name, visible, parentId for all non-root nodes.
Uses read lock for thread safety.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: SceneModule class (TDD)

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/scene_module.hpp`
- Create: `src/libs/scene/src/scene_module.cpp`
- Modify: `src/libs/scene/test/scene_module_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Add SceneModule tests to the test file**

Append to `src/libs/scene/test/scene_module_test.cpp`. These tests need `PluginComponentFactory`:

```cpp
#include <opengeolab/scene/scene_module.hpp>
#include <opengeolab/core/module_data_event.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

TEST_SUITE("SceneModule") {

TEST_CASE("module name is scene") {
    Kangaroo::Util::PluginComponentFactory factory;
    SceneModule module(factory);

    CHECK(module.moduleName() == "scene");
}

TEST_CASE("sceneGraph accessor returns owned graph") {
    Kangaroo::Util::PluginComponentFactory factory;
    SceneModule module(factory);

    auto* node = module.sceneGraph().addNode("TestNode");
    REQUIRE(node != nullptr);
    CHECK(module.sceneGraph().findNode(node->id()) == node);
}

TEST_CASE("set_visibility dispatches through module process") {
    Kangaroo::Util::PluginComponentFactory factory;
    SceneModule module(factory);

    auto* node = module.sceneGraph().addNode("Box");
    REQUIRE(node->isVisible());

    nlohmann::json request = {
        {"action", "set_visibility"},
        {"param", {{"nodes", {{{"nodeId", node->id()}, {"visible", false}}}}}}
    };
    auto result = module.process(request, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["updated"] == 1);
    CHECK_FALSE(node->isVisible());
}

TEST_CASE("list_nodes dispatches through module process") {
    Kangaroo::Util::PluginComponentFactory factory;
    SceneModule module(factory);

    module.sceneGraph().addNode("A");
    module.sceneGraph().addNode("B");

    nlohmann::json request = {
        {"action", "list_nodes"},
        {"param", nlohmann::json::object()}
    };
    auto result = module.process(request, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["nodes"].size() == 2);
}

TEST_CASE("dataChanged emitted on set_visibility mutation") {
    Kangaroo::Util::PluginComponentFactory factory;
    SceneModule module(factory);

    auto* node = module.sceneGraph().addNode("Box");

    int signal_count = 0;
    auto conn = module.dataChanged.connect(
        [&](Core::ModuleDataEvent) { ++signal_count; });

    // nodeAdded already fired during addNode, reset counter
    signal_count = 0;

    nlohmann::json request = {
        {"action", "set_visibility"},
        {"param", {{"nodes", {{{"nodeId", node->id()}, {"visible", false}}}}}}
    };
    module.process(request, nullptr);

    // setNodeVisible triggers nodeUpdated → dataChanged(ItemModified)
    CHECK(signal_count > 0);
}

} // TEST_SUITE
```

- [ ] **Step 2: Write the SceneModule header**

Create `src/libs/scene/include/opengeolab/scene/scene_module.hpp`:

```cpp
/**
 * @file scene_module.hpp
 * @brief SceneModule — scene state management module
 *
 * Owns the SceneGraph and exposes scene operations (visibility, etc.)
 * through the Command/Action protocol.
 *
 * Request format: {"module": "scene", "action": "<name>", "param": {...}}
 */

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <kangaroo/util/signal.hpp>

#include <vector>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Scene {

/**
 * @brief Scene module — owns SceneGraph and delegates to factory-managed IAction singletons.
 *
 * Bridges SceneGraph signals (nodeAdded, nodeRemoved, nodeUpdated) to
 * ModuleBase::dataChanged for the event bus.
 */
class OPENGEOLAB_SCENE_EXPORT SceneModule final : public Core::ModuleBase {
public:
    explicit SceneModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~SceneModule() override;

    /** @brief Access the scene graph owned by this module. */
    [[nodiscard]] SceneGraph& sceneGraph();
    [[nodiscard]] const SceneGraph& sceneGraph() const;

    static constexpr std::string_view MODULE_NAME{"scene"};

private:
    SceneGraph m_sceneGraph;
    std::vector<Kangaroo::Util::ScopedConnection> m_graphConnections;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Write the SceneModule implementation**

Create `src/libs/scene/src/scene_module.cpp`:

```cpp
/**
 * @file scene_module.cpp
 * @brief SceneModule — registers scene actions and bridges signals
 */

#include <opengeolab/scene/scene_module.hpp>

#include <opengeolab/core/module_data_event.hpp>
#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/set_visibility_action.hpp>

#include <functional>

namespace OpenGeoLab::Scene {

SceneModule::SceneModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "Scene state management module.", factory) {
    registerAction<SetVisibilityAction>(std::ref(m_sceneGraph));
    registerAction<ListNodesAction>(std::cref(m_sceneGraph));

    m_graphConnections.push_back(
        m_sceneGraph.nodeAdded.connect([this](NodeId) {
            dataChanged.emit(Core::ModuleDataEvent::ItemAdded);
        }));
    m_graphConnections.push_back(
        m_sceneGraph.nodeRemoved.connect([this](NodeId) {
            dataChanged.emit(Core::ModuleDataEvent::ItemRemoved);
        }));
    m_graphConnections.push_back(
        m_sceneGraph.nodeUpdated.connect([this](NodeId) {
            dataChanged.emit(Core::ModuleDataEvent::ItemModified);
        }));
}

SceneModule::~SceneModule() = default;

SceneGraph& SceneModule::sceneGraph() { return m_sceneGraph; }
const SceneGraph& SceneModule::sceneGraph() const { return m_sceneGraph; }

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Add new files to scene CMakeLists.txt**

Add `include/opengeolab/scene/scene_module.hpp` to `scene_public_headers`.
Add `src/scene_module.cpp` to `scene_sources`.

Note: the scene module already links to `OpenGeoLab::Core` (which provides `ModuleBase`, `IAction`, `PluginComponentFactory`). No new link dependencies needed for the scene lib.

- [ ] **Step 5: Build and run all scene module tests**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Then: `cmake --build build --config RelWithDebInfo --target opengeolab_scene_module_test --parallel 8`
Then: `ctest --test-dir build -C RelWithDebInfo -R scene_module --output-on-failure`

Expected: All 15 tests pass (6 + 4 + 5).

- [ ] **Step 6: Commit**

```
git add src/libs/scene/include/opengeolab/scene/scene_module.hpp \
        src/libs/scene/src/scene_module.cpp \
        src/libs/scene/test/scene_module_test.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): add SceneModule owning SceneGraph

SceneModule registers set_visibility and list_nodes actions.
Bridges SceneGraph signals to ModuleBase::dataChanged for event bus.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: Register SceneModule in command dispatcher

**Files:**
- Modify: `src/libs/command/src/module_registry.cpp`
- Modify: `src/libs/command/CMakeLists.txt`

- [ ] **Step 1: Add Scene link dependency to command CMakeLists.txt**

In `src/libs/command/CMakeLists.txt`, add `OpenGeoLab::Scene` to `PUBLIC_LINKS`:

Change:
```cmake
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::IO
    OpenGeoLab::Geometry)
```
To:
```cmake
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::IO
    OpenGeoLab::Geometry
    OpenGeoLab::Scene)
```

- [ ] **Step 2: Register SceneModule in module_registry.cpp**

Add include and registration to `src/libs/command/src/module_registry.cpp`:

Add include:
```cpp
#include <opengeolab/scene/scene_module.hpp>
```

Add registration block after the Geometry registration:
```cpp
    if(!is_registered(Scene::SceneModule::MODULE_NAME)) {
        factory.bindSingleton<Core::ModuleBase, Scene::SceneModule>(
            Scene::SceneModule::MODULE_NAME, std::ref(factory));
        LOG_INFO("Registered module '{}'", Scene::SceneModule::MODULE_NAME);
    }
```

- [ ] **Step 3: Build the full project**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`

Expected: Full build succeeds. All existing tests still pass.

- [ ] **Step 4: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

Expected: All tests pass including the new scene_module tests.

- [ ] **Step 5: Commit**

```
git add src/libs/command/CMakeLists.txt src/libs/command/src/module_registry.cpp
git commit -m "build(command): register SceneModule in builtin modules

Adds OpenGeoLab::Scene link and registers scene module at startup.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: ModuleDataNotifier — add sceneDataChanged signal

**Files:**
- Modify: `src/app/include/opengeolab/app/module_data_notifier.h`
- Modify: `src/app/src/module_data_notifier.cpp`

- [ ] **Step 1: Add sceneDataChanged signal to header**

In `src/app/include/opengeolab/app/module_data_notifier.h`, add to Q_SIGNALS:

```cpp
Q_SIGNALS:
    /** @brief Emitted on main thread when geometry module data changes. */
    void geometryDataChanged();

    /** @brief Emitted on main thread when scene module data changes. */
    void sceneDataChanged();
```

- [ ] **Step 2: Subscribe to scene module events in implementation**

In `src/app/src/module_data_notifier.cpp`, add a second subscription inside the constructor, after the existing geometry subscription:

```cpp
    auto scene_handle =
        dispatcher.onModuleDataChanged("scene", [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::sceneDataChanged,
                                      Qt::QueuedConnection);
        });
    if(scene_handle.isConnected()) {
        m_connections.push_back(std::move(scene_handle));
    }
```

- [ ] **Step 3: Build the app target**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`

Expected: Build succeeds (MOC picks up the new signal).

- [ ] **Step 4: Commit**

```
git add src/app/include/opengeolab/app/module_data_notifier.h \
        src/app/src/module_data_notifier.cpp
git commit -m "feat(app): add sceneDataChanged signal to ModuleDataNotifier

Subscribes to scene module data-change events and emits sceneDataChanged
on the main thread via QueuedConnection.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: Refactor main.cpp — SceneGraph from SceneModule

**Files:**
- Modify: `src/app/src/main.cpp`

- [ ] **Step 1: Update main.cpp to get SceneGraph from SceneModule**

In `src/app/src/main.cpp`:

1. Add include:
```cpp
#include <opengeolab/scene/scene_module.hpp>
```

2. Remove the standalone `SceneGraph scene_graph;` declaration (line 76).

3. After `CommandDispatcher dispatcher(factory);` (line 73), add:
```cpp
    auto scene_module_ptr = dispatcher.findModule("scene");
    auto* scene_module =
        dynamic_cast<OpenGeoLab::Scene::SceneModule*>(scene_module_ptr.get());
```

4. Update the `GeometrySceneBridge` construction to use `scene_module->sceneGraph()`:
```cpp
    if(geometry_module != nullptr && scene_module != nullptr) {
        scene_bridge = std::make_unique<OpenGeoLab::Scene::GeometrySceneBridge>(
            scene_module->sceneGraph(), geometry_module->shapeStore(), topology_index);
    }
```

5. Update `viewport->setSceneGraph()` to use `scene_module->sceneGraph()`:
```cpp
        viewport->setSceneGraph(&scene_module->sceneGraph());
```

6. Add sceneDataChanged → viewport update connection after the existing geometryDataChanged connection:
```cpp
        QObject::connect(&module_notifier,
                         &OpenGeoLab::App::ModuleDataNotifier::sceneDataChanged, viewport,
                         [viewport]() { viewport->update(); });
```

- [ ] **Step 2: Build the full project**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`

Expected: Build succeeds.

- [ ] **Step 3: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

Expected: All tests pass.

- [ ] **Step 4: Commit**

```
git add src/app/src/main.cpp
git commit -m "refactor(app): get SceneGraph from SceneModule instead of local

SceneModule now owns the SceneGraph. main.cpp retrieves it via
dispatcher.findModule('scene') and passes references to viewport
and bridge. Also connects sceneDataChanged to viewport update.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: Remove GLViewport.setShapeVisible

**Files:**
- Modify: `src/app/include/opengeolab/app/gl_viewport.hpp`
- Modify: `src/app/src/gl_viewport.cpp`

- [ ] **Step 1: Remove setShapeVisible from header**

In `src/app/include/opengeolab/app/gl_viewport.hpp`, remove the `setShapeVisible` declaration (lines 128-132):

```cpp
    // DELETE this block:
    /**
     * @brief Set the visibility of a scene node by its shape (node) id.
     * @param shapeId Integer node id in the scene graph.
     * @param visible Whether the shape should be visible.
     */
    Q_INVOKABLE void setShapeVisible(int shapeId, bool visible);
```

- [ ] **Step 2: Remove setShapeVisible from implementation**

In `src/app/src/gl_viewport.cpp`, remove the `setShapeVisible` implementation (lines 137-143):

```cpp
    // DELETE this block:
    void GLViewport::setShapeVisible(int shapeId, bool visible) {
        if(m_sceneGraph == nullptr) {
            return;
        }
        m_sceneGraph->setNodeVisible(static_cast<Scene::NodeId>(shapeId), visible);
        update();
    }
```

- [ ] **Step 3: Build the full project**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`

Expected: Build succeeds (QML hasn't been updated yet, but the symbol is no longer called from C++).

Note: The QML side still references `viewport.setShapeVisible` but this will be fixed in Task 8. If the build fails because of QML linkage, that's expected — the QML change is next.

- [ ] **Step 4: Commit**

```
git add src/app/include/opengeolab/app/gl_viewport.hpp src/app/src/gl_viewport.cpp
git commit -m "refactor(app): remove GLViewport.setShapeVisible

Visibility is now controlled through the scene.set_visibility command
via RequestService. Direct SceneGraph manipulation is no longer needed.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: Refactor SidebarPanel.qml — command-based visibility

**Files:**
- Modify: `src/app/resource/qml/sections/SidebarPanel.qml`
- Modify: `src/app/resource/qml/Main.qml`

- [ ] **Step 1: Refactor SidebarPanel.qml**

Replace the entire `SidebarPanel.qml` content with:

```qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../theme"
import "../components"

/// @brief Left sidebar — shape explorer with real-time list.
Item {
    id: root

    required property AppTheme theme

    property var shapeList: []
    property var nodeMap: ({})

    implicitWidth: 280

    function fetchShapeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "geometry",
            action: "list_shapes",
            param: {},
            mute: true
        }))
    }

    function fetchNodeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: "list_nodes",
            param: {},
            mute: true
        }))
    }

    function toggleGeoVisibility(shapeId) {
        let newVisible = !root.geoVisible(shapeId)

        // Optimistic local update — prevents stale reads on rapid double-click
        let map = root.nodeMap
        if (!(shapeId in map)) {
            map[shapeId] = { visible: newVisible, meshVisible: false }
        } else {
            map[shapeId].visible = newVisible
        }
        root.nodeMap = map

        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: "set_visibility",
            param: {
                nodes: [{ nodeId: shapeId, visible: newVisible }]
            },
            mute: true
        }))
    }

    function toggleMeshVisibility(shapeId) {
        // Mesh visibility is a local UI concern (no scene graph involvement yet)
        let map = root.nodeMap
        if (!(shapeId in map)) {
            map[shapeId] = { visible: true, meshVisible: false }
        }
        map[shapeId].meshVisible = !map[shapeId].meshVisible
        root.nodeMap = map
    }

    function geoVisible(shapeId) {
        if (shapeId in root.nodeMap && root.nodeMap[shapeId].visible !== undefined) {
            return root.nodeMap[shapeId].visible
        }
        return true
    }

    function meshVisible(shapeId) {
        return (shapeId in root.nodeMap)
               && root.nodeMap[shapeId].meshVisible === true
    }

    Timer {
        id: refreshTimer
        interval: 100
        repeat: false
        onTriggered: root.fetchShapeList()
    }

    Timer {
        id: sceneRefreshTimer
        interval: 100
        repeat: false
        onTriggered: root.fetchNodeList()
    }

    Connections {
        target: ModuleDataNotifier
        function onGeometryDataChanged() {
            refreshTimer.restart()
        }
        function onSceneDataChanged() {
            sceneRefreshTimer.restart()
        }
    }

    Connections {
        target: RequestService
        function onResponseReady(responseJson, muted) {
            try {
                const resp = JSON.parse(responseJson)
                if (resp.action === "list_shapes" && resp.ok) {
                    root.shapeList = resp.shapes || []
                }
                if (resp.action === "list_nodes" && resp.ok) {
                    let map = {}
                    let oldMap = root.nodeMap
                    for (let i = 0; i < resp.nodes.length; ++i) {
                        let n = resp.nodes[i]
                        map[n.nodeId] = {
                            visible: n.visible,
                            meshVisible: (n.nodeId in oldMap)
                                         ? oldMap[n.nodeId].meshVisible === true
                                         : false
                        }
                    }
                    root.nodeMap = map
                }
            } catch (e) {
                // Ignore non-JSON or unrelated responses
            }
        }
    }

    Component.onCompleted: {
        fetchShapeList()
        fetchNodeList()
    }

    SectionCard {
        anchors.fill: parent
        anchors.margins: 0
        theme: root.theme
        title: qsTr("Scene Explorer")
        subtitle: ""

        // Empty state
        Column {
            visible: root.shapeList.length === 0
            width: parent.width
            spacing: 8
            topPadding: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "\uD83D\uDCE6"
                font.pixelSize: 32
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No geometry loaded.")
                font.pixelSize: 13
                color: root.theme.textTertiary
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Create a shape or import a model to get started.")
                font.pixelSize: 12
                color: root.theme.textTertiary
                wrapMode: Text.WordWrap
            }
        }

        // Shape list
        ListView {
            visible: root.shapeList.length > 0
            width: parent.width
            height: contentHeight
            model: root.shapeList
            clip: true
            spacing: 2
            interactive: false

            delegate: ShapeListItem {
                required property var modelData
                required property int index

                width: ListView.view.width
                theme: root.theme
                shapeId: modelData.shapeId ?? 0
                name: modelData.name ?? ""
                shapeType: modelData.shapeType ?? "Shape"
                hasTessellation: modelData.hasTessellation ?? false
                topology: modelData.topology ?? {}
                boundingBox: modelData.boundingBox ?? {}
                shapeColor: modelData.color ?? ""
                geoVisible: root.geoVisible(modelData.shapeId ?? 0)
                meshVisible: root.meshVisible(modelData.shapeId ?? 0)

                onToggleGeoVisibility: (sid) => root.toggleGeoVisibility(sid)
                onToggleMeshVisibility: (sid) => root.toggleMeshVisibility(sid)
            }
        }
    }
}
```

- [ ] **Step 2: Remove viewport property from SidebarPanel in Main.qml**

In `src/app/resource/qml/Main.qml`, find the SidebarPanel instantiation (around line 215-219):

```qml
                        SidebarPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: appTheme
                            viewport: viewportPanel.glViewport
                        }
```

Remove the `viewport: viewportPanel.glViewport` line:

```qml
                        SidebarPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: appTheme
                        }
```

- [ ] **Step 3: Build the full project**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`

Expected: Build succeeds.

- [ ] **Step 4: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

Expected: All tests pass.

- [ ] **Step 5: Commit**

```
git add src/app/resource/qml/sections/SidebarPanel.qml \
        src/app/resource/qml/Main.qml
git commit -m "refactor(app): SidebarPanel uses command for visibility

Removes local visibilityState and viewport dependency.
Visibility toggle now goes through RequestService → scene.set_visibility.
Visibility state read from scene.list_nodes response.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 9: Final validation

- [ ] **Step 1: Full build**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`

Expected: Clean build, no warnings.

- [ ] **Step 2: Full test suite**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

Expected: All tests pass.

- [ ] **Step 3: Verify new tests are discovered**

Run: `ctest --test-dir build -C RelWithDebInfo -R scene_module -N`

Expected: Lists the scene_module_test executable with all test cases.

- [ ] **Step 4: clang-format check**

Run clang-format on all new/modified C++ files:
```
clang-format -i src/libs/scene/include/opengeolab/scene/scene_module.hpp \
    src/libs/scene/include/opengeolab/scene/set_visibility_action.hpp \
    src/libs/scene/include/opengeolab/scene/list_nodes_action.hpp \
    src/libs/scene/src/scene_module.cpp \
    src/libs/scene/src/set_visibility_action.cpp \
    src/libs/scene/src/list_nodes_action.cpp \
    src/libs/scene/test/scene_module_test.cpp \
    src/libs/command/src/module_registry.cpp \
    src/app/include/opengeolab/app/module_data_notifier.h \
    src/app/src/module_data_notifier.cpp \
    src/app/src/main.cpp \
    src/app/include/opengeolab/app/gl_viewport.hpp \
    src/app/src/gl_viewport.cpp
```

If any changes, commit them:
```
git add -u
git commit -m "style: apply clang-format to scene module files

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
