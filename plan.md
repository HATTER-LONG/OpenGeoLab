# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 优化下 SceneRender 中的 void SceneRenderer::renderMeshes 接口，复杂度太高了，请进行整理模块化拆分，去除冗余代码。
2. 完成 Select qml 控件，支持父类页面设置支持那些 entity type 可选(对应页面上只显示允许的type，并且按钮宽度自适应拉伸居中)，并且有接口以及信号获取当前选择的 entity 列表。
3. 完善 Geo Query 页面，设置 select 控件支持 点、线、面、wire、part 的拾取，并且可以通过按钮进行查询，查询结果在下方列表显示 entity id 以及 entity 详细信息。
    - Geometry 中增加一个 query_entity_info action，支持获取传入的 entities 的信息。
4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。
7. 为可以的代码单元编写单元测试代码，确保代码质量。
