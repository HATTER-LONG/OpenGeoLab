# 软件简介
软件主要功能是读取 BREP/STP 模型或手动创建几何后，通过 OpenGL 渲染显示；几何拓扑由 OCC 管理，支持后续鼠标几何编辑（如 trim）、选择点边面与 part，并计划引入网格剖分与 AI 辅助网格质量诊断/修复。

# 软件架构约束
- `include/` 仅放跨模块公共接口；
- `src/` 通过 `include/` 暴露接口进行模块调用，避免跨模块直接依赖实现细节。


# 开发步骤
0. 接通 Query 拾取功能，视察 qml 到 select 功能是否正常
1. 当前 edge 颜色太灰了，edge 颜色要与 face vertex 区分开，要亮色。
2. 模块化 render scene 模块功能，拆分成 render core 、以及 pass 处理模块，堆积到一起。
4. 实现一个全局的 color 管理类，支持根据 part id 获取不同颜色的，统一 edge、vertex、mesh node、line、element 颜色，包括 hover 颜色 选中高亮颜色等等。
5. 注释整改（分批）：按 `doxygen_comment_style.md` 与 `doxygen_comment_style_cn.md` 持续清理 `include/src/resources/qml/test`。
6. 文档与计划收口：同步更新中英文 README 与 JSON 协议文档。

# 当前完成状态（2026-02-25）
- [x] 0 Query 拾取链路（QML → SelectControl → 离屏 pick）连通
- [x] 1 Edge 颜色提亮并与 Face/Vertex 区分
- [x] 2 RenderScene 按 core / gpu-pass / picking 模块拆分
- [x] 4 全局 `RenderColorManager` 落地，统一 geometry/mesh 颜色入口并预留 hover/selected
- [~] 5 注释整改持续进行（本批次已覆盖新增模块）
- [x] 6 README 与 JSON 协议文档同步

# 验收标准
1. 工程可成功编译，程序可启动。
2. 视图预设、Fit、渲染模式切换请求可正常下发。
3. geometry/mesh 文档变更可触发渲染数据更新并驱动场景刷新。
4. 文档内容与实现一致（中文为主，英文同步）。
