---
name: viewport-operations
description: OpenGeoLab 3D 视口操作完整参考：capture_viewport, camera, selection, labels, visibility, pick_area; enforce describe_action then execute_action with exact schema
---

# Viewport Operations

## 目标与边界

本 skill 覆盖 `scene` 模块全部 20 个 action，用于：

- 视口观察与截图
- 相机控制
- 选择与框选
- 标签管理
- 可见性与场景管理

工具调用约定：

- `describe_action("scene", action_name)`：先拿 schema
- `execute_action("scene", action, params)`：再执行

## 强制执行流程

每次调用 action 都遵循以下顺序，不跳步：

1. 明确目标 action
2. 先调用 `describe_action("scene", action)` 获取最新 schema
3. 按 schema 组装参数（字段名、类型、必填严格一致）
4. 调用 `execute_action("scene", action, params)`
5. 校验返回：`ok == true` 才进入下一步；`ok == false` 时读取 `summary`/`error`，修正后重试

## 参数名陷阱（高频错误）

| 使用场景 | 实体类型字段名 |
|----------|----------------|
| `select` / `deselect` / `query_selection` / `set_hover` | `type` |
| `add_label` / `remove_label` / `describe_labels` | `entityType` |

有效的实体类型值：

| 类别 | 值 |
|------|-----|
| 几何 | `GeoVertex`、`GeoEdge`、`GeoWire`、`GeoFace`、`GeoSolid` |
| 网格 | `MeshNode`、`MeshEdge`、`MeshElement` |

⚠️ **实体类型必须精确匹配**：先 `query_selection` 查到精确的 `type`，再原样传给 `deselect`。

字段映射速查：

```json
{
   "selectionEntity": {"shapeId": 0, "type": "GeoFace", "localId": 3},
   "labelEntity": {"shapeId": 0, "entityType": "GeoFace", "localId": 3}
}
```

ID 规则：

- `shapeId`：来自 geometry 创建/导入结果，0-based
- `localId`：拓扑局部编号，**1-based**，不要传 0

---

## 观察与查询（4 个）

### capture_viewport

捕获视口截图与结构化元数据。

| 参数 | 类型 | 必填 | 默认 | 说明 |
|------|------|------|------|------|
| `width` | integer | 否 | 1024 | 截图宽度（像素） |
| `height` | integer | 否 | 768 | 截图高度（像素） |
| `includeMetadata` | boolean | 否 | true | 包含结构化元数据 |
| `captureImage` | boolean | 否 | true | 返回 base64 图片（JSON 中） |
| `outputPath` | string | 否 | — | 绝对路径，PNG 直接写入文件 |

返回元数据结构：

```json
{
  "viewport": {"width": 1024, "height": 768},
  "camera": {"eye": [x,y,z], "target": [x,y,z], "up": [x,y,z]},
  "visibleShapes": [{"shapeId": 0, "name": "Box", "screenBBox": {"x": 100, "y": 50, "w": 200, "h": 150}}],
  "selections": [{"shapeId": 0, "type": "GeoFace", "localId": 3}],
  "labels": [{"shapeId": 0, "entityType": "GeoFace", "localId": 3, "text": "Face 3"}],
  "hover": null
}
```

行为说明：

- `outputPath` 成功时返回 `savedPath`；失败时返回 `savedPathError`
- `captureImage=false` + `outputPath` = 只写文件不返回 base64
- `captureImage=true` 时，若 5 秒内渲染线程未返回：`image=null` 且带 `imageError`
- `includeMetadata=false` + `captureImage=false` + 无 `outputPath` = 仅返回最小成功结果

### list_nodes

查询所有场景节点。无参数。

返回：`{ok, nodes: [{sourceType, sourceId, name, visible}]}`

### query_selection

返回当前选中实体。无参数。

返回：`{ok, selections: [{shapeId, type, localId}]}`

### describe_labels

查询所有标签状态。无参数。

返回：`{ok, labels: [{text, shapeId, entityType, localId, color}]}`

---

## 相机（3 个）

### set_camera

直接设置相机位置、目标和上方向。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `position` | array [x,y,z] | **是** | 相机眼点位置 |
| `target` | array [x,y,z] | **是** | 注视目标点 |
| `up` | array [x,y,z] | **是** | 上方向向量 |

返回：`{ok}`

### set_view_preset

应用预设视角。

| 参数 | 类型 | 必填 | 有效值 |
|------|------|------|--------|
| `preset` | string | **是** | `"Front"` / `"Back"` / `"Top"` / `"Bottom"` / `"Left"` / `"Right"` / `"Isometric"` |

返回：`{ok}`

### fit_to_scene

相机适配场景范围。无参数。

返回：`{ok}`

---

## 选择（6 个）

### select

添加实体到选择集。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `entities` | array | **是** | `[{shapeId, type, localId}]` |
| `append` | boolean | 否 | true=追加, false=替换当前选择（默认追加） |

返回：`{ok, selected: <数量>}`

### deselect

从选择集移除实体。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `entities` | array | **是** | `[{shapeId, type, localId}]` — type 必须精确匹配 |

返回：`{ok, removed: <数量>}`

### clear_selection

清空选择集。无参数。

返回：`{ok}`

