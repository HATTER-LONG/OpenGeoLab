# Scene Module Design Spec

## Problem

Visibility control currently bypasses the Command/Action system:

```
QML SidebarPanel → GLViewport.setShapeVisible() → SceneGraph.setNodeVisible()
```

This means:
- **Python has no access** to visibility control
- **No undo/redo** possible for visibility changes
- **No event notification** — sidebar maintains a local `visibilityState` JS object (dual source of truth)
- **No batch support** — single node per call only

## Solution

Create a **SceneModule** (`scene`) that owns the SceneGraph and exposes visibility
(and future scene operations) through the standard Command/Action protocol.

All callers — QML and Python — go through `CommandDispatcher` using the same JSON envelope.

## Architecture

### Ownership Change

SceneModule **owns** SceneGraph (analogous to GeometryModule owning ShapeStore):

```
Before:  main.cpp creates SceneGraph → passes reference to GLViewport, Bridge
After:   SceneModule owns SceneGraph → exposes sceneGraph() accessor
         main.cpp gets SceneGraph& from SceneModule
```

This follows the established pattern where modules are the authority over their data.

### Call Flow (Unified)

```
QML:    RequestService.submitAsync(JSON) → CommandDispatcher → SceneModule → SceneGraph
Python: opengeolab_pywrapper.process(JSON) → CommandDispatcher → SceneModule → SceneGraph
```

### Notification Flow

```
SceneModule action mutates SceneGraph
  → SceneModule emits dataChanged(ItemModified)
  → CommandDispatcher event bus
  → ModuleDataNotifier.sceneDataChanged() [Qt QueuedConnection → main thread]
  → SidebarPanel: debounce timer → re-query scene.list_nodes
  → GLViewport: update() → repaint
```

## SceneModule

**Location:** `src/libs/scene/`  
**Namespace:** `OpenGeoLab::Scene`  
**Module name:** `"scene"`

```cpp
class SceneModule final : public Core::ModuleBase {
public:
    explicit SceneModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~SceneModule() override;

    [[nodiscard]] SceneGraph& sceneGraph();
    [[nodiscard]] const SceneGraph& sceneGraph() const;

    static constexpr std::string_view MODULE_NAME{"scene"};

private:
    SceneGraph m_sceneGraph;
    std::vector<Kangaroo::Util::ScopedConnection> m_graphConnections;
};
```

SceneModule connects SceneGraph signals to `dataChanged`:
- `nodeAdded` → `dataChanged(ItemAdded)`
- `nodeRemoved` → `dataChanged(ItemRemoved)`
- `nodeUpdated` → `dataChanged(ItemModified)`

## Actions

### `scene.set_visibility`

Batch-set node visibility.

**Request:**
```json
{
  "module": "scene",
  "action": "set_visibility",
  "param": {
    "nodes": [
      { "nodeId": 1, "visible": false },
      { "nodeId": 3, "visible": true }
    ]
  }
}
```

**Response (success):**
```json
{
  "ok": true,
  "action": "set_visibility",
  "updated": 2,
  "skipped": 0
}
```

**Behavior:**
- Iterates `nodes` array; for each entry:
  1. `findNode(nodeId)` — if nullptr → increment `skipped`
  2. If found and `isVisible() != visible` → call `setNodeVisible()` → increment `updated`
  3. If found and already at target → no-op (neither updated nor skipped)
- `updated` counts nodes whose visibility actually changed
- `skipped` counts nodes not found in the scene graph
- Empty `nodes` array succeeds with `updated: 0`
- Does **not** fail on individual node-not-found; reports it in `skipped`

**Implementation:**

```cpp
class SetVisibilityAction final : public Core::IAction {
public:
    static constexpr std::string_view ACTION_NAME{"set_visibility"};

    explicit SetVisibilityAction(SceneGraph& graph);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;
private:
    SceneGraph& m_graph;
};
```

### `scene.list_nodes`

Query all scene nodes with visibility state.

**Request:**
```json
{
  "module": "scene",
  "action": "list_nodes",
  "param": {}
}
```

**Response:**
```json
{
  "ok": true,
  "action": "list_nodes",
  "nodes": [
    { "nodeId": 1, "name": "Box_1", "visible": true, "parentId": 0 },
    { "nodeId": 2, "name": "Cylinder_1", "visible": false, "parentId": 0 }
  ]
}
```

**Behavior:**
- Traverses all non-root nodes (root id=0 is excluded)
- Returns `nodeId`, `name`, `visible`, `parentId` for each
- Uses `SceneGraph::readLock()` for thread safety
- Root node is an implementation detail, not exposed to callers

**Implementation:**

```cpp
class ListNodesAction final : public Core::IAction {
public:
    static constexpr std::string_view ACTION_NAME{"list_nodes"};

    explicit ListNodesAction(const SceneGraph& graph);

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;
private:
    const SceneGraph& m_graph;
};
```

## Integration Changes

### ModuleDataNotifier

Add `sceneDataChanged()` signal, subscribe to `"scene"` module events:

