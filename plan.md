# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 优化 mesh builder + render：
    - 排查当前网格剖分完不显示网格；
    - highlight pass mesh 需要把选中 ID 上传到 GL_RG32UI 纹理，修改为只在选中状态改变时更新纹理，避免每帧都上传数据。
    - batch 的 firsts counts 也要做缓存优化，只有当数据变化后才更新，避免每帧都构建 batch 数据。
    - hightlight 与 opaque pass 中，mesh 的 range 重复循环构建 batch 数据，是否可以在 build 阶段就构建好 batch 数据，渲染时直接使用，减少每帧构建 batch 的开销。
2. geo mesh 复用 batch 容器：把 firsts/counts（及索引偏移缓存）作为成员或传入的外部容器，clear() 后重用，避免每帧分配。
    - 优化 render select  manager，将 current sel 按照 tpye 进行分类存储，支持判断当前是否存在某种类型的选中状态；
3. 需要代码编译通过.