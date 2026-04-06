---
name: opengeolab-commands
description: OpenGeoLab 命令协议与完整 action 参数参考；先 describe_action 再 execute_action，涵盖 geometry mesh scene 三大模块
---

# OpenGeoLab 命令协议

## 执行铁律（每次都要遵守）

1. 先确定目标 action
2. 必须先调用 `describe_action(module_name, action_name)` 获取当前 schema
3. 参数名、类型、必填严格按 schema 构造
4. 调用 `execute_action(module, action, params)`
5. 若 `ok: false`：读取 `summary`/`error`，修正后重试
6. 任务末尾统一用 `capture_viewport` 做二次验收

## 模块名称（必须严格匹配，区分大小写）

| 模块名 | 用途 |
|--------|------|
| `geometry` | 几何体创建、导入、查询、删除 |
| `mesh` | 网格生成与查询 |
| `scene` | 场景状态管理（选择、相机、标签、可见性） |

⚠️ **仅这 3 个模块名有效**。`opengeolab`、`cad`、`io`、`opengeolab-commands` 等均不是模块名。

## ID 体系（关键！）

| 概念 | 说明 | 起始值 |
|------|------|--------|
| **shapeId** | 顶层形状标识符（由 create_box 等返回） | **0** |
| **localId** | 子形状在该类型内的编号 | **1**（不是 0！） |
| **EntityType** | 实体类型 | 见下方实体类型表 |

## 参数名易错点（重点）

实体 3 元组概念一致（shapeId + 类型 + localId），但字段名不统一：

| 使用场景 | 实体类型字段名 |
|----------|----------------|
| `select` / `deselect` / `query_selection` / `set_hover` | `type` |
| `add_label` / `remove_label` / `describe_labels` | `entityType` |
| `generate_mesh` / `query_mesh_info` 的 entities | `type` |

有效的实体类型值（`type` 和 `entityType` 通用）：

| 类别 | 值 |
|------|-----|
| 几何拓扑 | `GeoVertex`、`GeoEdge`、`GeoWire`、`GeoFace`、`GeoSolid` |
| 网格 | `MeshNode`、`MeshEdge`、`MeshElement` |

⚠️ **实体类型必须精确匹配**：从 `query_selection` 返回的 `type` 值原样传给 `select` / `deselect`，不要猜测或转换。

字段映射速查：

```json
{
	"selectionEntity": {"shapeId": 0, "type": "GeoFace", "localId": 3},
	"labelEntity": {"shapeId": 0, "entityType": "GeoFace", "localId": 3},
	"meshEntity": {"shapeId": 0, "type": "GeoFace", "localId": 1}
}
```

## 响应格式

- 成功：`{"ok": true, "action": "xxx", ...}`
- 失败：`{"ok": false, "action": "xxx", "summary": "错误描述", "error": "traceback"}`

---

## geometry 模块 (11 actions)

### 创建几何体（4 个）

所有创建 action 共享以下可选参数：

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `name` | string | 按类型生成 | 形状名称 |
| `tessellate` | boolean | true | 创建后自动三角化 |
| `linearDeflection` | number | 0.1 | 三角化线性偏差 |
| `angularDeflection` | number | 0.5 | 三角化角度偏差 |

返回：`{ok, action, shapeId, name, topology: {solids, faces, edges, vertices, wires}}`

| Action | 描述 | 专有参数（全部可选） |
|--------|------|---------------------|
| `create_box` | 创建长方体 | `width` (默认 1.0), `height` (默认 1.0), `depth` (默认 1.0), `origin` ([x,y,z] 默认 [0,0,0]) |
| `create_cylinder` | 创建圆柱体 | `radius` (默认 0.5), `height` (默认 1.0), `origin` ([x,y,z] 默认 [0,0,0]) |
| `create_sphere` | 创建球体 | `radius` (默认 1.0), **`center`** ([x,y,z] 默认 [0,0,0]) |
| `create_torus` | 创建环面体 | `majorRadius` (默认 1.0), `minorRadius` (默认 0.25), `origin` ([x,y,z] 默认 [0,0,0]) |

⚠️ sphere 使用 **`center`**，其余使用 `origin`。

### 导入（2 个）

| Action | 描述 | 必填参数 | 可选参数 |
|--------|------|----------|----------|
| `import_step` | 导入 STEP 文件 | **`path`** (string) | `name`, `tessellate`, `linearDeflection`, `angularDeflection` |
| `import_brep` | 导入 BRep 文件 | **`path`** (string) | `name`, `tessellate`, `linearDeflection`, `angularDeflection` |

⚠️ 参数名是 **`path`**，不是 `filePath`。

返回：`{ok, action, shapeId, name, topology}`

### 查询与管理（5 个）

| Action | 描述 | 参数 | 关键返回 |
|--------|------|------|----------|
| `list_shapes` | 列出所有形状 | 无 | `{ok, count, shapes: [{shapeId, name, shapeType, hasTessellation, topology, boundingBox}]}` |
| `query_shape` | 查询形状详情 | `shapeId` (**必填**) | `{ok, shapeId, name, topology, boundingBox: {min:[x,y,z], max:[x,y,z]}}` |
| `list_sub_shapes` | 列出子形状 localId | `shapeId` (**必填**) | `{ok, shapeId, name, subShapes: {faces:[1,2,...], edges:[...], vertices:[...], solids:[...], wires:[...]}}` |
| `tessellate` | 重新三角化 | `shapeId` (**必填**); 可选: `linearDeflection`, `angularDeflection` | `{ok, shapeId}` |
| `delete_shape` | 删除形状 | `shapeId` (**必填**) | `{ok, shapeId}` |

