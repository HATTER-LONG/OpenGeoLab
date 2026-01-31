# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务

1. 了解当前实现，按如下要求完成开发：
   - Geometry 对外的信号应当允许 geometry manager 控制，当 new document 时可以直接创建一个 document 并继承之前订阅者，完善 new model 功能。
   - 简化 opengl_viewport，只保留与 qml 交互相关接口，渲染的代码逻辑放到 render 中的源码中，自成组件，参考 reader 与 geometry service 组件。
   - 检查所有 cpp hpp 代码，优化日志记录，关键代码处增加日志输出，按照要求增加 trace debug info warning error 等日志输出，不要刷屏、滥用。
2. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
3. 完成前边修改后，仔细阅读项目中所有 qml cpp 代码，思考实现软件主题功能的前提下，代码是否足够清晰，重构不合理的代码以及架构。
4. 保证最终代码可以编译通过，并正确执行。
