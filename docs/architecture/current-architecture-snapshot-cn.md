# OpenGeoLab 当前架构快照与下一步计划

## 1. 当前稳定调用链路

这次提交之后，仓库的稳定边界已经不再是“占位模型 + 零散入口”，而是统一的请求协议与 action 分发骨架：

- QML UI -> `ActionRegistry.qml` / `RibbonConfig.qml` / `FeaturePageBase.qml` -> `OpenGeoLabController.runServiceRequest()` 或 `submitServiceRequest()` -> `CommandRecorder` / `CommandService` -> `ComponentRequestDispatcher` -> `<module>` service -> `<Action>` factory -> domain action
- 内嵌 Python -> `opengeolab_app.process(request)` -> `OpenGeoLabController` -> 同一条 command / dispatcher / module-action 链路
- 外部 Python -> `opengeolab.process(request)` 或 `OpenGeoLabPythonBridge.process(request)` -> `CommandService` -> 同一条 module-action 链路
- 运行时日志 -> `ModuleLogger` / `AppLogger` -> `QmlSpdlogSink` -> `OperationLogService` / `OperationLogModel` -> Activity Center 的 Events 页
- 请求与脚本输出 -> controller 的 `lastRequest` / `lastResponse` / `lastPythonOutput` -> Activity Center 的 Command Line 页

当前链路的价值不在于 OCC、Gmsh 或真实 viewport 已经落地，而在于 UI、内嵌 Python、外部 Python、日志反馈和脚本导出终于共享同一套请求与响应边界。

## 2. 当前核心协议与模块职责

### 2.1 请求协议

仓库当前以统一 JSON 信封作为服务边界：

```json
{
  "module": "geometry",
  "action": "createBox",
  "param": {
    "modelName": "Box_001",
    "origin": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "dimensions": { "x": 120.0, "y": 80.0, "z": 60.0 }
  }
}
```

`libs/core` 中的 `ServiceRequest` / `ServiceResponse` 已经把 `module`、`action`、`param`、`payload` 固化为公共协议。`param` 必须是 object；dispatcher、controller 和 Python bridge 都会在入口处做同样的校验。

### 2.2 模块职责

| 模块 | 当前职责 |
| --- | --- |
| `apps/OpenGeoLabApp` | 组装 QML 壳层、通用 controller、内嵌 Python runtime、Activity Center、运行时语言切换。 |
| `libs/core` | 定义 `ServiceRequest` / `ServiceResponse` / `ProgressCallback`、模块 dispatcher，以及统一 module logger 基础设施。 |
| `libs/command` | 负责模块注册引导、请求执行、record / replay、Python 脚本导出。 |
| `libs/geometry` | 暴露 `geometry` service，并通过 action factory 注册 `createBox`、`createCylinder`、`createSphere`、`createTorus`、`inspectModel`。 |
| `libs/scene` | 暴露 `scene` service，并提供 `buildScene` action 与稳定的 `SceneGraph` 结果。 |
| `libs/render` | 暴露 `render` service，并提供 `buildFrame` action 与稳定的 `RenderFrame` 结果。 |
| `libs/selection` | 暴露 `selection` service，并通过 `pickEntity`、`boxSelect` action 把 geometry -> scene -> render 的结果收束成 `SelectionResult`。 |
| `python/python_wrapper` | 对外提供 `opengeolab` pybind 模块，但内部只复用共享 `CommandService`，不再另起一套业务协议。 |

## 3. 当前设计判断

### 3.1 controller 只暴露通用协议，不承载业务入口

`OpenGeoLabController` 现在的稳定定位是“通用 request 适配器”，而不是几何、选择、渲染业务的控制器。

- QML 负责组装 `{module, action, param}`
- controller 负责校验、执行、记录、进度反馈和公共结果缓存
- 业务语义留在 module service 和 concrete action 中

这意味着以后新增用户可见能力时，优先新增 action 与 requestSpec，而不是在 controller 上继续堆 `createBox()`、`pickEntity()` 这类专有接口。

### 3.2 module service 已经稳定为 action factory 分发模式

`geometry`、`scene`、`render`、`selection` 四个模块都已经从 placeholder service 演进为一致的模式：

- service 只负责校验 `module` 与 `action`
- `ComponentRegistration.cpp` 统一注册 service 与 concrete action factory
- concrete action 负责参数归一化、进度上报、结果构建和日志输出

这次重构真正稳定下来的不是某个单独的选择实现，而是“所有 domain module 都通过 action component 暴露能力”的架构约束。

### 3.3 QML 壳层已经形成 action registry + feature page 结构

