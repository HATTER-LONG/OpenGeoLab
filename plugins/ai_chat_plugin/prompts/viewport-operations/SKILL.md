---
name: viewport-operations
description: Use when operating OpenGeoLab 3D viewport via scene actions: capture_viewport, camera, selection, labels, visibility, pick_area; enforce describe_action then execute_action with exact schema
---

# Viewport Operations

## 目标与边界

本 skill 仅覆盖 `scene` 模块的视口相关操作（共 20 个 action），用于让模型稳定完成：

- 视口观察与截图
- 相机控制
- 选择与框选
- 标签管理
- 可见性与场景重置

统一请求包络（底层 pywrapper 协议）：

```json
{"module":"scene","action":"<action_name>","param":{...}}
```

工具调用约定：

- `describe_action(module_name, action_name)`：先拿 schema
- `execute_action(module, action, params)`：再执行

## 强制执行流程

每次调用 action 都遵循以下顺序，不跳步：

1. 明确目标 action
2. 先调用 `describe_action("scene", action)` 获取最新 `params` / `returns`
3. 按 schema 组装参数（字段名、类型、必填严格一致）
4. 调用 `execute_action("scene", action, params)`
5. 校验返回：
   - `ok == true` 才进入下一步
   - `ok == false` 时读取 `summary` 或 `error`，修正参数后重试

## 参数名陷阱（高频错误）

实体 3 元组概念是一样的：`shapeId + type/entityType + localId`，但字段名不统一。

| 场景 | 正确字段名 |
|---|---|
| `select` / `deselect` / `query_selection` / `set_hover` / `capture_viewport.metadata.selections` | `type` |
| `add_label` / `remove_label` / `describe_labels.labels` | `entityType` |

有效的实体类型值（`type` 和 `entityType` 共用）：

| 类别 | 值 |
|------|-----|
| 几何拓扑 | `GeoVertex`、`GeoEdge`、`GeoWire`、`GeoFace`、`GeoSolid` |
| 网格 | `MeshNode`、`MeshEdge`、`MeshElement` |

⚠️ **实体类型必须精确匹配**：
- 从 `query_selection` 返回的 `type` 和从 `describe_labels` 返回的 `entityType` 是同一套值
- 执行 `deselect` / `select` 时，`type` 值必须与当前选中实体的实际类型完全一致
- 常见错误：标签/选择显示 `MeshElement`，却用 `GeoFace` 去 deselect → `removed: 0`
- **正确做法**：先 `query_selection` 查到精确的 `type`，再原样传给 `deselect`

字段映射速查（可直接复制）：

```json
{
   "selectionEntity": {"shapeId": 0, "type": "GeoFace", "localId": 3},
   "labelEntity": {"shapeId": 0, "entityType": "GeoFace", "localId": 3}
}
```

ID 规则：

- `shapeId`：来自 geometry 创建/导入结果，起始通常为 0
- `localId`：拓扑局部编号，1-based，不要传 0

## Scene Action 清单（20）

### 观察与查询

- `capture_viewport`
- `list_nodes`
- `query_selection`
- `describe_labels`

### 相机

- `set_camera`
- `set_view_preset`
- `fit_to_scene`

### 选择

- `select`
- `deselect`
- `clear_selection`
- `pick_area`
- `set_pick_mode`
- `set_hover`

### 标签

- `add_label`
- `remove_label`
- `clear_labels`
- `set_labels_visible`
- `set_auto_label`

### 场景管理

- `set_visibility`
- `new_model`

## 关键 action 语义

### capture_viewport

- 默认：`width=1024`，`height=768`，`includeMetadata=true`，`captureImage=true`
- `outputPath`：可选绝对路径，渲染线程直接写入 PNG 文件
  - 成功时返回 `savedPath`；失败时返回 `savedPathError`
  - 可与 `captureImage` 独立使用（`captureImage=false` + `outputPath` = 只写文件不返回 base64）
- `metadata` 含：`viewport`、`camera`、`visibleShapes`、`selections`、`labels`、`hover`
- `captureImage=true` 时，若 5 秒内渲染线程未返回：`image=null` 且带 `imageError`
- `includeMetadata=false` 且 `captureImage=false` 且无 `outputPath` 时，仅返回最小成功结果

### pick_area（异步）

- 返回 `async: true` 仅表示已排队，不代表选择已完成
- 正确链路：`pick_area` 后再 `query_selection`
- `coordType`：`normalized`（默认）或 `pixel`
- `pickAction`：`Add`（默认）或 `Remove`

### set_visibility

- 参数是 `type + nodes[]`
- `type` 是源类别：`geometry` / `mesh` / `node`
- `nodes[]` 每项是 `{id, visible}`，`id` 语义随 `type` 变化

## 推荐任务模板

### 看当前视图里有什么

1. `describe_action("scene","capture_viewport")`
2. `execute_action("scene","capture_viewport", {"width":1024,"height":768})`
3. 从 `metadata.visibleShapes`、`metadata.selections`、`metadata.labels` 解释视图

### 保存视口截图到文件

1. `describe_action("scene","capture_viewport")`
2. `execute_action("scene","capture_viewport", {"captureImage":false,"outputPath":"C:/path/to/output.png"})`
3. 检查返回 `savedPath` 确认写入成功

### 选择一个面并打标签

1. `describe_action("scene","select")`
2. `execute_action("scene","select", {"entities":[{"shapeId":0,"type":"GeoFace","localId":3}],"append":false})`
3. `describe_action("scene","add_label")`
4. `execute_action("scene","add_label", {"shapeId":0,"entityType":"GeoFace","localId":3})`
5. `execute_action("scene","capture_viewport", {})` 复核结果

### 框选区域

1. `describe_action("scene","pick_area")`
2. `execute_action("scene","pick_area", {"x0":0.2,"y0":0.3,"x1":0.8,"y1":0.7,"coordType":"normalized"})`
3. `execute_action("scene","query_selection", {})` 获取最终选中实体

## 完成判定

只有同时满足以下条件，才算任务完成：

1. 目标 action 全部 `ok=true`
2. 关键返回字段存在且结构正确（例如 `selections[]`、`labels[]`、`visibleShapes[]`）
3. 涉及异步选择时，已做二次查询（`query_selection`）
4. 所有任务最终都已用 `capture_viewport` 二次确认

## 失败恢复

当 `ok=false`：

1. 先读 `summary`/`error`
2. 重新 `describe_action` 校验参数名和类型
3. 修正后重试，避免盲目改动多个字段

禁止行为：

- 跳过 `describe_action` 直接猜参数
- 混用 `type` 与 `entityType`
- 对 `pick_area` 的返回直接当作最终选择结果
