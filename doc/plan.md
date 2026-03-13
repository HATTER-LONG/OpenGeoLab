# OpenGeoLab 当前架构与演进计划

## 1. 当前工程状态

当前仓库已经完成模块化骨架迁移：

- apps/OpenGeoLabApp
- libs/core
- libs/geometry
- libs/mesh
- libs/scene
- libs/render
- libs/selection
- libs/command
- python/python_wrapper

目前已经落地的一条完整垂直链路是：

QML UI -> app controller -> Python bridge -> Kangaroo ComponentFactory -> geometry lib

这条链路的核心价值是先把跨层调用方式、模块边界和脚本化入口稳定下来，而不是先追求几何能力完整。

## 2. 当前模块职责

### 2.1 apps/OpenGeoLabApp

- 宿主 QML UI 和应用级 controller
- 从 QML 接收 module + JSON 请求
- 不依赖 geometry 专用服务接口，而是统一走通用请求分发

### 2.2 libs/core

- 定义 IService 和 ServiceResponse
- 提供 ComponentRequestDispatcher
- 负责通过 Kangaroo ComponentFactory 按 moduleName 解析服务

### 2.3 libs/geometry

- 当前提供 PlaceholderGeometryModel
- 通过 registerGeometryComponents 注册 geometry 服务
- 后续将替换为 OCC 驱动的 GeometryModel、导入、拓扑和清理能力

### 2.4 python/python_wrapper

- 提供 OpenGeoLabPythonBridge
- 同时服务于 app 层和 pybind11 模块
- 统一高层 call(module, params) 入口，避免 UI 链路和 Python 链路各自演化

### 2.5 libs/scene

- 负责把几何占位模型组织成稳定的 scene graph 语义层
- 为 render 和 selection 提供节点 ID、显示标签、可选中对象集合

### 2.6 libs/render

- 负责把 scene graph 转换为占位 RenderFrame
- 承载 viewport 尺寸、相机姿态、draw item 和高亮状态等渲染侧数据

### 2.7 libs/selection

- 负责基于 RenderFrame 和 scene graph 生成占位拾取/框选结果
- 当前已可用单次请求串起 geometry -> scene -> render -> selection 的核心数据流

## 3. 对 QML + OpenGL + 鼠标交互 + Python 脚本记录的架构评估

结论：当前框架方向是合理的，scene、render、selection 的占位链路已经打通；下一步瓶颈已经收敛到真正的 OpenGL 视口宿主、命令系统接管和脚本记录语义化。

### 3.1 当前框架为什么方向正确

- UI 没有直接耦合几何内核，而是经 controller 和服务调用进入后端。
- Python bridge 已经是高层自动化入口，天然适合脚本回放与 LLM 生成调用。
- Kangaroo ComponentFactory 提供统一的 module 字符串分发模型，便于 geometry、mesh、scene、selection、command 按同一入口接入。
- render 被明确要求与 geometry/mesh 内核解耦，这是 OpenGL 视图、拾取和缓存管理的正确前提。

### 3.2 当前还缺的关键能力

- render 模块还没有真实的 OpenGL 视图宿主与 GPU 资源管理层。
- scene 模块虽然已有占位 scene graph，但还没有成为完整的显示对象、可见性、选择状态唯一真值来源。
- selection 模块虽然已有占位 pick/box-select 语义，但还没有真正接入射线拾取、ID buffer 和屏幕空间框选算法。
- command 模块还没有接管用户可见操作，因此 undo/redo 和脚本录制都还无法真正成立。

### 3.3 推荐的交互分层

建议采用以下职责分层：

1. QML 层
   - 只负责界面、工具状态、事件转发
   - 不做几何、拾取和渲染算法

2. app / InteractionController
   - 接收鼠标按下、移动、释放、滚轮等事件
   - 统一转换为 orbit camera、pick face、box select 等语义请求

3. scene 模块
   - 保存 scene node、可见性、激活状态、选择集
   - 作为 render 与 selection 的共同语义中间层

4. render 模块
   - 消费 scene 导出的 RenderData
   - 管理 GPU buffer、camera、render pass、highlight pass
   - 不直接访问 OCC 或 Gmsh 对象

5. selection 模块
   - 实现 ray cast、GPU picking、rectangle selection
   - 返回实体 ID 或 scene node ID
   - 不直接操作 UI

6. command 模块
   - 把所有用户可见操作落成命令
   - 负责 undo/redo 和 Python script recording 的统一语义入口

### 3.4 关于 Python 脚本记录的关键判断

脚本录制不应该记录“鼠标拖了多少像素”，而应该记录“完成了什么工程动作”。

正确粒度应当是：

- 旋转视角 -> set_camera_pose
- 点击拾取 -> select_face 或 select_entities
- 框选 -> box_select(screen_rect, filter)
- 几何清理 -> geometry.removeSmallFaces(params)
- 生成网格 -> mesh.generateSurface(params)

也就是说，鼠标事件只触发命令，命令再导出 Python 语义调用。不要把 UI 坐标系细节固化进脚本层。

## 4. 当前 CMake 设计判断

当前 CMake 相比最初的单体结构已经明显更合理：

- 根 CMake 管理依赖和全局选项
- 每个模块有独立 CMakeLists.txt
- geometry 测试已迁移到 libs/geometry/tests
- 首方库支持通过 OPENGEOLAB_BUILD_SHARED_LIBS 切换静态/动态构建
- core 与 geometry 已加入生成式 export header，满足 Windows/MSVC 共享库导出规则
- 已增加基础 install/export 规则，为后续发布库和 package config 预留空间

仍需持续坚持的原则：

- 所有公共库模块都必须维护自己的 export header
- 单元测试按模块内聚，不回退到根目录 tests
- 顶层 tests 目录只保留跨模块集成测试

## 5. 下一阶段推荐顺序

### 阶段 A：引入真正的 OpenGL 视图宿主

目标：让 QML 视口可以处理 orbit、pan、zoom 与 redraw。

### 阶段 B：把 command 模块接到用户操作链路

目标：建立 undo/redo 和 Python 脚本录制的统一入口。

### 阶段 C：把 geometry placeholder 替换成 OCC 真实模型

目标：从演示链路升级成真正可编辑、可导入的几何模块。