```cpp
// module_data_notifier.h — add signal
Q_SIGNALS:
    void geometryDataChanged();  // existing
    void sceneDataChanged();     // new

// module_data_notifier.cpp — add subscription
auto scene_handle =
    dispatcher.onModuleDataChanged("scene", [this](Core::ModuleDataEvent) {
        QMetaObject::invokeMethod(this, &ModuleDataNotifier::sceneDataChanged,
                                  Qt::QueuedConnection);
    });
if (scene_handle.isConnected()) {
    m_connections.push_back(std::move(scene_handle));
}
```

### main.cpp

```cpp
// Before (current):
SceneGraph scene_graph;
registerBuiltinModules(factory);
CommandDispatcher dispatcher(factory);
auto* geometry_module = dynamic_cast<GeometryModule*>(dispatcher.findModule("geometry").get());
scene_bridge = make_unique<GeometrySceneBridge>(scene_graph, ...);
viewport->setSceneGraph(&scene_graph);

// After:
registerBuiltinModules(factory);          // now also registers SceneModule
CommandDispatcher dispatcher(factory);

auto* scene_module = dynamic_cast<SceneModule*>(dispatcher.findModule("scene").get());
auto* geometry_module = dynamic_cast<GeometryModule*>(dispatcher.findModule("geometry").get());

scene_bridge = make_unique<GeometrySceneBridge>(scene_module->sceneGraph(), ...);
viewport->setSceneGraph(&scene_module->sceneGraph());

// Connect sceneDataChanged → viewport update
QObject::connect(&module_notifier,
                 &ModuleDataNotifier::sceneDataChanged, viewport,
                 [viewport]() { viewport->update(); });
```

### module_registry.cpp

Add SceneModule registration:

```cpp
#include <opengeolab/scene/scene_module.hpp>

if (!is_registered(Scene::SceneModule::MODULE_NAME)) {
    factory.bindSingleton<Core::ModuleBase, Scene::SceneModule>(
        Scene::SceneModule::MODULE_NAME, std::ref(factory));
}
```

### GLViewport

- **Remove** `setShapeVisible()` Q_INVOKABLE method
- **Remove** related includes and forward declarations if any
- GLViewport keeps `setSceneGraph()` — it still renders the graph directly

### SidebarPanel.qml

**Remove:**
- Local `visibilityState` property/object
- `toggleGeoVisibility()` that maintains local state + calls `viewport.setShapeVisible()`

**Add:**
- `Connections { target: ModuleDataNotifier; function onSceneDataChanged() { ... } }`
- Visibility toggle calls `RequestService.submitAsync()` with `scene.set_visibility` JSON
- Visibility state read from `scene.list_nodes` response (merged with `geometry.list_shapes` by nodeId)

Data merge strategy:
```
geometry.list_shapes  →  { nodeId, name, faceCount, edgeCount, ... }
scene.list_nodes      →  { nodeId, name, visible, parentId }
merge by nodeId       →  shape list items with visibility state
```

Both queries are triggered on their respective `dataChanged` signals. A geometry change
triggers shape list refresh; a scene change triggers node list refresh. The QML model
merges the two by nodeId.

## CMake Changes

### scene CMakeLists.txt

Add new source files:
- `include/opengeolab/scene/scene_module.hpp`
- `include/opengeolab/scene/set_visibility_action.hpp`
- `include/opengeolab/scene/list_nodes_action.hpp`
- `src/scene_module.cpp`
- `src/set_visibility_action.cpp`
- `src/list_nodes_action.cpp`

Add test:
- `test/scene_module_test.cpp` — tests set_visibility and list_nodes actions

### command CMakeLists.txt

Add `OpenGeoLab::Scene` to link dependencies (for module_registry.cpp to include scene_module.hpp).

## Testing

### Unit Tests (`scene_module_test.cpp`)

1. **set_visibility — single node** — create node, set invisible, verify
2. **set_visibility — batch** — create 3 nodes, set 2 invisible, verify counts
3. **set_visibility — node not found** — verify skipped count, no error
4. **set_visibility — empty array** — verify ok with updated=0
5. **set_visibility — no actual change** — set already-visible to visible, verify updated=0
6. **list_nodes — empty scene** — root only → empty nodes array
7. **list_nodes — multiple nodes** — verify nodeId, name, visible, parentId
8. **list_nodes — visibility reflected** — set invisible then list, verify visible=false
9. **dataChanged signal** — verify SceneModule emits dataChanged on set_visibility

### Integration (manual)

- QML toggle triggers visibility change and re-renders
- Python `process('{"module":"scene","action":"set_visibility",...}')` works
- SidebarPanel shows correct visibility state after toggle

## Out of Scope

- Undo/redo for visibility changes (future — requires command history infrastructure)
- Moving TopologyIndex into SceneModule (separate concern, can be done later)
- Node selection, hover, display mode actions (future actions, same pattern)
- Recursive visibility (hiding parent hides children) — current behavior is per-node
