# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务

1. 当拾取类型是 part 或者 solid 则自动查找当前子类型的父类，并对父类所有的面进行选取高亮。
    - 更新 geometry entity 相关组件更好的查找 part solid 父类的方法，优化查找效率；
    - 更新 opengl 视口的拾取逻辑；
2. 构建一个 SelectManagerService 中间层，用于 qml 订阅 SelectManager 的选取变化信号，避免直接在 qml 中调用 SelectManager 静态类方法。
    - 实现 SelectManagerService 类，提供选取相关接口；
    - 在 qml 中使用 SelectManagerService 订阅拾取数据以及修改删除 entity；
3. 构建一个页面参考 GeoQueryPage 以及独立控件 Selector 用于实现几何拾取功能:
    - 实现 Selector 组件，点、线、面可以多选，solid part 只能单选；
    - 实现 GeoQueryPage 页面，集成 Selector 组件，当前执行体仅仅打印选择的信息即可；
4. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
5. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
6. 保证最终代码可以编译通过，并正确执行。
7. 为可以的代码单元编写单元测试代码，确保代码质量。
