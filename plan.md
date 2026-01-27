软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息，请按如下要求实现：

1. 完善 Geometry 层：
   - 链接 reader 模块，可以加载 brep stp 文件到 Geometry 层中，由 document 管理加载的几何实体。为所有的 shape 单独创建 entity 并设置好 parent child 关系由 document 保存，单独把 part 由于经常使用可以单拎出来快速索引，part 作为顶层容器，面向 ui 以及渲染提供接口。不同 part 提供不同的颜色显示。
2. 参考 #file:model_reader.hpp  帮我实现一个GeometryService，给这些 Page 增加功能，
   - 支持创建几何 append 加入到已有的 document 中，或者 reset 一个新的 document，New Model 按钮功能实现出来，直接 new document；
   - 所有的功能都放到 Geometry service 中不太好将代码模块化拆分便于后续扩展，例如分为 geometry create 组件 edite 组件等；
3. 完成前边修改后，仔细阅读项目中所有 qml cpp 代码，思考实现软件主题功能的前提下，代码是否足够清晰，重构不合理的代码以及架构。
4. 为所有代码完善注释，并确保代码风格统一，编写单元测试覆盖主要功能点，确保代码质量, 代码风格参考 clang-format clang-tidy。
5. 生成的代码所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
6. 保证最终代码可以编译通过，并正确执行。
