# OpenGeoLab JSON 协议说明

本文档整理了当前工程中 QML ↔ C++ 的 JSON 通信协议（基于 `BackendService.request()`）。

## 1. 总体约定

### 1.1 调用入口（QML）
- API：`BackendService.request(moduleName, jsonString)`
- `moduleName`：服务名（当前已注册：`RenderService` / `GeometryService` / `ReaderService`）
- `jsonString`：JSON 字符串（建议始终传对象）

### 1.2 action 约定
- 大多数服务使用 `params.action` 作为路由字段。
- `BackendService` 会从 `params.action` 提取 `actionName`，并在 `operationFinished(module, actionName, result)` / `operationFailed(module, actionName, error)` 信号中回传。

### 1.3 _meta 扩展
当前支持：
- `_meta.silent: true|false`
  - `true`：同步执行，不弹进度 overlay，不刷日志（用于频繁 UI 操作，如视口控制）
  - `false/缺省`：走异步 worker 线程，可进度/可取消

---

## 2. RenderService

### 2.1 ViewPortControl
- 入口：`RenderService::processRequest()`
- action：`"ViewPortControl"`
- 用途：刷新场景/相机 Fit/视图预设（前后左右上下）

#### 2.1.1 Refresh
请求：
```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "refresh": true },
  "_meta": { "silent": true }
}
```
响应：
```json
{ "status": "success", "action": "ViewPortControl" }
```

### 2.2 SelectControl
- 入口：`RenderService::processRequest()`
- action：`"SelectControl"`
- 用途：控制视口拾取模式（开关/拾取类型），并获取或清空当前拾取结果集合。

#### 2.2.1 启用/关闭拾取模式
请求：
```json
{
  "action": "SelectControl",
  "select_ctrl": { "enabled": true },
  "_meta": { "silent": true }
}
```
响应：
```json
{ "status": "success", "action": "SelectControl" }
```

#### 2.2.2 设置拾取类型
说明：
- `vertex/edge/face` 可组合。
- `solid/part` 为互斥模式（且与 `vertex/edge/face` 互斥）。

请求（字符串数组）：
```json
{
  "action": "SelectControl",
  "select_ctrl": { "types": ["vertex", "edge", "face"] },
  "_meta": { "silent": true }
}
```

#### 2.2.3 获取当前拾取结果
请求：
```json
{
  "action": "SelectControl",
  "select_ctrl": { "get": true },
  "_meta": { "silent": true }
}
```
响应：
```json
{
  "status": "success",
  "action": "SelectControl",
  "pick_enabled": true,
  "pick_types": 7,
  "selections": [
    { "type": "Face", "uid": 123 }
  ]
}
```

#### 2.2.4 清空拾取结果
请求：
```json
{
  "action": "SelectControl",
  "select_ctrl": { "clear": true },
  "_meta": { "silent": true }
}
```

#### 2.1.2 Fit to Scene
请求：
```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "fit": true },
  "_meta": { "silent": true }
}
```
响应：
```json
{ "status": "success", "action": "ViewPortControl" }
```

#### 2.1.3 View Preset
`view_ctrl.view` 为整数枚举：
- 0: Front
- 1: Back
- 2: Left
- 3: Right
- 4: Top
- 5: Bottom

请求：
```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "view": 0 },
  "_meta": { "silent": true }
}
```
响应：
```json
{ "status": "success", "action": "ViewPortControl" }
```

---

## 3. GeometryService

### 3.1 new_model
- action：`"new_model"`
- 用途：新建空文档（清空当前模型）

请求：
```json
{ "action": "new_model" }
```
响应：
```json
{ "success": true, "message": "New model created successfully" }
```
失败响应示例：
```json
{ "success": false, "error": "Failed to create new document" }
```

### 3.2 get_part_list
- action：`"get_part_list"`
- 用途：获取文档中所有 Part 列表及统计信息（侧边栏展示）

请求（常见为 silent）：
```json
{
  "action": "get_part_list",
  "_meta": { "silent": true }
}
```
响应：
```json
{
  "success": true,
  "total_parts": 1,
  "parts": [
    {
      "id": 1,
      "uid": "...",
      "name": "box",
      "color": "#RRGGBBAA",
      "color_rgba": [0.1, 0.2, 0.3, 1.0],
      "entity_counts": {
        "faces": 6,
        "edges": 12,
        "vertices": 8,
        "solids": 1,
        "shells": 1,
        "wires": 0,
        "total": 28
      },
      "entity_ids": {
        "face_ids": [101, 102],
        "edge_ids": [201, 202],
        "vertex_ids": [301, 302]
      },
      "bounding_box": {
        "min": [0.0, 0.0, 0.0],
        "max": [10.0, 10.0, 10.0]
      }
    }
  ]
}
```

