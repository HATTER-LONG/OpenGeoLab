# SceneNode sourceType/sourceId 外部标识统一 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 对外 API 统一使用 sourceId (shapeId/meshId/...) 而非内部 nodeId，`set_visibility` 增加 `type` 字段区分操作类型。

**Architecture:** SceneNode 增加 `sourceType` + `sourceId` 元数据字段；SceneGraph 提供按 source 查找方法；GeometrySceneBridge 创建节点时写入 `sourceType="geometry", sourceId=shapeId`；SetVisibilityAction 协议增加 `type` 字段路由查找逻辑；ListNodesAction 输出包含 source 信息；SidebarPanel 用 shapeId 直接操作。

**Tech Stack:** C++20, nlohmann/json, doctest, QML/JS

---

## 改动文件清单

| 操作 | 文件 | 职责 |
|------|------|------|
| Modify | `src/libs/scene/include/opengeolab/scene/scene_node.hpp` | 增加 sourceType/sourceId 字段及访问器 |
| Modify | `src/libs/scene/src/scene_node.cpp` | 实现 setSource/sourceType/sourceId |
| Modify | `src/libs/scene/include/opengeolab/scene/scene_graph.hpp` | 增加 findNodeBySource() |
| Modify | `src/libs/scene/src/scene_graph.cpp` | 实现 findNodeBySource() |
| Modify | `src/libs/scene/src/geometry_scene_bridge.cpp` | onShapeAdded/onShapeUpdated 写入 source |
| Modify | `src/libs/scene/include/opengeolab/scene/set_visibility_action.hpp` | 无签名变化，仅更新注释 |
| Modify | `src/libs/scene/src/set_visibility_action.cpp` | 协议增加 type 字段，按 source 查找 |
| Modify | `src/libs/scene/src/list_nodes_action.cpp` | 输出增加 sourceType/sourceId |
| Modify | `src/libs/scene/test/scene_module_test.cpp` | 更新所有测试适配新协议 |
| Modify | `src/app/resource/qml/sections/SidebarPanel.qml` | 发送 type:"geometry"，用 sourceId 关联 |

---

### Task 1: SceneNode 增加 sourceType / sourceId

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/scene_node.hpp`
- Modify: `src/libs/scene/src/scene_node.cpp`

- [ ] **Step 1: 在 scene_node.hpp 增加 source 相关声明**

在 `setVisible` / `isVisible` 附近（visibility 块之后）增加 source 访问器，在 private 成员变量区增加字段：

```cpp
// ---- 在 public 区域，setVisible/isVisible 之后、displayMode 之前 ----

/** @brief Source type tag (e.g. "geometry", "mesh"). Empty if unset. */
[[nodiscard]] std::string_view sourceType() const;

/** @brief Source-domain identifier (e.g. shapeId). Zero if unset. */
[[nodiscard]] uint32_t sourceId() const;

/**
 * @brief Set the source origin of this node.
 * @param type Domain tag, e.g. "geometry".
 * @param id   Domain-specific identifier, e.g. shapeId.
 */
void setSource(std::string type, uint32_t id);
```

在 private 成员变量区（`m_visible` 之后）增加：

```cpp
std::string m_sourceType;
uint32_t m_sourceId{0};
```

- [ ] **Step 2: 在 scene_node.cpp 实现 source 访问器**

```cpp
std::string_view SceneNode::sourceType() const { return m_sourceType; }

uint32_t SceneNode::sourceId() const { return m_sourceId; }

void SceneNode::setSource(std::string type, uint32_t id) {
    m_sourceType = std::move(type);
    m_sourceId = id;
}
```

- [ ] **Step 3: 构建验证**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: 编译成功

---

### Task 2: SceneGraph 增加 findNodeBySource()

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- Modify: `src/libs/scene/src/scene_graph.cpp`

- [ ] **Step 1: 在 scene_graph.hpp 声明 findNodeBySource**

在 `findNode(NodeId)` 声明之后：

```cpp
/**
 * @brief Find a node by source type and source id.
 * @param type  Source type tag (e.g. "geometry").
 * @param srcId Source-domain identifier (e.g. shapeId).
 * @return Pointer to node, or nullptr if not found.
 */
