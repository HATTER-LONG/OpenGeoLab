# OpenGeoLab 当前架构快照与下一步计划

## 1. 当前垂直链路

仓库目前已经形成一条可验证交互与自动化边界的占位链路：

- QML UI -> app controller -> command recorder -> command service -> libs
- 内嵌 Python runtime -> opengeolab_app API -> app controller -> command recorder -> command service -> libs
- 外部 Python 模块 -> opengeolab.OpenGeoLabPythonBridge -> command service -> libs

这条链路的价值不是几何能力已经完整，而是用户可见调用路径、记录回放边界、以及 Python 自动化入口已经统一。

## 2. 当前模块职责

| 模块 | 当前职责 |
| --- | --- |
| apps/OpenGeoLabApp | 承载 QML 壳层、应用 controller 和内嵌 Python runtime。 |
| libs/core | 定义 IService、ServiceResponse 与按模块名分发的 dispatcher。 |
| libs/command | 负责命令执行、记录、回放和 Python 导出辅助能力。 |
| libs/geometry | 提供占位几何模型构建和 geometry 服务注册。 |
| libs/scene | 把 geometry 结果整理成稳定的占位 scene graph。 |
| libs/render | 在不暴露几何内核细节的前提下生成占位 render frame。 |
| libs/selection | 基于 scene 与 render 数据给出占位拾取结果。 |
| python/python_wrapper | 提供对外 Python bridge，但内部复用共享 command service。 |

## 3. 应用与自动化设计判断

### 3.1 command 是唯一正确的状态修改入口

用户可见状态变化现在应当统一收敛到 command 层。这是后续能力的基础：

- replay
- 未来的 undo / redo
- Python script export
- LLM 编排

app 层不再需要在语义上依赖 python_wrapper。Python 只是 command 的适配器，不是 UI 的主执行骨架。

### 3.2 内嵌 Python 的定位

应用进程现在内嵌了解释器，并暴露内建模块 opengeolab_app，用于在程序运行时通过 Python API 修改 application 状态。

当前内嵌 API 形态：

- opengeolab_app.run_command(module_name, params)
- opengeolab_app.replay_commands()
- opengeolab_app.clear_commands()
- opengeolab_app.get_state()

这样可以保证运行时脚本与 QML 走同一套 command 和状态模型，而不是额外再做一条旁路。

### 3.3 脚本记录粒度

脚本记录应当保留工程语义，而不是记录偶发 UI 细节。

推荐粒度：

- selection.pickEntity(...)
- scene.setVisibility(...)
- geometry.cleanup(...)
- mesh.generateSurface(...)
- camera.setPose(...)

鼠标移动、悬停、逐帧相机增量，不应作为主要回放产物直接落入脚本层。

## 4. 当前仍然缺失的能力

虽然命令与自动化边界已经更正确，但产品能力仍有明显缺口：

- render 还没有真实 OpenGL viewport host 和 GPU 资源生命周期
- scene 还不是完整的可见性、激活状态、选择状态唯一真值来源
- selection 仍是占位逻辑，还没有真实射线拾取、ID buffer 或框选算法
- command 目前已有 record / replay，但还没有 undo / redo
- 内嵌 Python 目前运行在 app 线程，应继续定位为高层自动化入口，而不是长耗时计算通道

## 5. 当前实践判断

和之前的 UI -> Python bridge -> libs 路径相比，当前方向更符合仓库目标，因为：

- app 到 domain 的状态修改经过 command 边界
- Python 自动化复用同一边界，而不是绕开它
- 导出的 Python 保持可读、可回放
- QML 仍然保持薄表现层角色

当前 UI 壳层也已经形成更明确的组织方向：

- workbench header 应继续拆成可复用的 ribbon group、menu panel、viewport overlay 等子组件
- icon 应统一通过 theme-aware 着色组件输出，避免亮暗主题切换后出现低对比图标
- ribbon 的尺寸、组标题、背景占比应保留工程软件风格，优先服务信息层次和可点击性，而不是装饰性

接下来需要持续坚持的纪律是：先在 command 中落语义，再向 QML 和 Python 暴露适配接口。

## 6. 推荐下一步顺序

### 阶段 A：引入真实视口宿主

把 render 从占位 frame 推进到真正的 OpenGL 视口与渲染资源管理。

### 阶段 B：扩展 command 语义

补充可见性、相机、选择集修改、几何清理和后续网格能力的显式命令。

### 阶段 C：补齐 undo / redo

从 replay-only 的命令历史演进到可逆命令契约。

### 阶段 D：替换占位后端

在保持 command 与自动化边界稳定的前提下，把 geometry、scene、render、selection 逐步替换成 OCC / Gmsh 驱动的真实实现。

### 阶段 E：整理 QML 壳层

继续把过长的 QML 页面拆成 theme、components、sections 三层，降低单文件体积，并沉淀可复用的 ribbon、菜单、状态面板组件。