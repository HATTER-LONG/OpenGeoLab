# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 审查当前 render 模块，从 render data，到 geometry mesh render builder，再到 render core pass scene 等，基于效率和设计的角度，根据最佳实践进行重构优化，明确各个类的职责边界，减少不必要的耦合和重复代码。
2. geo mesh 复用 batch ，例如再更新渲染数据时将 batch 生成好，给各个 pass 用。
3. mesh 网格与surface 互相重合干扰显示效果不好，请重新设计 mesh 与 surface 关系，mesh 应当显示在 surface edge 之上，但是要避免用固定的视空间前推，把本该被遮挡的元素硬拉到了前面，导致显示效果不自然。
3. 需要代码编译通过.