[[nodiscard]] SceneNode* findNodeBySource(std::string_view type, uint32_t srcId) const;
```

- [ ] **Step 2: 在 scene_graph.cpp 实现 findNodeBySource**

在 `findNode()` 实现之后：

```cpp
SceneNode* SceneGraph::findNodeBySource(std::string_view type, uint32_t srcId) const {
    std::shared_lock lock(m_mutex);
    SceneNode* result = nullptr;
    std::function<void(SceneNode*)> search = [&](SceneNode* node) {
        if(result != nullptr) {
            return;
        }
        if(node->sourceType() == type && node->sourceId() == srcId) {
            result = node;
            return;
        }
        for(const auto& child : node->children()) {
            search(child.get());
        }
    };
    search(m_root.get());
    return result;
}
```

- [ ] **Step 3: 构建验证**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: 编译成功

---

### Task 3: GeometrySceneBridge 写入 source

**Files:**
- Modify: `src/libs/scene/src/geometry_scene_bridge.cpp`

- [ ] **Step 1: onShapeAdded 设置 source**

在 `onShapeAdded()` 中，`m_shapeToNode[shapeId] = node->id();` 之后加一行：

```cpp
node->setSource("geometry", shapeId);
```

即：
```cpp
m_shapeToNode[shapeId] = node->id();
node->setSource("geometry", shapeId);
attachComponents(m_scene, *node, shapeId, entry);
```

- [ ] **Step 2: onShapeUpdated 中新建节点时也设置 source**

在 `onShapeUpdated()` 中，当创建新节点的分支（node == nullptr 且 entry.visualData != nullptr），在 `m_shapeToNode[shapeId] = new_node->id();` 之后加：

```cpp
new_node->setSource("geometry", shapeId);
```

- [ ] **Step 3: 构建验证**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: 编译成功

---

### Task 4: SetVisibilityAction 协议改造

**Files:**
- Modify: `src/libs/scene/src/set_visibility_action.cpp`

- [ ] **Step 1: 更新 describe()**

```cpp
nlohmann::json SetVisibilityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Batch-set visibility of scene nodes."},
        {"params",
         {{"type",
           {{"type", "string"},
            {"required", true},
            {"description", "Source type: \"geometry\", \"mesh\", or \"node\" (internal)."}}},
          {"nodes",
           {{"type", "array"},
            {"required", true},
            {"description",
             "Array of {id: int, visible: bool} pairs. "
             "\"id\" is interpreted according to \"type\": "
             "shapeId for \"geometry\", meshId for \"mesh\", nodeId for \"node\"."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"updated",
           {{"type", "integer"}, {"description", "Nodes whose visibility actually changed."}}},
          {"skipped",
           {{"type", "integer"},
            {"description", "Entries skipped (malformed or source not found)."}}}}}};
}
```

- [ ] **Step 2: 更新 execute() — 支持 type 路由**

```cpp
nlohmann::json SetVisibilityAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto type = param.value("type", std::string{});
    const auto& nodes = param.value("nodes", nlohmann::json::array());
    int updated = 0;
    int skipped = 0;

    for(const auto& entry : nodes) {
        if(!entry.contains("id")) {
            ++skipped;
            continue;
        }

        const auto id = entry.value("id", static_cast<uint32_t>(0));
        const auto visible = entry.value("visible", true);

        SceneNode* node = nullptr;
        if(type == "node") {
            node = m_graph.findNode(static_cast<NodeId>(id));
        } else {
            node = m_graph.findNodeBySource(type, id);
        }

        if(node == nullptr) {
            ++skipped;
            continue;
        }
        if(m_graph.setNodeVisible(node->id(), visible)) {
            ++updated;
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "set_visibility"}, {"updated", updated}, {"skipped", skipped}};
}
```

> **关键变化：**
> - JSON 字段从 `nodeId` → `id`
> - 新增 `type` 参数路由查找方式
> - `type=="node"` 走 `findNode()`，其余走 `findNodeBySource(type, id)`
> - `skipped` 描述修正为 "Entries skipped (malformed or source not found)."

- [ ] **Step 3: 构建验证**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: 编译成功

---

### Task 5: ListNodesAction 输出 source 信息

**Files:**
- Modify: `src/libs/scene/src/list_nodes_action.cpp`

- [ ] **Step 1: 更新 describe()**

```cpp
nlohmann::json ListNodesAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "List all scene nodes with visibility and source info."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"nodes",
           {{"type", "array"},
            {"description",
             "Array of {sourceType, sourceId, name, visible} objects."}}}}}};
}
```

- [ ] **Step 2: 更新 execute() — 输出 source 字段**

```cpp
nlohmann::json ListNodesAction::execute(const nlohmann::json& /*param*/,
                                        const Core::ProgressCallback& progress) {
    nlohmann::json nodes = nlohmann::json::array();

    m_graph.forEachNode([&](const SceneNode& node) {
        nodes.push_back({{"sourceType", std::string(node.sourceType())},
                         {"sourceId", node.sourceId()},
                         {"name", std::string(node.name())},
                         {"visible", node.isVisible()}});
    });

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "list_nodes"}, {"nodes", nodes}};
}
```

> **关键变化：**
> - 移除 `nodeId` 和 `parentId`（内部 ID 不再暴露）
> - 增加 `sourceType` 和 `sourceId`

- [ ] **Step 3: 构建验证**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: 编译成功

---

### Task 6: 更新测试

**Files:**
- Modify: `src/libs/scene/test/scene_module_test.cpp`

- [ ] **Step 1: 更新 SetVisibilityAction 测试**

所有 SetVisibility 测试需要：
1. 在创建节点后调用 `node->setSource("geometry", <id>)` 设置 source
2. 请求 JSON 从 `{"nodes": [{"nodeId": X, "visible": Y}]}` 改为 `{"type": "geometry", "nodes": [{"id": X, "visible": Y}]}`

```cpp
TEST_SUITE("SetVisibilityAction") {
    TEST_CASE("single node set invisible") {
        SceneGraph graph;
        auto* node = graph.addNode("A");
        REQUIRE(node != nullptr);
        node->setSource("geometry", 100);

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"id", 100}, {"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK(result["skipped"] == 0);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("batch set visibility") {
        SceneGraph graph;
        auto* a = graph.addNode("A");
        auto* b = graph.addNode("B");
        auto* c = graph.addNode("C");
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(c != nullptr);
        a->setSource("geometry", 10);
        b->setSource("geometry", 20);
        c->setSource("geometry", 30);

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "geometry"},
             {"nodes",
              {{{"id", 10}, {"visible", false}},
               {{"id", 20}, {"visible", false}},
               {{"id", 30}, {"visible", true}}}}},
            nullptr);

        CHECK(result["updated"] == 2);
        CHECK(result["skipped"] == 0);
        CHECK_FALSE(a->isVisible());
        CHECK_FALSE(b->isVisible());
        CHECK(c->isVisible());
    }

    TEST_CASE("source not found increments skipped") {
        SceneGraph graph;
        SetVisibilityAction action(graph);

        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"id", 999}, {"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["skipped"] == 1);
        CHECK(result["updated"] == 0);
    }

    TEST_CASE("missing id increments skipped") {
        SceneGraph graph;
        graph.addNode("A");

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["skipped"] == 1);
        CHECK(result["updated"] == 0);
    }

    TEST_CASE("empty nodes array succeeds") {
        SceneGraph graph;
        SetVisibilityAction action(graph);

        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", nlohmann::json::array()}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
        CHECK(result["skipped"] == 0);
    }

    TEST_CASE("no actual change yields updated=0") {
        SceneGraph graph;
        auto* node = graph.addNode("A");
        REQUIRE(node != nullptr);
        node->setSource("geometry", 42);
        CHECK(node->isVisible());

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "geometry"}, {"nodes", {{{"id", 42}, {"visible", true}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 0);
    }

    TEST_CASE("type=node uses internal nodeId") {
        SceneGraph graph;
        auto* node = graph.addNode("A");
        REQUIRE(node != nullptr);
        auto nid = node->id();

        SetVisibilityAction action(graph);
        auto result = action.execute(
            {{"type", "node"}, {"nodes", {{{"id", nid}, {"visible", false}}}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("describe returns valid schema") {
        SceneGraph graph;
        SetVisibilityAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "set_visibility");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("type"));
        CHECK(desc["params"].contains("nodes"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"]["ok"]["description"] ==
              "true when the action completes successfully.");
    }
}
```

- [ ] **Step 2: 更新 ListNodesAction 测试**

```cpp
TEST_SUITE("ListNodesAction") {
    TEST_CASE("empty scene returns no nodes") {
        SceneGraph graph;
        ListNodesAction action(graph);

        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "list_nodes");
        CHECK(result["nodes"].size() == 0);
    }

    TEST_CASE("lists multiple nodes with source info") {
        SceneGraph graph;
        auto* a = graph.addNode("Alpha");
        auto* b = graph.addNode("Beta");
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);
        a->setSource("geometry", 10);
        b->setSource("geometry", 20);

        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        auto& nodes = result["nodes"];
        CHECK(nodes.size() == 2);

        bool found_alpha = false;
        bool found_beta = false;
        for(const auto& n : nodes) {
            CHECK(n.contains("sourceType"));
            CHECK(n.contains("sourceId"));
            CHECK(n.contains("name"));
            CHECK(n.contains("visible"));
            if(n["name"] == "Alpha") {
                found_alpha = true;
                CHECK(n["sourceType"] == "geometry");
                CHECK(n["sourceId"] == 10);
                CHECK(n["visible"] == true);
            }
            if(n["name"] == "Beta") {
                found_beta = true;
                CHECK(n["sourceType"] == "geometry");
                CHECK(n["sourceId"] == 20);
                CHECK(n["visible"] == true);
            }
        }
        CHECK(found_alpha);
        CHECK(found_beta);
    }

    TEST_CASE("visibility reflected in list_nodes") {
        SceneGraph graph;
        auto* node = graph.addNode("X");
        REQUIRE(node != nullptr);
        node->setSource("geometry", 5);
        graph.setNodeVisible(node->id(), false);

        ListNodesAction action(graph);
        auto result = action.execute({}, nullptr);

        CHECK(result["nodes"].size() == 1);
        CHECK(result["nodes"][0]["visible"] == false);
    }

    TEST_CASE("describe returns valid schema") {
        SceneGraph graph;
        ListNodesAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "list_nodes");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"]["ok"]["description"] ==
              "true when the action completes successfully.");
    }
}
```

- [ ] **Step 3: 更新 SceneModule 集成测试**

```cpp
TEST_SUITE("SceneModule") {
    TEST_CASE("module name is scene") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        CHECK(mod.moduleName() == "scene");
    }

    TEST_CASE("sceneGraph accessor returns owned graph") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        auto* node = mod.sceneGraph().addNode("test");
        REQUIRE(node != nullptr);
        CHECK(mod.sceneGraph().findNode(node->id()) == node);
    }

    TEST_CASE("set_visibility dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        auto* node = mod.sceneGraph().addNode("A");
        REQUIRE(node != nullptr);
        node->setSource("geometry", 77);

        auto result = mod.process(
            {{"action", "set_visibility"},
             {"param", {{"type", "geometry"}, {"nodes", {{{"id", 77}, {"visible", false}}}}}}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["updated"] == 1);
        CHECK_FALSE(node->isVisible());
    }

    TEST_CASE("list_nodes dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        auto* node = mod.sceneGraph().addNode("B");
        REQUIRE(node != nullptr);
        node->setSource("geometry", 88);

        auto result = mod.process({{"action", "list_nodes"}, {"param", {}}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["nodes"].size() == 1);
        CHECK(result["nodes"][0]["sourceId"] == 88);
    }

    TEST_CASE("dataChanged emitted on set_visibility mutation") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        auto* node = mod.sceneGraph().addNode("C");
        REQUIRE(node != nullptr);
        node->setSource("geometry", 99);

        int signal_count = 0;
        auto conn = mod.dataChanged.connect(
            [&](Core::ModuleDataEvent) { ++signal_count; });

        mod.process(
            {{"action", "set_visibility"},
             {"param", {{"type", "geometry"}, {"nodes", {{{"id", 99}, {"visible", false}}}}}}},
            nullptr);

        CHECK(signal_count > 0);
    }
}
```

- [ ] **Step 4: 构建并运行测试**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo -R scene_module --output-on-failure`
Expected: 全部通过

