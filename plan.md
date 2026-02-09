# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 重构 entity index 模块，内部查询应当使用 Entity ID 作为下标在 vector 中进行查询，避免 map 带来的性能损失。
    - 考利如何重构 entity index 模块，设计新的数据结构
    - Geometry Entity 尽量不要使用 Shared Pointer 进行管理，改为直接使用对象存储在 vector 中，通过 Entity ID 进行索引查询。
    - 返回 Geometry Entity 时，使用引用进行返回，避免拷贝开销
    - 实现新的 entity index 模块
    - 修改相关调用代码
4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。
7. 为可以的代码单元编写单元测试代码，确保代码质量。