`Main.qml` 已经收敛为状态装配与信号转发层，真正的页面与行为定义开始外移：

- `ActionRegistry.qml` 维护 action 元数据、页面标题、摘要、workflowKind 和 requestSpec
- `RibbonConfig.qml` 维护 ribbon tab / group / action 的展示组织
- `FeaturePageBase.qml` 提供统一的浮层工作面板骨架
- `GeometryCreateFeaturePage.qml` + `GeometryCreatePageLogic.js` 负责复杂表单的校验、派生指标和 request 构建

这意味着后续扩展 QML 时，优先复用 registry、base page 和 sidecar JS 逻辑，而不是回到 `Main.qml` 中堆长篇内联逻辑。

### 3.4 Activity Center 已经成为统一反馈面

这次提交把“请求结果”“Python 输出”“模块 logger”三条反馈流收敛到了同一个 UI 面：

- Events 页显示 `OperationLogService` 捕获的运行时日志
- Command Line 页显示最近一次 request / response，以及 Python 命令输出
- 运行时日志级别与面板内可见性过滤已经分离，避免把“是否发日志”和“是否显示日志”混成一个开关
- `submitServiceRequest()` 已经具备异步请求与进度通知路径，overlay 与 Activity Center 可以共享反馈信息

这使得 Activity Center 不再只是装饰面板，而是当前 UI / automation / runtime 统一的可观测性入口。

### 3.5 日志、本地化与 Python 自动化已经开始共享统一约束

- `ModuleLogger.hpp` 提供统一 logger 创建与 `OGL_LOG_*` 宏，输出等级、模块名、文件:行号和线程号
- `HeaderMenuPanel.qml` 与 `UiSettingsController` 把主题和语言切换收敛到同一组 workspace 设置入口
- QML 新字符串已经系统性使用 `qsTr()`
- 外部 Python 与内嵌 Python 都已经收敛到 `process(request)` 形式，只是返回类型分别偏向 JSON 文本与 Python 对象

这三条约束共同说明：当前仓库开始把“可脚本化”“可本地化”“可观测”视作同等级架构需求，而不是附属特性。

## 4. 当前仍然缺失的能力与边界

虽然架构骨架已经比上一版清晰得多，但当前实现仍然有明确边界：

- `scene`、`render`、`selection` 仍然是稳定数据流占位实现，不是真实 viewport host、GPU 生命周期与拾取算法
- `mesh` 与 `AI` ribbon 动作当前主要停留在 UI 元数据层，还没有接入对应 module-action 服务
- command 当前已经有 record / replay / export，但还没有真正的 undo / redo 契约
- `runServiceRequest()` 的同步路径仍运行在 UI 线程，复杂请求应优先走 `submitServiceRequest()`
- pybind 对外 API 当前更偏通用桥接层，而不是高层 typed Python workflow API

## 5. 当前实践结论

和之前“UI -> Python bridge -> libs”的旧提示相比，当前方向更符合仓库目标，因为：

- QML、内嵌 Python、外部 Python 共用同一套 request envelope
- domain service 已经不再直接暴露 placeholder 语义，而是暴露工程动作语义
- command recorder 真正成为回放与 Python 导出的中间层
- Activity Center 可以同步呈现运行时日志与请求交换
- QML 壳层已经开始收敛为 registry / config / feature page / overlay 的可维护结构

当前必须持续坚持的纪律是：先定义 module action 与结构化 payload，再向 QML、Python、Activity Center 暴露适配层。

## 6. 推荐下一步顺序

### 阶段 A：把占位 action 替换为真实 domain 后端

在保持 `module + action + param` 协议不变的前提下，把 geometry、scene、render、selection 从占位数据生成逐步替换为 OCC / Gmsh / 真实渲染与拾取实现。

### 阶段 B：扩展 module-action 覆盖面

让 mesh、scene 可见性、相机、选择集等用户可见操作也进入统一 action factory 与 command 记录体系，而不是继续停留在 UI 占位页。

### 阶段 C：把 command history 从 replay-only 推进到可逆契约

在当前 record / replay / export 基础上补齐 undo / redo 所需的命令逆操作与状态边界。

### 阶段 D：继续压实 QML 壳层结构

继续沿着 `ActionRegistry`、`RibbonConfig`、`FeaturePageBase`、`OperationLogPanel` 这条方向拆分页面，减少 `Main.qml` 与大页面组件的职责堆积。

### 阶段 E：完善语言、日志与部署收口

新增 QML 页面、图标、脚本入口与模块日志时，持续保持 `qmldir`、app CMake QML 列表、translation TS 列表和 logger sink 管线同步更新。
