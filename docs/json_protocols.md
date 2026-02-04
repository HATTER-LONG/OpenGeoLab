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
  - `type`: `box|cylinder|sphere|cone|torus`
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

#### 3.3.2 cylinder
圆柱体创建：
- `radius`: 半径
- `height`: 高度
- `x, y, z`（可选）：底面圆心位置，缺省为原点

请求：
```json
{
  "action": "create",
  "type": "cylinder",
  "name": "Cylinder-1",
  "radius": 5.0,
  "height": 10.0,
  "x": 0.0,
  "y": 0.0,
  "z": 0.0
}
```

#### 3.3.3 sphere
球体创建：
- `radius`: 半径
- `x, y, z`（可选）：球心位置，缺省为原点

请求：
```json
{
  "action": "create",
  "type": "sphere",
  "name": "Sphere-1",
  "radius": 5.0,
  "x": 0.0,
  "y": 0.0,
  "z": 0.0
}
```

#### 3.3.4 torus
圆环创建：
- `majorRadius`: 主半径（环中心到管道中心的距离）
- `minorRadius`: 次半径（管道半径）
- `x, y, z`（可选）：中心位置，缺省为原点

请求：
```json
{
  "action": "create",
  "type": "torus",
  "name": "Torus-1",
  "majorRadius": 10.0,
  "minorRadius": 3.0,
  "x": 0.0,
  "y": 0.0,
  "z": 0.0
}
```

### 3.4 query_entity
- action：`"query_entity"`
- 用途：查询实体详细信息

#### 3.4.1 单个实体查询
请求：
```json
{
  "action": "query_entity",
  "entity_id": 123
}
```
响应：
```json
{
  "success": true,
  "entity": {
    "id": 123,
    "uid": 45,
    "type": "Face",
    "type_enum": 4,
    "name": "Face_45",
    "parent_ids": [100],
    "child_ids": [200, 201],
    "owning_part_id": 1,
    "owning_part_name": "Box-1",
    "part_color": "#3498DB",
    "bounding_box": {
      "min": [0.0, 0.0, 0.0],
      "max": [10.0, 10.0, 10.0]
    },
    "center": [5.0, 5.0, 5.0],
    "size": [10.0, 10.0, 10.0]
  }
}
```

#### 3.4.2 批量实体查询
请求：
```json
{
  "action": "query_entity",
  "entity_ids": [123, 124, 125]
}
```
响应：
```json
{
  "success": true,
  "entities": [
    { "id": 123, "uid": 45, "type": "Face", ... },
    { "id": 124, "uid": 46, "type": "Face", ... }
  ],
  "queried_count": 2
}
```

---

## 4. 实体拾取 (Entity Picking)

### 4.1 概述
GLViewport 组件支持通过鼠标点击在3D视口中拾取几何实体。拾取功能将实体 UID 和类型编码到 OpenGL 颜色缓冲区中，实现精确的实体选择。

### 4.2 QML 属性接口

GLViewport 提供以下属性用于控制拾取模式：

| 属性 | 类型 | 说明 |
|------|------|------|
| `pickModeEnabled` | bool | 是否启用拾取模式 |
| `pickEntityType` | string | 实体类型过滤器（Vertex/Edge/Face/Solid/Part） |

### 4.3 QML 信号

| 信号 | 参数 | 说明 |
|------|------|------|
| `entityPicked` | (entityType: string, entityUid: int) | 拾取到实体时触发 |
| `pickCancelled` | 无 | 右键取消拾取时触发 |

### 4.4 拾取流程

1. **启用拾取模式**
   - 设置 `viewport.pickModeEnabled = true`
   - 设置 `viewport.pickEntityType = "Face"` （可选，过滤特定类型）

2. **用户交互**
   - 左键点击：在点击位置周围搜索匹配的实体（吸附半径 5 像素）
   - 右键点击：取消拾取模式

3. **接收拾取结果**
   ```qml
   Connections {
       target: viewport
       function onEntityPicked(entityType, entityUid) {
           console.log("Picked:", entityType, entityUid);
       }
       function onPickCancelled() {
           console.log("Pick cancelled");
       }
   }
   ```

### 4.5 Selector 组件集成

Selector 组件（`resources/qml/util/Selector.qml`）提供了完整的拾取 UI：

**属性：**
| 属性 | 类型 | 说明 |
|------|------|------|
| `visibleEntityTypes` | object | 控制显示哪些实体类型按钮 |
| `selectedEntities` | array | 已选择的实体列表 [{type, uid}, ...] |
| `pickModeActive` | bool | 当前是否处于拾取模式 |
| `selectedType` | string | 当前选择的实体类型 |

**示例：仅显示 Face 类型**
```qml
Selector {
    visibleEntityTypes: {
        "Vertex": false,
        "Edge": false,
        "Face": true,
        "Solid": false,
        "Part": false
    }
}
```

---

## 5. ReaderService

### 5.1 导入模型文件
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