**典型流程：创建几何体 → 查子形状 → 生成网格**

```
1. execute_action("geometry", "create_box", {"width":10, "height":10, "depth":10})  → shapeId: 0
2. execute_action("geometry", "list_sub_shapes", {"shapeId": 0})  → subShapes: {faces:[1,2,3,4,5,6], edges:[1..12], ...}
3. execute_action("mesh", "generate_mesh", {"entities":[{"shapeId":0, "type":"GeoFace", "localId":1}, ...], "elementSize":1.0})
```

⚠️ 禁止假设 localId！必须先用 `list_sub_shapes` 查询。

---

## mesh 模块 (3 actions)

### generate_mesh

从几何面/实体生成网格（gmsh 引擎）。

| 参数 | 类型 | 必填 | 默认 | 说明 |
|------|------|------|------|------|
| `entities` | array | **是** | — | `[{shapeId, type, localId}]`，type 限 `GeoFace` 或 `GeoSolid` |
| `elementSize` | number | 否 | 1.0 | 目标单元尺寸 |
| `dimension` | integer | 否 | 2 | 网格维度：2（表面）或 3（体积） |
| `elementType` | string | 否 | `"triangle"` | `"triangle"` 或 `"quad"` |
| `algorithm` | string | 否 | `"delaunay"` | 网格算法名称 |
| `advanced` | object | 否 | — | `{minSize, maxSize, order, optimize}` |

返回：`{ok, results: [{shapeId, nodeCount, elementCount}]}`

### query_mesh_info

查询网格节点/边/单元详情。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `entities` | array | **是** | `[{shapeId, type, localId}]`，type 限 `MeshNode` / `MeshEdge` / `MeshElement` |

⚠️ 参数是 **`entities`** 数组（**必填**），不是 `shapeId`。

返回（按 type 不同）：
- **MeshNode**：`{shapeId, type, localId, ok, position: [x,y,z]}`
- **MeshElement**：`{shapeId, type, localId, ok, elementType: "Triangle"|"Quad"|..., nodeLocalIds: [...]}`
- **MeshEdge**：`{shapeId, type, localId, ok, summary: "Mesh edges are derived at render time only."}`

### clear_mesh

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `shapeId` | integer | 否 | 指定形状的网格；省略则清除全部 |

返回：`{ok, cleared: <数量>}`

---

## scene 模块 (20 actions)

详见 **viewport-operations** skill 文档，包含完整参数表和任务模板。

快速参考：

| Action | 描述 | 关键参数 |
|--------|------|----------|
| `capture_viewport` | 捕获视口截图与元数据 | `width`, `height`, `captureImage`, `outputPath` |
| `select` | 添加实体到选择集 | `entities[]`, `append` |
| `deselect` | 移除选择 | `entities[]` |
| `clear_selection` | 清空选择集 | 无 |
| `query_selection` | 返回当前选中实体 | 无 |
| `set_pick_mode` | 设置拾取模式和掩码 | `pickMask`, `enabled` |
| `set_hover` | 设置悬停实体 | `entity` (可选对象，null 清除) |
| `set_visibility` | 批量设置节点可见性 | `type` ("geometry"/"mesh"/"node"), `nodes[]` |
| `list_nodes` | 查询所有场景节点 | 无 |
| `fit_to_scene` | 相机适配场景 | 无 |
| `set_view_preset` | 预设视角 | `preset`: Front/Back/Top/Bottom/Left/Right/Isometric |
| `set_camera` | 设置相机位置 | `position[]`, `target[]`, `up[]` (各 [x,y,z]) |
| `pick_area` | 框选（**异步**） | `x0`, `y0`, `x1`, `y1`, `coordType`, `pickAction` |
| `new_model` | 重置工作区 | 无 |
| `add_label` | 创建标签（文本自动生成） | `shapeId`, `entityType`, `localId` |
| `remove_label` | 移除标签 | `shapeId`, `entityType`, `localId` |
| `clear_labels` | 清除所有标签 | 无 |
| `describe_labels` | 查询标签状态 | 无 |
| `set_labels_visible` | 启用/禁用标签渲染 | `visible` |
| `set_auto_label` | 启用/禁用自动标签 | `enabled` |

⚠️ `pick_area` 为异步动作，返回 `async: true` 后要再调用 `query_selection` 获取最终选择结果。

---

## 参数约束

- 参数名和类型必须与 `describe_action` 返回的 schema 完全匹配
- `shapeId` 来自创建/导入结果，禁止虚构
- `localId` 是 **1-based**，禁止使用 0
- 数值参数使用标准 JSON 数字格式
- 文件路径使用绝对路径

## 错误处理

- `ok: false` 时：`summary` 包含简洁错误描述，`error` 包含详细 traceback
- 出错时先 `describe_action` 重新检查参数 schema，再修正重试
- 常见错误：参数名拼写错误（如 `filePath` 应为 `path`）、localId 从 0 开始、实体类型不匹配

## 完成判定

1. 核心业务 action 返回 `ok: true`
2. 必要回查已完成：
	- 框选后做 `query_selection`
	- 网格后做 `query_mesh_info`（若任务目标包含网格）
3. 最后一轮做 `capture_viewport`，确认视图状态与任务一致
