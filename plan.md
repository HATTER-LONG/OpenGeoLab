# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 根据当前的 render_data 将渲染模块分为 OpaquePass TransparentPass  WireframePass selectionPass HighlightPass PostProcessPass UIPass模块，分别处理不透明物体、透明物体、线框、选择高亮、后处理和 UI 渲染，优化渲染流程和性能。
2. select pass 首先只渲染当前拾取类型的对象，其次读取 pixel 直接分析出最佳拾取结果，避免每次拾取都要分析所有拾取数据。Vertex > Edge > Face , mesh node > mesh line > mesh element
3. 渲染尽量使用  instancing 或 multi draw 来减少 draw call，尤其是对于大量重复的对象，例如几何面、顶点、网格元素、线段等。