---

### Task 7: SidebarPanel QML 适配

**Files:**
- Modify: `src/app/resource/qml/sections/SidebarPanel.qml`

- [ ] **Step 1: 更新 toggleGeoVisibility 发送 type 和 id**

```javascript
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
            type: "geometry",
            nodes: [{ id: shapeId, visible: newVisible }]
        },
        mute: true
    }))
}
```

- [ ] **Step 2: 更新 list_nodes 响应处理 — 使用 sourceId**

```javascript
if (resp.action === "list_nodes" && resp.ok) {
    let map = {}
    let oldMap = root.nodeMap
    for (let i = 0; i < resp.nodes.length; ++i) {
        let n = resp.nodes[i]
        let sid = n.sourceId
        map[sid] = {
            visible: n.visible,
            meshVisible: (sid in oldMap)
                         ? oldMap[sid].meshVisible === true
                         : false
        }
    }
    root.nodeMap = map
}
```

- [ ] **Step 3: 构建验证**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Expected: 编译成功

---

### Task 8: clang-format + 全量测试

- [ ] **Step 1: clang-format 所有改动文件**

```bash
clang-format -i \
  src/libs/scene/include/opengeolab/scene/scene_node.hpp \
  src/libs/scene/src/scene_node.cpp \
  src/libs/scene/include/opengeolab/scene/scene_graph.hpp \
  src/libs/scene/src/scene_graph.cpp \
  src/libs/scene/src/geometry_scene_bridge.cpp \
  src/libs/scene/src/set_visibility_action.cpp \
  src/libs/scene/src/list_nodes_action.cpp \
  src/libs/scene/test/scene_module_test.cpp
```

- [ ] **Step 2: 全量构建和测试**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: 全部通过（23+ tests）

- [ ] **Step 3: 提交（需用户确认）**

```
feat(scene): unify external API on sourceType/sourceId

SceneNode gains sourceType + sourceId metadata fields.
SceneGraph gains findNodeBySource() for source-based lookup.
GeometrySceneBridge writes sourceType="geometry", sourceId=shapeId.

set_visibility protocol adds required "type" field ("geometry",
"mesh", "node") and uses "id" instead of "nodeId".

list_nodes output replaces nodeId/parentId with sourceType/sourceId.

SidebarPanel sends type:"geometry" and correlates by sourceId.

Fixes: set_visibility had no effect because QML sent shapeId as
nodeId, which are different ID spaces.
```
