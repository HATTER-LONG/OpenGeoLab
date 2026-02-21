# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 设计一套 mesh node、element 间的引用关系，支持 node \ element edge \ element (不同的类型) 之间快速索引。
2. 优化 render 数据，mesh node、element 不要单独占用 render mesh，而是统一放到一个 render mesh 中，避免 vbo 过多导致的性能问题。 几何同理进行优化。
3. 检查 mesh document 构建数据是否合理尤其  gmshTypeToMeshElementType 接口，确保所有的 gmsh 元素类型都能正确转换为 mesh document 中的元素类型。
4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。
