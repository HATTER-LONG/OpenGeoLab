# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 优化 mesh builder + render：
    - highlight pass mesh 需要把选中 ID 上传到 GL_RG32UI 纹理，修改为只在选中状态改变时更新纹理，避免每帧都上传数据。
    - 用 PBO/SSBO 做异步或更大规模的查找结构 或 采用“按 part 粗分 + GPU 精细识别”的混合方案，首先在 CPU 端根据 part 粗略判断可能被选中的对象列表，然后在 GPU 端通过 render select pass 精确识别最终选中对象，减少每次拾取时需要分析的对象数量，提高拾取效率。
    - hightlight 与 opaque pass 中，mesh 的 range 重复循环构建 batch 数据，是否可以在 build 阶段就构建好 batch 数据，渲染时直接使用，减少每帧构建 batch 的开销。
2. geo mesh 复用 batch 容器：把 firsts/counts（及索引偏移缓存）作为成员或传入的外部容器，clear() 后重用，避免每帧分配。
5. 需要代码编译通过.