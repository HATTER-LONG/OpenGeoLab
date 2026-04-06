---
name: action-remote-call
description: 需要通过 HTTP 控制 OpenGeoLab 时使用 — 创建几何体、截取视窗、控制相机、管理选择、生成网格，或通过 REST API 远程调用任意 Action
---

# Action Remote Call — 渐进式发现

OpenGeoLab 暴露统一的 HTTP REST API（`POST /api/v1/action`）用于远程控制。
所有操作共享同一 JSON 信封格式；使用内置 **schema 发现** action 在运行时查询精确参数，而不是死记硬背。

## 快速开始

```bash
URL=http://127.0.0.1:8080/api/v1/action

# 健康检查
curl -s http://127.0.0.1:8080/api/v1/health

# 执行任意 action
curl -s -X POST $URL -H "Content-Type: application/json" \
  -d '{"module":"geometry","action":"create_box","param":{"width":2}}'
```

**请求格式：** `{"module": "<模块名>", "action": "<动作名>", "param": {...}}`
**成功响应：** `{"ok": true, ...结果字段...}`
**失败响应：** `{"ok": false, "summary": "...", "errors": [...]}`

默认地址：`http://127.0.0.1:8080`（端口可在 UI 中配置）。
已启用 CORS（`Access-Control-Allow-Origin: *`）。

---

## Schema 发现（执行前必须）

执行任何 action 之前，**必须先通过 `system` 模块查询其精确参数**。
这是唯一权威来源 — **不要猜测参数名或类型**。

### 第一步：获取模块的 action 列表

根据下方模块概览，选择目标模块并查询其所有 action：

```bash
curl -s -X POST $URL -d '{"module":"system","action":"describe_module","param":{"module":"geometry"}}'
```

返回：`{"ok":true, "module":"geometry", "description":"...", "actions":[{"name":"create_box","description":"..."}, ...]}`

### 第二步：获取具体 action 的精确参数

```bash
curl -s -X POST $URL -d '{"module":"system","action":"describe_action","param":{"module":"geometry","action":"create_box"}}'
```

返回完整参数 schema，包含类型、默认值和必填标记 — 依据此构造请求。

### 完整系统 schema（高级，一次性获取）

```bash
curl -s -X POST $URL -d '{"module":"system","action":"describe","param":{}}'
```

返回所有模块、所有 action、所有参数/返回值 schema。适合批量内省。

---

## 模块概览

每个模块负责一个独立领域。使用 `system.describe_module` 发现模块内的所有 action。

| 模块 | Action 数 | 职责 |
|------|-----------|------|
| `geometry` | 11 | **创建**基元体（长方体、球体、圆柱、圆环）、**导入**外部 CAD 文件（STEP、BRep）、**查询**形状拓扑/包围盒、**列出**子拓扑（面、边、顶点）、**删除**形状、**细分**可视化 |
| `mesh` | 3 | 从几何面/体**生成**有限元网格（三角形/四边形，可配置算法）、**查询**网格节点/边/单元信息、**清除**网格数据 |
| `scene` | 20 | **截取**视窗截图（PNG 文件或 base64 + 元数据）、**控制**相机（位置/预设/适配）、**选择/取消选择**实体、**标注**几何实体、**管理**可见性和场景树、**重置**工作区 |
| `io` | 1 | 文件读取操作（部分实现） |
| `system` | 5 | `capabilities`、`describe`、`list_modules`、`describe_module`、`describe_action` — 运行时内省 |

模块名**区分大小写**，仅以上 5 个名称有效。

---

## 关键 Action（代表性示例）

以下展示几个重要 action 的用法。完整参数请使用 `system.describe_action` 查询。

### geometry.create_box — 创建长方体

```bash
curl -s -X POST $URL -d '{"module":"geometry","action":"create_box","param":{"width":3,"height":2,"depth":1}}'
```

返回：`{"ok":true, "shapeId":0, "name":"Box", "topology":{"solids":1,"faces":6,"edges":12,"vertices":8}}`

⚠️ **易错点：** `create_sphere` 使用 `center`（不是 `origin`）。导入操作使用 `path`（不是 `filePath`）。

