# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 优化 GeometryRenderBuilder::build 方法，调整 render_data 结构，直接在 build 阶段生成按 topology 分类的 DrawRangeEx，避免每次渲染都要调用 collectDrawRangesEx 来重新计算一次 topology 的 draw range。
2. 为 render select 增加计数，记录每个 entity type 的 uid 列表，在渲染时，先一次draw call 普通的 render data，再一次 draw call render select data over pass 高亮目标，也就是增加一个 hight pass。
3. 优化拾取逻辑， m_pickResolver.rebuild 等没有必要 与 render data 里的 m_pickData 重复，尽量在 build 阶段就构建好拾取数据，减少每次拾取时的计算开销。select pass 首先只渲染当前拾取类型的对象，其次读取 pixel 后 直接分析出最佳拾取结果，避免每次拾取都要分析所有拾取数据。
4. 将渲染模块分为 OpaquePass TransparentPass  WireframePass selectionPass（替换当前的 pick pass） HighlightPass PostProcessPass UIPass模块，分别处理不透明物体、透明物体、线框、选择高亮、后处理和 UI 渲染，优化渲染流程和性能。
