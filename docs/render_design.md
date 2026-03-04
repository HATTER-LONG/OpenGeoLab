# OpenGeoLab 渲染设计（当前实现）

## 1. 目标

当前渲染层目标：
- Geometry 与 Mesh 统一为按 DrawRange 批次渲染；
- Mesh 以 Part 为单位组织渲染批次；
- Mesh 三角面颜色与对应 Part 保持一致，并做轻度变暗；
- 通过统一的 `RenderPassContext` 传递渲染参数，避免 pass 接口参数爆炸；
- 在保证拾取/高亮能力的前提下，优先采用 `multiDrawArrays/multiDrawElements` 批量提交。

---

## 2. 核心数据结构

### 2.1 DrawRange 与 DrawRangeEx

- `DrawRange`：GPU 连续区间（offset/count/topology）
- `DrawRangeEx`：在 `DrawRange` 上补充语义信息：
  - `m_entityKey`
  - `m_partUid`
  - `m_wireUid`

Geometry/Mesh 的三类拓扑都由 `std::vector<DrawRangeEx>` 维护：
- `m_geometryTriangleRanges / m_geometryLineRanges / m_geometryPointRanges`
- `m_meshTriangleRanges / m_meshLineRanges / m_meshPointRanges`

### 2.2 RenderNode 语义树

`RenderData` 保留语义树：
- `m_geometryRoots`
- `m_meshRoots`

语义树用于层级语义与可见性表达；真实绘制依赖 DrawRangeEx 批次数据。

### 2.3 Part 归属映射

`PickResolutionData::m_entityToPartUid` 使用 **编码 PickId(type+uid)** 作为 key，避免不同实体类型 UID 冲突。

---

## 3. Mesh 按 Part 渲染设计

### 3.1 网格单元来源 Part

`MeshElement` 新增 `sourcePartUid`，由网格生成阶段写入。

- 文件：`src/mesh/action/generate_mesh_action.cpp`
- 逻辑：
  1. 输入实体先归并为「每个 part 一个 compound shape」；
  2. 导入 Gmsh 时记录 dim-tag -> partUid；
  3. 提取单元时按 dim-tag 归属创建 `MeshElement(type, partUid)`。

### 3.2 MeshRenderBuilder 输出

`MeshRenderBuilder` 按 part 构建三相缓冲：
1. Triangles（按 part + element type 分批）
2. Lines（按 part 去重边）
3. Points（按 part 节点集合）

并写出 `m_mesh*Ranges`，每个 range 均带 `m_partUid`。

### 3.3 颜色策略

输入：
- `defaultSurfaceColor`
- `partSurfaceColors`
- `partDarkenFactor`（默认 0.85）

最终颜色：
- 先取 part 基色（无则 fallback）
- 再 `ColorMap::darkenColor(base, factor)` 得到 mesh surface 颜色

满足“与 part 一致但略深”的视觉要求。

---

## 4. 统一 RenderPassContext

新增文件：`src/render/pass/render_pass_context.hpp`

`RenderPassContext` 包含：
- `PassRenderParams`（view/proj/camera/xray）
- Geometry 输入（buffer + tri/line/point ranges）
- Mesh 输入（buffer + tri/line/point ranges + display mode）

pass 接口统一：
- `OpaquePass::render(const RenderPassContext&)`
- `TransparentPass::render(const RenderPassContext&)`
- `WireframePass::render(const RenderPassContext&)`
- `HighlightPass::render(const RenderPassContext&)`
- `SelectionPass::renderToFbo(const RenderPassContext&, RenderEntityTypeMask)`

---

## 5. RenderSceneImpl 流程

### 5.1 同步阶段

`synchronize()`：
- 上传 geometry/mesh GPU buffer；
- 缓存 geometry + mesh 的 DrawRangeEx；
- 缓存显示模式与拾取解析数据引用。

### 5.2 渲染阶段

`render()`：
1. 重置 GL 状态 + 清屏
2. 组装 `RenderPassContext`
3. Surface pass：
   - XRay 开：Transparent
   - XRay 关：Opaque
4. Wireframe pass
5. Highlight pass（仅在 hover 或 selection 非空时执行）
6. Post/UI pass

### 5.3 拾取阶段

`processHover/processPicking`：
- 同样组装 `RenderPassContext` 给 SelectionPass；
- SelectionPass 使用 DrawRangeEx 批次绘制（含 mesh ranges）；
- 通过 PickResolver 解析结果并做 Wire/Part 反查。

---

## 6. multiDrawArrays 实践

Mesh 三类绘制均从“总 count 单次 glDrawArrays”迁移为“按 ranges 的 batch multi-draw”：
- `PassUtil::buildArrayBatch`
- `PassUtil::multiDrawArrays`

收益：
- 支持按 part 过滤可见性；
- 支持按实体类型筛选拾取；
- 与 geometry 的 DrawRange 思路一致。

---

## 7. Highlight 优化点

Mesh 高亮由“单一选中数组”拆分为：
- surface selections
- line selections
- node selections

每个拓扑单独上传选中 pickId 列表，减少 shader 内 `for` 循环次数与无效比较。

---

## 8. 兼容与注意事项

1. `MeshDocument::getRenderData` 接口已更新：
   - 需要传入 `default_surface_color` 与 `part_surface_colors`。
2. `PickResolutionData::m_entityToPartUid` key 语义变化为编码 pickId。
3. Mesh 节点如果被多个 part 共享，节点 pick 到 part 的映射采用首次归属（`try_emplace`）。

---

## 9. 后续建议

- 如需更强高亮性能，可继续把 mesh 高亮从“片元 discard”升级为“CPU 预筛 DrawRange 后单独绘制”；
- 如需更稳定节点归属，可在网格生成阶段显式记录 node->part 多重关系并在拾取阶段做优先级策略；
- 若后续增加更多 pass，优先沿用 `RenderPassContext`，避免再次出现参数膨胀。