### pick_area（异步）

框选区域内的实体。

| 参数 | 类型 | 必填 | 默认 | 说明 |
|------|------|------|------|------|
| `x0` | number | **是** | — | 框选起点 X |
| `y0` | number | **是** | — | 框选起点 Y |
| `x1` | number | **是** | — | 框选终点 X |
| `y1` | number | **是** | — | 框选终点 Y |
| `coordType` | string | 否 | `"normalized"` | `"normalized"` (0-1) 或 `"pixel"` |
| `pickAction` | string | 否 | `"Add"` | `"Add"` 或 `"Remove"`（首字母大写） |

⚠️ 返回 `{ok, async: true}` 仅表示已排队，**不代表选择已完成**。必须后续调用 `query_selection` 获取最终选择结果。

### set_pick_mode

设置拾取模式和掩码。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `pickMask` | integer | 否 | uint32 拾取掩码位域 |
| `enabled` | boolean | 否 | 启用/禁用拾取 |

返回：`{ok}`

### set_hover

设置或清除悬停高亮实体。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `entity` | object 或 null | 否 | `{shapeId, type, localId}` 设置悬停；`null` 或省略则清除 |

⚠️ 是**单个 `entity` 对象**（可选），不是 3 个独立参数。

返回：`{ok}`

---

## 标签（5 个）

### add_label

为实体创建或替换标签。标签文本**自动生成**（无 text 输入参数）。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `shapeId` | integer | **是** | 形状 ID |
| `entityType` | string | **是** | 实体类型（注意是 `entityType` 不是 `type`） |
| `localId` | integer | **是** | 子形状编号 |

返回：`{ok, text: "生成的标签文本"}`

### remove_label

移除实体标签。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `shapeId` | integer | **是** | 形状 ID |
| `entityType` | string | **是** | 实体类型 |
| `localId` | integer | **是** | 子形状编号 |

返回：`{ok}`

### clear_labels

清除所有标签。无参数。

返回：`{ok}`

### set_labels_visible

控制标签渲染显示。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `visible` | boolean | **是** | true=显示, false=隐藏 |

返回：`{ok}`

### set_auto_label

控制自动标签功能。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `enabled` | boolean | **是** | true=启用, false=禁用 |

返回：`{ok}`

---

## 场景管理（2 个）

### set_visibility

批量设置节点可见性。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `type` | string | **是** | 源类别：`"geometry"` / `"mesh"` / `"node"` |
| `nodes` | array | **是** | `[{id, visible}]` |

`id` 语义随 `type` 变化：
- `type="geometry"` → id 是 shapeId
- `type="mesh"` → id 是 meshId（通常等于 shapeId）
- `type="node"` → id 是内部 nodeId

返回：`{ok, updated: <数量>, skipped: <数量>}`

### new_model

重置工作区（清空所有几何体、网格、标签、选择）。无参数。

⚠️ **破坏性操作**，执行前应确认用户意图。

返回：`{ok}`

---

## 推荐任务模板

### 看当前视图里有什么

```
execute_action("scene", "capture_viewport", {})
→ 从 metadata.visibleShapes、metadata.selections、metadata.labels 解释视图
```

### 保存视口截图到文件

```
execute_action("scene", "capture_viewport", {"captureImage": false, "outputPath": "C:/path/output.png"})
→ 检查返回 savedPath 确认写入成功
```

### 选择一个面并打标签

```
execute_action("scene", "select", {"entities": [{"shapeId": 0, "type": "GeoFace", "localId": 3}], "append": false})
execute_action("scene", "add_label", {"shapeId": 0, "entityType": "GeoFace", "localId": 3})
execute_action("scene", "capture_viewport", {})  // 复核结果
```

### 框选区域

```
execute_action("scene", "pick_area", {"x0": 0.2, "y0": 0.3, "x1": 0.8, "y1": 0.7, "coordType": "normalized"})
execute_action("scene", "query_selection", {})  // 获取最终选中实体
```

### 隐藏一个几何体

```
execute_action("scene", "set_visibility", {"type": "geometry", "nodes": [{"id": 0, "visible": false}]})
```

### 切换相机视角

```
execute_action("scene", "set_view_preset", {"preset": "Front"})
// 或自定义：
execute_action("scene", "set_camera", {"position": [0, 0, 100], "target": [0, 0, 0], "up": [0, 1, 0]})
```

---

## 屏幕坐标系

- 原点 (0,0) 在**左上角**
- X 向右增大，Y 向下增大
- `screenBBox` 以像素为单位，相对于截图分辨率
- `pick_area` 归一化坐标：(0,0) 左上 → (1,1) 右下

## 完成判定

1. 目标 action 全部 `ok=true`
2. 关键返回字段存在且结构正确
3. 异步选择（pick_area）已做二次查询（query_selection）
4. 最终用 `capture_viewport` 二次确认

## 失败恢复

当 `ok=false`：

1. 先读 `summary`/`error`
2. 重新 `describe_action` 校验参数名和类型
3. 修正后重试，避免盲目改动多个字段

禁止行为：

- 跳过 `describe_action` 直接猜参数
- 混用 `type` 与 `entityType`
- 对 `pick_area` 的返回直接当作最终选择结果
