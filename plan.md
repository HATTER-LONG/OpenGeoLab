# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务


1. 重构 pick 功能 getpixel 时通过传入的 type 生成一帧只有目标类型的 render data 来进行拾取，减少 render data 中冗余信息的存储。优化 render 相关数据结构，当前太混乱了，并且有很多冗余的，请进行精简去除，保证代码整洁，并且效率高效。
2. 梳理 render signal 相关，为 mesh 增加新的 signal，使得剖分完自动刷新 mesh 显示, 去除冗余的signal。
3. mesh element 拾取功能区分 line、element 类型，支持拾取 mesh line 或者 其他三角网格等等，同一个 line 被多个 element 引用时根据当前鼠标位置获取这个 line 所属的 element。
4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。