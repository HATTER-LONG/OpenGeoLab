# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务


1. 按要求完善 GeoQueryPage.qml 页面：
   - Selector 拾取组件每个 Entity button 的 icon 有些太靠上边缘了，而且每个button 宽度不一样，建议统一宽度，icon 居中显示。
   - selector 要支持父页面设置那些 entity type 可以显示，哪些不显示，例如当前只显示 Face，那么 Part、Edge、Vertex 都不显示。
   - 渲染层需要将几何的 entity uid + type 信息编码到 opengl 的颜色 buffer 中，方便拾取时通过读取像素颜色获取 entity uid + type。注意拾取鼠标周围一个小区域的像素，取出符合 type 类型的 id，实现吸附拾取的效果，当鼠标左键点击通过信号告诉控件拾取的 type + id，鼠标右键取消拾取。不要简单实现一个简单的 control，要实现类似 CAD 软件的拾取效果。
   - 渲染层通过信号将拾取到的信息推送给 Selector 组件，Selector 组件根据传入的 type + uid 进行显示当前选中了那些。
   - GeoQueryPage.qml 页面主要通过日志打印下 Selector 拾取的 entity 信息（通过 query 接口获取到的），后续会完善显示 entity 详细信息的面板。
   - 注意所有的实现不要破坏现有的架构设计，尽量使用 json 协议进行模块间通信，不要随意新增 qt service 组件与 qml 通信。
2. 几何圆柱、圆环、圆渲染锯齿比较多 而写 edge 显示不全的问题需要解决，确保圆柱、圆环、圆显示效果良好。
3. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
4. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
5. 保证最终代码可以编译通过，并正确执行。
