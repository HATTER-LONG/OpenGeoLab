# 面向大模型的几何与网格协议设计

## 1. 目标

本文档定义一套适合 OpenGeoLab 与大模型协作的几何/网格数据协议约定，目标是让模型能够：

- 高效查询几何与网格的结构、属性和关系。
- 调用受控操作接口执行剖分、平滑、局部修改和质量诊断。
- 在对话中稳定引用 Part、Face、Edge、Node、Element 等实体。
- 以“可分析、可追踪、可回放”的方式输出建议和操作计划。

## 2. 设计原则

- 统一实体标识：所有实体必须以 type + uid 组合标识，避免仅靠名字或索引定位。
- 查询与修改分离：query 类接口只返回事实，mutate 类接口必须显式返回影响范围与结果摘要。
- 保留关系图：返回结果中应同时提供父子关系、邻接关系和来源关系，减少模型二次猜测。
- 支持摘要层级：同一对象既能返回 summary，也能按需展开 detail。
- 支持可解释执行：所有自动操作都要返回参数、步骤和影响实体。

## 3. 统一实体模型

建议所有接口统一返回以下基础结构：

```json
{
  "type": "Face",
  "uid": 123,
  "name": "optional-name",
  "part_uid": 1,
  "attributes": {},
  "relations": {}
}
```

### 3.1 几何实体

- Vertex
- Edge
- Wire
- Face
- Shell
- Solid
- Part

### 3.2 网格实体

- Node
- Line
- Triangle
- Quad4
- Tetra4
- Hexa8
- Prism6
- Pyramid5

## 4. 查询协议建议

### 4.1 文档级摘要

接口示例：`query_document_summary`

返回建议：

```json
{
  "success": true,
  "geometry": {
    "partCount": 2,
    "faceCount": 18,
    "edgeCount": 42,
    "boundingBox": {
      "min": [0.0, 0.0, 0.0],
      "max": [120.0, 80.0, 30.0]
    }
  },
  "mesh": {
    "nodeCount": 12840,
    "elementCount": 25120,
    "elementBreakdown": {
      "Triangle": 18000,
      "Quad4": 7120
    }
  }
}
```

### 4.2 实体详情

接口示例：`query_entity_detail`

输入：`[{"type":"Face","uid":123}]`

输出建议：

```json
{
  "success": true,
  "entities": [
    {
      "type": "Face",
      "uid": 123,
      "part_uid": 1,
      "geometry": {
        "surfaceType": "Plane",
        "area": 35.6,
        "boundingBox": {
          "min": [0.0, 0.0, 0.0],
          "max": [10.0, 8.0, 0.0]
        }
      },
      "relations": {
        "parent": [{"type": "Solid", "uid": 8}],
        "children": [{"type": "Wire", "uid": 55}],
        "adjacentFaces": [{"type": "Face", "uid": 124}]
      }
    }
  ]
}
```

### 4.3 网格局部详情

接口示例：`query_mesh_region`

输入支持：

- Part 级别
- Face 级别
- Mesh Element 级别
- 自定义包围盒或实体集合

输出建议至少包含：

- 节点列表或节点摘要
- 单元列表或单元摘要
- 邻接信息
- 质量统计
- 来源几何映射

## 5. 修改协议建议

### 5.1 通用 mutate 包结构

```json
{
  "action": "mutate_mesh",
  "operation": "smooth",
  "targets": [
    {"type": "Part", "uid": 1},
    {"type": "Triangle", "uid": 8001}
  ],
  "parameters": {
    "method": "taubin",
    "iterations": 8,
    "factor": 0.35,
    "preserveBoundaries": true
  }
}
```

返回建议：

```json
{
  "success": true,
  "operation": "smooth",
  "affected": {
    "targetNodeCount": 420,
    "smoothedNodeCount": 396,
    "boundaryNodeCount": 24
  },
  "metrics": {
    "maxDisplacement": 0.018,
    "qualityBefore": {"min": 0.12, "mean": 0.64},
    "qualityAfter": {"min": 0.24, "mean": 0.71}
  }
}
```

### 5.2 剖分操作

建议统一为：

- `generate_mesh`
- `refine_mesh`
- `coarsen_mesh`
- `smooth_mesh`
- `repair_mesh`

所有操作返回：

- 参数快照
- 影响实体统计
- 失败实体列表
- 新质量指标
- 可选的回滚 token

## 6. 面向大模型的分析接口

### 6.1 方案推荐

接口示例：`analyze_meshing_strategy`

输入内容：

- 几何摘要
- 目标物理场景
- 尺寸尺度
- 质量要求
- 当前限制条件

输出内容：

- 推荐网格维度
- 推荐单元类型
- 推荐算法
- 推荐尺寸场策略
- 风险点
- 建议操作步骤

### 6.2 质量诊断

接口示例：`diagnose_mesh_quality`

输出建议：

```json
{
  "success": true,
  "summary": {
    "worstElements": 52,
    "mainIssues": ["skewness", "aspect_ratio"]
  },
  "issues": [
    {
      "type": "Triangle",
      "uid": 8801,
      "score": 0.08,
      "reasons": ["high_aspect_ratio"],
      "suggestions": [
        "局部平滑 6 次",
        "缩小边界附近目标尺寸到 0.6"
      ]
    }
  ]
}
```

## 7. 对话友好性要求

为方便大模型稳定推理，建议所有接口满足：

- 数组字段显式命名，不返回位置含义不明的裸数组。
- 返回值同时提供统计摘要与可展开细节。
- 对枚举值使用稳定字符串，不使用仅内部可读的数字编码。
- 修改类接口返回前后对比，而不是只返回 success。
- 当目标过大时，支持 `summaryOnly`、`limit`、`cursor`。

## 8. 推荐实现顺序

1. 补齐统一实体详情查询接口，覆盖 geometry + mesh。
2. 为 mesh 操作统一返回 affected/metrics/result 三段式结构。
3. 增加质量诊断接口，把质量指标与问题单元列表标准化。
4. 增加分析类接口，让大模型先给方案，再调用具体 mutate 接口。
5. 最后增加自动修复编排接口，用于多步质量修复闭环。

## 9. 与当前工程的对齐建议

当前工程已经具备以下基础：

- 几何与网格的 `type + uid` 选择体系。
- JSON action 路由模型。
- `generate_mesh` 与 `smooth_mesh` 这类可扩展 action 入口。
- 渲染与选择快照可作为后续 query 接口的数据基础。

建议下一步在 `docs/json_protocols.md` 中继续补齐 MeshService 的查询与修改协议，并让所有 mesh action 的响应都携带统一摘要字段。
