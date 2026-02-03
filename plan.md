# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务

1. 将 add point、 add line 删除改为添加圆柱体与球体与圆环三种几何体，注意修改相关 UI 界面、添加 icon 并完善后端 create action 逻辑;
2. 按要求完善 GeoQueryPage.qml 页面：
   - 首先实现其核心控件：Selector.qml，其支持 支持点、边、面、体、part 拾取，并显示当前拾取的 type:uid 可以拾取多个 ，当激活 select 控件后(有颜色变化)，使得 opengl view port 进入拾取模式。
   - 渲染层需要将几何的 entity uid + type 信息编码到 opengl 的颜色 buffer 中，方便拾取时通过读取像素颜色获取 entity uid + type。注意拾取鼠标周围一个小区域的像素，取出符合 type 类型的 id，实现吸附拾取的效果，当鼠标左键点击通过信号告诉控件拾取的 type + id，鼠标右键取消拾取。
   - 实现 GeoQueryPage.qml 页面 UI 布局，包含选择类型的 selector 控件，主要功能打印下 当前拾取的对象信息，通过 query 接口。
   - 注意美化控件与页面布局。
3. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
4. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
5. 保证最终代码可以编译通过，并正确执行。