### scene.capture_viewport — 截图与场景分析

```bash
# 保存到文件
curl -s -X POST $URL -d '{"module":"scene","action":"capture_viewport","param":{"outputPath":"C:/test/viewport.png","captureImage":false}}'

# 获取 base64 图片 + 元数据
curl -s -X POST $URL -d '{"module":"scene","action":"capture_viewport","param":{"captureImage":true,"includeMetadata":true}}'
```

返回元数据（相机、可见形状及屏幕包围盒、选择状态、标签）和/或 base64 PNG 图片。
`outputPath` 和 `captureImage` 可独立使用或同时使用。

### scene.select — 选择实体

```bash
curl -s -X POST $URL -d '{"module":"scene","action":"select","param":{"entities":[{"shapeId":0,"type":"GeoFace","localId":1}]}}'
```

⚠️ 选择/取消选择使用 `type`，标签操作使用 `entityType`。`localId` 是 **1-based** — 必须先用 `geometry.list_sub_shapes` 查询。

### mesh.generate_mesh — 生成网格

```bash
curl -s -X POST $URL -d '{"module":"mesh","action":"generate_mesh","param":{"entities":[{"shapeId":0,"type":"GeoFace","localId":1}],"elementSize":0.5}}'
```

⚠️ 使用 `system.describe_action` 发现 `dimension`、`elementType`、`algorithm`、`advanced` 等选项。

---

## 实体引用格式

所有涉及实体引用的 action 使用此结构：

```json
{"shapeId": 0, "type": "GeoFace", "localId": 1}
```

| 字段 | 说明 |
|------|------|
| `shapeId` | 0-based 形状 ID（来自 `create_*` / `list_shapes`） |
| `type` 或 `entityType` | 选择/悬停用 `type`；标签操作用 `entityType` |
| `localId` | **1-based** 拓扑 ID（来自 `list_sub_shapes`） |

**几何类型：** `GeoVertex`、`GeoEdge`、`GeoWire`、`GeoFace`、`GeoSolid`
**网格类型：** `MeshNode`、`MeshEdge`、`MeshElement`

---

## 典型工作流

### 建模 → 查看 → 截图

```bash
curl -s -X POST $URL -d '{"module":"geometry","action":"create_box","param":{"width":3,"height":2,"depth":1}}'
curl -s -X POST $URL -d '{"module":"scene","action":"fit_to_scene","param":{}}'
curl -s -X POST $URL -d '{"module":"scene","action":"set_view_preset","param":{"preset":"Isometric"}}'
curl -s -X POST $URL -d '{"module":"scene","action":"capture_viewport","param":{"outputPath":"C:/test/shot.png","captureImage":false}}'
```

### 选择 → 标注 → 分析

```bash
curl -s -X POST $URL -d '{"module":"geometry","action":"list_sub_shapes","param":{"shapeId":0}}'
curl -s -X POST $URL -d '{"module":"scene","action":"select","param":{"entities":[{"shapeId":0,"type":"GeoFace","localId":1}]}}'
curl -s -X POST $URL -d '{"module":"scene","action":"add_label","param":{"shapeId":0,"entityType":"GeoFace","localId":1}}'
curl -s -X POST $URL -d '{"module":"scene","action":"capture_viewport","param":{}}'
```

---

## 常见错误

| 错误做法 | 正确做法 |
|----------|----------|
| 猜测参数名 | 先用 `system.describe_action` 查询 |
| `localId` 从 0 开始 | `localId` 是 **1-based**，先用 `list_sub_shapes` 查 |
| 用 `filePath` 导入文件 | 参数名是 **`path`** |
| sphere 用 `origin` | sphere 使用 **`center`** |
| add_label 用 `type` | 标签操作使用 **`entityType`** |
| 把 `pick_area` 当同步 | `pick_area` 是异步 — 需后续调用 `query_selection` |
| 文件路径用单反斜杠 | 使用 `C:/path` 或 `C:\\path` |
| 未启动 HTTP Server 就调用 | 需先在 OpenGeoLab UI 中启动 HTTP Server 插件 |
