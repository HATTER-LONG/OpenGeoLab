---
name: opengeolab-commands
description: OpenGeoLab 命令协议与执行工作流，适用于 geometry mesh scene io；先 describe_action 再 execute_action，避免参数名和 localId 误用
---

# OpenGeoLab 命令协议

## 执行铁律（每次都要遵守）

1. 先确定目标 action
2. 必须先调用 `describe_action(module_name, action_name)` 获取当前 schema
3. 参数名、类型、必填严格按 schema 构造
4. 调用 `execute_action(module, action, params)`
5. 若 `ok: false`：读取 `summary`/`error`，修正后重试
6. 任务末尾统一用 `scene.capture_viewport` 做二次验收

## 模块名称（必须严格匹配，区分大小写）

| 模块名 | 用途 |
|--------|------|
| `geometry` | 几何体创建、导入、查询、删除 |
| `mesh` | 网格生成与查询 |
| `scene` | 场景状态管理（选择、相机、标签、可见性） |
| `io` | 文件读写 |

⚠️ **仅这 4 个模块名有效**。`opengeolab`、`cad`、`opengeolab-commands` 等均不是模块名。

## 工作流

1. 根据下方 action 列表确定目标
2. `describe_action(module_name, action_name)` 获取参数 schema
3. 用正确参数调用 `execute_action(module, action, params)`
4. `describe_module` 通常不需要（action 列表已知），仅在动态发现时使用
5. 若涉及选择/标签/可见性/相机变化，最后执行 `scene.capture_viewport` 验证结果

## ID 体系（关键！）

| 概念 | 说明 | 起始值 |
|------|------|--------|
| **shapeId** | 顶层形状标识符（由 create_box 等返回） | **0** |
| **localId** | 子形状在该类型内的编号 | **1**（不是 0！） |
| **EntityType** | 子形状类型 | GeoFace, GeoEdge, GeoVertex, GeoSolid, MeshNode, MeshElement |

**典型流程：创建几何体 → 查子形状 → 生成网格**
```
1. execute_action("geometry", "create_box", {width:10, height:10, depth:10})  → shapeId: 0
2. execute_action("geometry", "list_sub_shapes", {shapeId: 0})  → {faces:[1,2,3,4,5,6], edges:[1..12], ...}
3. execute_action("mesh", "generate_mesh", {entities:[{shapeId:0, type:"GeoFace", localId:1}, ...], ...})
```
**⚠️ 禁止假设 localId！必须先用 `list_sub_shapes` 查询。**

## 参数名易错点（重点）

实体语义一致，但字段名不统一：

| 场景 | 实体类型字段名 |
|------|----------------|
| `scene.select` / `scene.deselect` / `scene.query_selection` / `scene.set_hover` | `type` |
| `scene.add_label` / `scene.remove_label` / `scene.describe_labels` | `entityType` |

有效的实体类型值（`type` 和 `entityType` 通用）：

- 几何：`GeoVertex`、`GeoEdge`、`GeoWire`、`GeoFace`、`GeoSolid`
- 网格：`MeshNode`、`MeshEdge`、`MeshElement`

⚠️ **实体类型必须精确匹配，不要猜测**：
- `describe_labels` 返回 `entityType: "MeshElement"` → `deselect` 时 `type` 必须用 `"MeshElement"`，不能用 `"GeoFace"`
- 先 `query_selection` 获取精确的 `type` 值，再原样传给 `select` / `deselect`

速查模板：

```json
{
	"selectionEntity": {"shapeId": 0, "type": "GeoFace", "localId": 3},
	"labelEntity": {"shapeId": 0, "entityType": "GeoFace", "localId": 3}
}
```

## geometry 模块 (11 actions)

| Action | 描述 | 关键参数 |
|--------|------|----------|
| create_box | 创建长方体 | width, height, depth |
| create_cylinder | 创建圆柱体 | radius, height |
| create_sphere | 创建球体 | radius |
| create_torus | 创建环面体 | majorRadius, minorRadius |
| import_brep | 导入 BRep 文件 | filePath |
| import_step | 导入 STEP 文件 | filePath |
| tessellate | 三角化形状（可视化用） | shapeId |
| query_shape | 查询拓扑计数和包围盒 | shapeId |
| list_sub_shapes | **列出子形状 localId（生成网格前必用）** | shapeId |
| list_shapes | 列出所有已注册形状 | 无 |
| delete_shape | 删除形状 | shapeId |

## mesh 模块 (3 actions)

| Action | 描述 | 关键参数 |
|--------|------|----------|
| generate_mesh | gmsh 生成网格 | entities (含 shapeId+type+localId), elementSize, dimension |
| query_mesh_info | 查询网格统计 | shapeId (可选) |
| clear_mesh | 清除网格数据 | shapeId (可选, 不传则全部清除) |

## scene 模块 (20 actions)

| Action | 描述 |
|--------|------|
| capture_viewport | 捕获视口截图与元数据（AI 视觉验收核心），支持 outputPath 直接保存 PNG |
| select | 添加实体到选择集 |
| deselect | 移除选择 |
| clear_selection | 清空选择集 |
| query_selection | 返回当前选中实体 |
| set_pick_mode | 设置拾取模式和掩码 |
| set_hover | 设置悬停实体 |
| set_visibility | 批量设置节点可见性 |
| list_nodes | 查询所有场景节点 |
| fit_to_scene | 相机适配场景 |
| set_view_preset | 预设视角 (Front/Back/Top/Bottom/Left/Right/Isometric) |
| set_camera | 直接设置相机位置 |
| pick_area | 框选 |
| new_model | 重置工作区 |
| add_label | 创建/替换标签 |
| remove_label | 移除标签 |
| clear_labels | 清除所有标签 |
| describe_labels | 查询标签状态 |
| set_labels_visible | 启用/禁用标签渲染 |
| set_auto_label | 启用/禁用自动标签 |

说明：`pick_area` 为异步动作，返回 `async: true` 后要再调用 `query_selection` 获取最终选择结果。

## io 模块 (1 action)

| Action | 描述 |
|--------|------|
| read_brep | 读取 BRep 文件 |

## 参数约束

- 参数名和类型必须与 `describe_action` 返回的 schema 完全匹配
- `shapeId` 来自创建/导入结果，禁止虚构
- `localId` 是 **1-based**，禁止使用 0
- 数值参数使用标准 JSON 数字格式
- 对 scene 任务，优先补充一次 `capture_viewport` 做最终状态验收

## 错误处理

- `ok: false` 时，`summary` 或 `errors` 包含错误描述
- 出错时先 `describe_action` 检查参数 schema，再修正重试

## 完成判定（Completion Checks）

1. 核心业务 action 返回 `ok: true`
2. 必要回查已完成：
	- 框选后做 `query_selection`
	- 网格后做 `query_mesh_info`（若任务目标包含网格）
3. 最后一轮做 `scene.capture_viewport`，确认视图状态与任务一致
