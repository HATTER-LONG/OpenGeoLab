# OpenGeoLab JSON 协议说明

本文档整理了当前工程中 QML ↔ C++ 的 JSON 通信协议（基于 `BackendService.request()`）。

## 1. 总体约定

### 1.1 调用入口（QML）
- API：`BackendService.request(moduleName, jsonString)`
- `moduleName`：服务名（当前已注册：`RenderService` / `GeometryService` / `ReaderService` / `MeshService`）
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
- `mesh_node/mesh_element` 可组合，用于拾取网格节点/单元。

请求（字符串数组）：
```json
{
  "action": "SelectControl",
  "select_ctrl": { "types": ["vertex", "edge", "face"] },
  "_meta": { "silent": true }
}
```

网格拾取示例：
```json
{
  "action": "SelectControl",
  "select_ctrl": { "types": ["mesh_node", "mesh_element"] },
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
      "uid": 1,
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


### 3.4 query_entity_info
- action：`"query_entity_info"`
- 用途：根据 QML 拾取返回的 (uid,type) 列表，查询实体的详细信息（用于 Geo Query 页面）。

请求：
```json
{
  "action": "query_entity_info",
  "entities": [
    { "type": "Face", "uid": 123 },
    { "type": "Edge", "uid": 45 }
  ]
}
```

响应：
```json
{
  "success": true,
  "total": 2,
  "entities": [
    {
      "type": "Face",
      "uid": 123,
      "id": 1001,
      "name": "",
      "bounding_box": {
        "min": [0.0, 0.0, 0.0],
        "max": [10.0, 10.0, 10.0]
      },
      "related": {
        "parts": [{"id": 1, "uid": 1, "type": "Part"}],
        "solids": [{"id": 2, "uid": 1, "type": "Solid"}],
        "wires": [],
        "faces": [],
        "edges": [],
        "vertices": [],
        "shells": []
      }
    }
  ],
  "not_found": [
    { "type": "Edge", "uid": 45 }
  ]
}
```

失败响应示例：
```json
{ "success": false, "error": "Missing or invalid 'entities' array" }
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

## 5. MeshService

### 5.1 generate_mesh
- action：`"generate_mesh"`
- 用途：根据所选几何面实体，通过 Gmsh 生成 2D 网格（节点+单元），存储到 MeshDocument 中。生成完成后自动触发场景刷新。

请求：
```json
{
  "action": "generate_mesh",
  "entities": [
    { "uid": 5, "type": "Face" }
  ],
  "elementSize": 1.0,
  "meshDimension": 2,
  "elementType": "triangle"
}
```

字段说明：
- `entities`（必须）：要剖分的实体列表，每项含 `uid`（EntityUID）和 `type`（实体类型字符串，如 `"Face"`）
- `elementSize`（可选）：全局网格尺寸，默认 `1.0`，必须为正数
- `meshDimension`（可选）：网格维度，`2` 为表面网格，`3` 为体网格，默认 `2`
- `elementType`（可选）：单元类型，`"triangle"` / `"quad"` / `"auto"`，默认 `"triangle"`
  - `"triangle"`：使用 Frontal-Delaunay 算法（Gmsh Algorithm 6）
  - `"quad"`：使用 Packing of Parallelograms 算法（Gmsh Algorithm 8）+ RecombineAll
  - `"auto"`：自动选择（当前默认 Frontal-Delaunay）

响应（成功）：
```json
{
  "success": true,
  "nodeCount": 256,
  "elementCount": 480
}
```

响应（失败）示例：
```json
{ "success": false, "error": "Missing or invalid 'entities' array" }
```

可能的失败原因：
- 参数不是 JSON 对象
- 缺少或非法的 `entities` 数组
- 实体类型无效（`EntityType::None`）
- 未提供有效实体
- `elementSize` 为非正数
- Gmsh 导入/剖分失败
- 操作被取消

### 5.2 query_mesh_entity_info
- action：`"query_mesh_entity_info"`
- 用途：根据 QML 拾取返回的 (uid, type) 列表，查询网格节点/单元的详细信息（用于 Mesh Query 页面）。

请求：
```json
{
  "action": "query_mesh_entity_info",
  "entities": [
    { "uid": 42, "type": "MeshNode" },
    { "uid": 7, "type": "MeshElement" }
  ]
}
```

字段说明：
- `entities`（必须）：要查询的网格实体列表，每项含 `uid`（整数）和 `type`（`"MeshNode"` 或 `"MeshElement"`）

响应（成功）：
```json
{
  "success": true,
  "total": 2,
  "entities": [
    {
      "type": "MeshNode",
      "nodeId": 42,
      "position": [1.0, 2.0, 3.0],
      "adjacentElements": [
        { "elementId": 1, "elementUID": 1, "elementType": "Triangle" }
      ],
      "adjacentNodes": [
        { "nodeId": 43, "position": [1.5, 2.5, 3.5] }
      ]
    },
    {
      "type": "MeshElement",
      "elementId": 7,
      "elementUID": 7,
      "elementType": "Triangle",
      "nodeCount": 3,
      "nodes": [
        { "nodeId": 1, "position": [0.0, 0.0, 0.0] },
        { "nodeId": 2, "position": [1.0, 0.0, 0.0] },
        { "nodeId": 3, "position": [0.5, 1.0, 0.0] }
      ],
      "adjacentElements": [
        { "elementId": 8, "elementUID": 8, "elementType": "Triangle" }
      ]
    }
  ],
  "not_found": []
}
```

响应（失败）示例：
```json
{ "success": false, "error": "Missing or invalid 'entities' array" }
```

可能的失败原因：
- 参数不是 JSON 对象
- 缺少或非法的 `entities` 数组
- 实体句柄缺少 `uid` 或 `type` 字段
- 网格文档未加载
- 操作被取消
