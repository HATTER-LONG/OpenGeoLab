软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息，请按如下要求实现：

1. 了解当前试下，完成 Geometry 层功能开发：
    - 链接 reader 模块，完成 brep stp 文件的读取，加载到 document 进行管理, document 是模块间的接口，不要直接使用 src 里的头文件。
    - document 以 part entity 作为根，支持获取离散数据用于 opengl 渲染（每个 part 不同颜色）。
    - document支持提供几何元数据用于网格剖分。
    - 支持 create box 等 append shape 到现有 document。
    - 支持 entity 反查所属得 part、face、edge 等等，例如有一个 edge entity，我想要知道他所属那些 face ，可以传入 face type，返回 Face entites 的 vector 合集，注意运行效率。
    - 支持删除 entities 时要联动把相关得子 entities , 再 entity 的析构时 将 occ 形状一并删除。
2. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
3. 完成前边修改后，仔细阅读项目中所有 qml cpp 代码，思考实现软件主题功能的前提下，代码是否足够清晰，重构不合理的代码以及架构。
4. 为所有代码完善注释，并确保代码风格统一，编写单元测试覆盖主要功能点，确保代码质量, 代码风格参考 clang-format clang-tidy。
5. 生成的代码所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
6. 保证最终代码可以编译通过，并正确执行。
