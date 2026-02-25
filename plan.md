# 软件简介
软件主要功能是读取 BREP/STP 模型或手动创建几何后，通过 OpenGL 渲染显示；几何拓扑由 OCC 管理，支持后续鼠标几何编辑（如 trim）、选择点边面与 part，并计划引入网格剖分与 AI 辅助网格质量诊断/修复。

# 软件架构约束
- `include/` 仅放跨模块公共接口；
- `src/` 通过 `include/` 暴露接口进行模块调用，避免跨模块直接依赖实现细节。


# 开发步骤
1. 渲染存入 56 位 id + 8 位 type 的 U32GL 纹理，支持 vertex/edge/face/solid/part/node/line element 的拾取。
2. pick 功能优化获取像素，只针对目标类型进行渲染一帧数据(例如只拾取 vertex edge face，只渲染这三个，不渲染网格)，提升性能。
3. 支持反向查询，已知 vertex edge face solid part node line element id 能获取对应的几何信息（例如 vertex id 获取 vertex 坐标，edge id 获取 edge 连接的 vertex id 等等）。以支持 part wire solid 拾取。
4. 优化 render data 中的 color ，是否可以通过 part id 获取 color，达到同一 part 同一 color 的效果。而不用每个 render data 存储，edge vertex mesh 相关同理公用同一套颜色即可。
5. 注释整改（分批）：按 `doxygen_comment_style.md` 与 `doxygen_comment_style_cn.md` 持续清理 `include/src/resources/qml/test`。
6. 文档与计划收口：同步更新中英文 README 与 JSON 协议文档。

# 验收标准
1. 工程可成功编译，程序可启动。
2. 视图预设、Fit、渲染模式切换请求可正常下发。
3. geometry/mesh 文档变更可触发渲染数据更新并驱动场景刷新。
4. 文档内容与实现一致（中文为主，英文同步）。
