# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 参考现在工程已有的 Geometry service 或者 render service 创建一个 Mesh service 模块在 Mesh 目录中：
    - 添加一个 Action，支持通过传入的 geometry entities 通过 GMsh 进行网格剖分，返回剖分后的 mesh entities 列表
    - 支持传入的 entity type 包括 面、part
    - mesh service 需要支持通过 geometry entity 对象进行网格剖分
    - 创建一个 mesh 对象数据管理类，支持存储 mesh entity 对象，并且支持通过 element id 等获取对应的 mesh entity 对象
    - 支持渲染显示 mesh element 对象
2. 重构 GenerateMeshPage 页面，支持选择 geometry entity face\solid\part 进行网格剖分，并且可以设置网格剖分参数，例如全局网格尺寸等，剖分结果在下方列表显示 mesh entity id 以及 mesh 详细信息。
3. geometry service 需要将 geometry entity 类型暴露出来，并且 document 应当支持通过 entity id 以及 uid + type 获取对应的 geometry entity 对象，便于 gmsh 获取 occ shape 进行网格剖分。
4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。
7. 为可以的代码单元编写单元测试代码，确保代码质量。
