软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息，请按如下要求实现：

1. 设计Geometry 层，支持管理 occ 几何实体，设计 geometry manager 单例工厂(参考 model reader 写法) 管理实体 document，document 则通过 entity index 持有当前模型的 entities：
    - 链接 reader 模块，完成 brep stp 文件的读取，并将读取到的几何信息转换为 geometry 层的几何实体进行管理，并且要设置好 geometry entity 层的父子关系。
2. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
3. 完成前边修改后，仔细阅读项目中所有 qml cpp 代码，思考实现软件主题功能的前提下，代码是否足够清晰，重构不合理的代码以及架构。
4. 为所有代码完善注释，并确保代码风格统一，编写单元测试覆盖主要功能点，确保代码质量, 代码风格参考 clang-format clang-tidy。
5. 生成的代码所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
6. 保证最终代码可以编译通过，并正确执行。
