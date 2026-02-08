# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用
- 你是一个 C++ 专家，写出的代码符合现代标准，并且是模块化的，不要堆砌再一个函数内

# 计划任务

1. 完善 generate_mesh_action 功能，实现通过 gmsh 对传入的 geometry entity 进行网格剖分：
    - 当传入多个 shape 时，将shape 合并成一个 component 再进行剖分
    - 使用 gmsh::model::occ::importShapesNativePointer 接口直接导入 occ shape 指针，避免中间文件读写

2. 实现一个 mesh 数据管理类，参考 geometry types 来个 element id，element uid + type（node、element）：
    - 支持存储 mesh 数据，分配好 node、element，映射 geometry entity 与 mesh entity 之间的关系，支持通过 mesh node、mesh element、geometry entity 见相互查找
    - 例如支持通过 mesh element id 获取对应的 mesh element 对象
    - 参考 geometry document 实现实现 render 数据导出接口，支持渲染显示 mesh element，要显示出 mesh node、edge、face 等不同类型

3. render 拾取相关梳理：
    - 当一个面被网格剖分后，要支持能拾取 element、element node 类型也要能拾取 geometry face 类型，考虑如何处理深度，因为一般 网格 element 会覆盖 geometry face 的拾取
    - select manager service 需要支持 mesh element、mesh node 类型的拾取
    - selector qml 组件需要支持 mesh element、mesh node 类型的拾取，注意生成对应的 icon

4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。
7. 为可以的代码单元编写单元测试代码，确保代码质量。