### 3.3 create
- action：`"create"`
- 用途：创建基础几何体并追加到当前文档（作为 Part）
- 公共字段：
  - `type`: `box|cylinder|sphere|cone`
  - `name`（可选）：Part 名称，缺省为 `type`

响应约定：
- 成功：返回 `success: true` 与新建 Part 的 `entity_id/entity_count/name/type`
- 失败：返回 `success: false` 与 `error` 字符串（该 action 通常不会抛异常）

#### 3.3.1 box
当前实现要求 **嵌套格式**：
- `dimensions: {x, y, z}`
- `origin: {x, y, z}`

请求：
```json
{
  "action": "create",
  "type": "box",
  "name": "Box-1",
  "dimensions": {"x": 10.0, "y": 20.0, "z": 30.0},
  "origin": {"x": 0.0, "y": 0.0, "z": 0.0}
}
```
响应：
```json
{
  "success": true,
  "entity_id": 1,
  "entity_count": 28,
  "name": "Box-1",
  "type": "box"
}
```


---

## 4. ReaderService

### 4.1 导入模型文件
- module：`"ReaderService"`
- action：`"load_model"`
- 字段：
  - `file_path`: string，本地文件路径（Windows 示例：`C:\\path\\model.step`）

请求：
```json
{ "action": "load_model", "file_path": "C:/models/demo.step" }
```
响应：
- 成功：
```json
{
  "success": true,
  "action": "load_model",
  "file_path": "C:/models/demo.step",
  "reader": "StepReader",
  "entity_count": 123
}
```
- 失败：抛异常并走 `operationFailed`（error 为字符串描述）

> 说明：ReaderService 会校验 `action == "load_model"`；其它 action 会返回失败（走 `operationFailed`）。

---

## 5. PickManager (QML Singleton)

### 5.1 概述
`PickManager` 是 QML 中用于几何实体拾取和选择的单例服务。它封装了 C++ 层的 `SelectManager`，提供 QML 友好的 API。

### 5.2 属性

| 属性名 | 类型 | 说明 |
|--------|------|------|
| `selectedType` | string | 当前拾取的实体类型（Vertex/Edge/Face/Solid/Part） |
| `pickModeActive` | bool | 拾取模式是否激活 |
| `selectedEntities` | array | 已选实体列表，格式：`[{type: "Face", uid: 123}, ...]` |
| `contextKey` | string | 选择上下文键，用于隔离不同 UI 面板的选择 |
| `expandPartSolidSelection` | bool | 是否将 Part/Solid 选择展开为所有下级 Face |

### 5.3 方法

#### 5.3.1 activatePickMode(entityType)
激活拾取模式。
- `entityType`: string - 要拾取的实体类型

```qml
PickManager.activatePickMode("Face")
```

#### 5.3.2 deactivatePickMode()
关闭拾取模式。

```qml
PickManager.deactivatePickMode()
```

#### 5.3.3 addSelection(entityType, entityUid)
添加实体到选择集。

```qml
PickManager.addSelection("Face", 123)
```

#### 5.3.4 removeSelection(entityType, entityUid)
从选择集移除实体。

```qml
PickManager.removeSelection("Face", 123)
```

#### 5.3.5 clearSelection()
清空所有选择。

```qml
PickManager.clearSelection()
```

#### 5.3.6 isSelected(entityType, entityUid)
检查实体是否被选中。

```qml
let selected = PickManager.isSelected("Face", 123)
```

#### 5.3.7 selectionCount()
获取已选实体数量。

```qml
let count = PickManager.selectionCount()
```

### 5.4 信号

| 信号名 | 参数 | 说明 |
|--------|------|------|
| `selectedTypeChanged` | - | 选择类型变化 |
| `pickModeActiveChanged` | - | 拾取模式状态变化 |
| `selectedEntitiesChanged` | - | 选择列表变化 |
| `contextKeyChanged` | - | 上下文键变化 |
| `expandPartSolidSelectionChanged` | - | 展开设置变化 |
| `pickModeChanged` | contextKey, enabled, entityType | 拾取模式变化（详细） |
| `selectionChanged` | contextKey, entities | 选择变化（详细） |
| `entityPicked` | contextKey, entityType, entityUid | 实体被拾取 |

### 5.5 选择行为

- **Vertex/Edge/Face**：多选模式，添加到现有选择
- **Solid/Part**：单选模式，当 `expandPartSolidSelection` 为 true 时，会清空现有选择并添加所有下级 Face 实体
