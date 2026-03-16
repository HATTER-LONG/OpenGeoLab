# OpenGeoLab 架构详解与项目概览

## 1. 文档目的

这份文档不是产品宣传页，也不是单次提交说明，而是基于当前仓库代码现状给出的：

- 项目定位与当前能力说明
- 仓库结构与跨层调用链路说明
- QML -> Python -> core -> libs 的统一协议说明
- 当前实现的主要优势
- 当前代码中仍然存在的冗余、耦合、过长文件与可维护性问题
- 后续整改优先级建议

结合当前代码状态，可以给出一个明确判断：

> **OpenGeoLab 当前架构方向是对的，但还不是最佳实现形态。**  
> 仓库已经建立了统一 request 协议、模块 action 分发、命令记录与 Activity Center 可观测性基础；与此同时，QML 壳层、Python 入口、controller、domain action helper 仍存在明显重复与职责膨胀，需要系统性收敛。

## 2. 项目定位

OpenGeoLab 是一个面向 CAE 前处理场景的软件平台，目标能力包括：

- 几何导入与管理
- 几何创建、清理与修复
- 网格生成与质量检查
- 3D 场景与可视化
- Python 自动化
- 面向 LLM 的流程编排与脚本生成

当前仓库最有价值的部分，不是 OCC / Gmsh / 真实 OpenGL 能力已经完备，而是**跨层边界正在统一**：

- UI 不再走一套、Python 不再走另一套
- request / response 协议开始稳定
- 模块服务通过 action factory 暴露能力
- 记录、回放、导出与运行时日志开始汇聚

## 3. 仓库结构速览

### 3.1 顶层目录

| 目录 | 作用 |
| --- | --- |
| `apps/` | 应用层，当前主要是 `OpenGeoLabApp`，承载 QML 壳层、controller、内嵌 Python runtime 与 Activity Center。 |
| `libs/` | 核心基础与领域模块，包括 `core`、`command`、`geometry`、`scene`、`render`、`selection`、`mesh` 等。 |
| `python/` | pybind11 对外桥接层与 Python 相关测试。 |
| `cmake/` | 构建、安装、部署相关脚本。 |
| `docs/` | 规划、架构与补充说明文档。 |
| `3rd/` | 第三方依赖或其相关配置。 |

### 3.2 当前最关键的代码落点

| 路径 | 当前职责 |
| --- | --- |
| `apps/OpenGeoLabApp/qml/` | 顶层壳层、动作注册表、Ribbon 配置、FeaturePage、Activity Center、主题与分区组件。 |
| `apps/OpenGeoLabApp/src/OpenGeoLabController.cpp` | QML-facing 通用 request 入口、同步/异步调度、命令记录、脚本导出、内嵌 Python 与操作反馈。 |
| `libs/core/` | `ServiceRequest` / `ServiceResponse` / `ProgressCallback`、模块 dispatcher、统一 module logger 基础设施。 |
| `libs/command/` | 模块注册引导、请求执行、record / replay / Python export。 |
| `libs/geometry` / `scene` / `render` / `selection` | 各自的 module service、component registration、action factory 与 placeholder domain pipeline。 |
| `python/python_wrapper/` | 外部 `opengeolab` pybind 模块与 `OpenGeoLabPythonBridge`。 |

## 4. 统一请求协议

当前仓库已经形成统一的 JSON request envelope：

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

对应响应协议在 `libs/core/include/ogl/core/IService.hpp` 中收敛为：

- `success`
- `module`
- `action`
- `message`
- `payload`

这层协议现在已经被以下入口共同复用：

- QML 通过 `OpenGeoLabController.runServiceRequest()` / `submitServiceRequest()`
- 内嵌 Python 通过 `opengeolab_app.process(request)`
- 外部 Python 通过 `opengeolab.process(request)` 或 `OpenGeoLabPythonBridge.process(request)`
- 内部模块通过 `ComponentRequestDispatcher`

### 4.1 当前协议的优点

- UI、Python、module service 终于共享同一套调用边界
- 后续 command record / replay / export 可以围绕同一请求结构展开
- Activity Center 能稳定记录 request / response 与日志反馈

### 4.2 当前协议仍有的实现问题

虽然协议本身是正确的，但**校验与解析逻辑还没有做到单点复用**：

- `OpenGeoLabController.cpp` 内部有 `parseCommandRequest`
- `python/python_wrapper/src/OpenGeoLabPythonBridge.cpp` 也有一套 request 校验
- `python/python_wrapper/src/module.cpp` 与 `apps/OpenGeoLabApp/src/EmbeddedPythonRuntime.cpp` 各自维护了 `parsePythonJsonArgument`

也就是说，**协议统一了，但入口实现还没有完全统一**。这已经是明显的整改点。

## 5. 跨层调用链路

### 5.1 QML 请求链路

当前 QML 侧已经不是把业务逻辑直接塞进 `Main.qml`，而是形成了如下组织方式：

```text
Ribbon / menu click
  -> ActionRegistry.qml 取得 action 元数据
  -> RibbonConfig.qml 决定展示组织
  -> FeaturePageBase / GeometryCreateFeaturePage 展示工作面板
  -> OpenGeoLabController.runServiceRequest() / submitServiceRequest()
  -> CommandRecorder / CommandService
  -> ComponentRequestDispatcher
  -> <module> service
  -> concrete action
```

这条链路的核心优点是：

- QML 可以继续保持“薄表现层”
- controller 可以保持“通用 request adapter”
- 业务动作不再堆在 app 层

### 5.2 内嵌 Python 链路

应用内脚本通过 `EmbeddedPythonRuntime` 暴露的 `opengeolab_app.process(request)` 进入相同 controller / command 路径。

当前优势：

- 内嵌脚本与 QML 操作共享同一命令边界
- Activity Center 可以同时看到 request / response / Python 输出

当前问题：

- `EmbeddedPythonRuntime.cpp` 里保留了一套独立的 Python JSON 参数解析逻辑
- 同步 request 路径仍可能运行在 UI 线程

### 5.3 外部 Python 链路

对外 pybind 模块 `opengeolab` 当前提供：

- `opengeolab.process(request)`
- `OpenGeoLabPythonBridge.process(request)`

这条路径的优点是薄、直接、可脚本导出。

当前问题在于：

- 外部 pybind 返回格式化 JSON 文本
- 内嵌 Python 返回 Python 对象

这意味着**调用形态统一了，但返回形态还不完全一致**。短期可接受，长期应考虑是否要进一步统一 developer experience。

### 5.4 运行时反馈链路

当前运行时反馈已基本汇聚：

```text
ModuleLogger / AppLogger
  -> QmlSpdlogSink
  -> OperationLogService / OperationLogModel
  -> Activity Center / Events

lastRequest / lastResponse / lastPythonOutput
  -> Activity Center / Command Line
```

这使得 Activity Center 成为真正的运行时观测入口，而不是静态展示面板。

## 6. 模块职责矩阵

| 模块 | 当前职责 | 当前状态判断 |
| --- | --- | --- |
| `apps/OpenGeoLabApp` | QML shell、controller、内嵌 Python、Activity Center、语言切换 | 功能完整，但 controller 和部分 QML 页面已偏大。 |
| `libs/core` | request/response 协议、dispatcher、logger 基础设施 | 基座设计基本正确，适合作为稳定核心。 |
| `libs/command` | 执行、记录、回放、导出 Python | 方向正确，但仍可进一步抽公共 helper。 |
| `libs/geometry` | `geometry` service 与 `createBox` / `createCylinder` / `createSphere` / `createTorus` / `inspectModel` | action 化完成，但参数规范化与 Python 生成 helper 有重复。 |
| `libs/scene` | `buildScene` 与 `SceneGraph` | 作为 placeholder chain 一环合理，但与上下游类型耦合较强。 |
| `libs/render` | `buildFrame` 与 `RenderFrame` | 同上。 |
| `libs/selection` | `pickEntity` / `boxSelect` 与 `SelectionResult` | 当前调用链清晰，但 public header 对 scene/render concrete type 依赖较重。 |
| `python/python_wrapper` | 对外 pybind bridge | 薄桥接思路正确，但解析逻辑重复。 |

## 7. 当前实现做得好的地方

### 7.1 统一协议方向是对的

相比旧的“UI 一套、Python 一套、内部一套”思路，当前 `module + action + param` request envelope 是明显进步。

### 7.2 action factory 模式已经稳定

`geometry`、`scene`、`render`、`selection` 都已经完成：

- module service
- action factory
- concrete action
- component registration

这套模式对于后续替换 placeholder backend 非常关键。

### 7.3 命令记录与 Python 导出已经成为中间层

`CommandRecorder` 不再只是测试工具，而是开始承担：

- record
- replay
- export script

这对于未来 undo / redo、automation 和 LLM orchestration 都是正确前置。

### 7.4 QML 壳层开始形成可维护结构

当前已经出现一条比较健康的 QML 收敛路径：

- `ActionRegistry.qml`
- `RibbonConfig.qml`
- `FeaturePageBase.qml`
- `GeometryCreatePageLogic.js`
- `OperationLogPanel.qml`

说明项目已经有明确的“页面编排层”和“通用工作面板层”。

### 7.5 可观测性基础明显加强

`ModuleLogger` + `QmlSpdlogSink` + `OperationLogService` 让日志真正进入 UI，这对工程软件非常重要。

## 8. 当前最主要的问题

## 8.1 P0：QML 热点文件过长，职责混合明显

当前最突出的 QML 文件热点：

| 文件 | 行数 | 主要问题 |
| --- | ---: | --- |
| `apps/OpenGeoLabApp/qml/components/OperationLogPanel.qml` | 848 | Events、Command Line、过滤器、滚动条、状态提示全部耦合在一个文件内。 |
| `apps/OpenGeoLabApp/qml/components/GeometryCreateFeaturePage.qml` | 373 | 表单、异步提交、状态提示、派生指标与大量 UI 段落混写。 |
| `apps/OpenGeoLabApp/qml/ActionRegistry.qml` | 330 | 大量 action 元数据集中堆叠，扩展性和可读性开始下降。 |
| `apps/OpenGeoLabApp/qml/components/FeaturePageBase.qml` | 302 | 通用面板骨架已偏大，继续加行为会变成新的巨石文件。 |
| `apps/OpenGeoLabApp/qml/Main.qml` | 245 | 虽然比以前清晰，但仍承担 feature page 管理、状态派生与页面路由。 |

这说明：

- QML 已经开始模块化，但**还没有拆到足够细**
- 继续加需求时，如果不主动拆，会再次回到大文件堆积状态

## 8.2 P0：`OpenGeoLabController` 过于肥大

`apps/OpenGeoLabApp/src/OpenGeoLabController.cpp` 目前达到 **775 行**，同时负责：

- request 解析
- 同步执行
- 异步执行
- command recorder 状态更新
- script export
- embedded Python
- operation feedback
- operation log 追加
- QML property 同步

这已经明显超出“thin controller”的理想边界。  
当前它仍然能工作，但不是最佳实现，应尽快朝以下方向拆分：

- request parsing / execution helper
- operation feedback coordinator
- Python runtime / script export helper
- QML-facing state adapter

## 8.3 P0：重复 helper 已经开始形成维护负担

当前最明显的重复点：

### Python / request 解析重复

- `python/python_wrapper/src/module.cpp`
- `apps/OpenGeoLabApp/src/EmbeddedPythonRuntime.cpp`
- `python/python_wrapper/src/OpenGeoLabPythonBridge.cpp`
- `apps/OpenGeoLabApp/src/OpenGeoLabController.cpp`

这几处共同维护 request 解析、Python object -> JSON 转换、`module/action/param` 校验。

### action helper 重复

- `libs/geometry/src/GeometryActionUtilities.hpp`
- `libs/selection/src/SelectionActionUtilities.hpp`
- `libs/scene/src/BuildSceneAction.cpp`
- `libs/render/src/BuildFrameAction.cpp`

都在重复实现：

- `reportProgress`
- `cancellationResponse`
- `equivalentPython` 模板生成

这些重复在短期内还能忍，但已经进入“后面一定会踩坑”的阶段。

## 8.4 P1：QML 颜色与 action 元数据存在重复来源

### 颜色映射重复

当前至少存在三套近似逻辑：

- `FeaturePageBase.qml::resolveAccentColor`
- `GeometryCreateFeaturePage.qml::resolveAccentColorByName`
- `HeaderRibbonGroup.qml::accentColor`

这意味着新增 accent 或调整映射规则时，需要多处同步修改。

### action 元数据重复

`ActionRegistry.qml` 负责：

- action 摘要
- icon
- accent
- requestSpec

而 `RibbonConfig.qml` 又重复维护：

- action key
- icon
- accentOne / accentTwo

换句话说，**一个 action 的“展示元数据”当前有两个来源**。这不是最佳实现，后续应让 `ActionRegistry` 成为单一事实来源，RibbonConfig 只描述分组与布局。

## 8.5 P1：module registration 样板代码过多

`geometry` / `scene` / `render` / `selection` 四个 `ComponentRegistration.cpp` 文件都在重复：

- supported action 列表
- unsupported response
- 内联 `Service` 类
- 内联 `ServiceFactory` 类
- 近似相同的 dispatch 流程

这说明当前模式是对的，但**还没有抽出公共支撑层**。  
后续如果 action 数量继续增长，这部分样板会持续放大。

## 8.6 P1：placeholder pipeline 的 public type coupling 需要提前警惕

当前 public header 中存在明显线性依赖：

- `SceneGraph.hpp` 依赖 `GeometryModel.hpp`
- `RenderFrame.hpp` 依赖 `SceneGraph.hpp`
- `SelectionResult.hpp` 依赖 `SceneGraph.hpp` 与 `RenderFrame.hpp`

对当前 placeholder chain 来说，这个线性依赖**并非立即错误**，因为它准确表达了当前数据流。  
但如果未来切到真实 OCC / render / picking backend，这种 concrete type coupling 会迅速放大维护成本。

因此更准确的判断不是“现在必须全部改成 JSON”，而是：

> **当前线性依赖作为占位阶段是可接受的，但在真实后端替换前，必须收敛到更窄的 DTO 或接口边界。**

## 8.7 P1：当前还不是“最佳实现”的几处明确证据

如果用“是否已经是最佳实现与最佳编码规范”来评价，当前答案应当是：

**不是。**

原因不是代码质量差，而是以下热点已经明确说明还有可收敛空间：

- QML 大文件仍偏多
- controller 过重
- request / Python 解析逻辑重复
- action helper 重复
- action metadata 双来源
- module registration 样板代码重复

## 9. 当前缺口与边界

除了可维护性问题，仓库当前还存在几个“能力边界”：

- `mesh` 与 `AI` 页面对 UI 元数据支持较多，但后端 module-action 能力尚未完全对齐
- `scene` / `render` / `selection` 目前仍主要是稳定 placeholder data flow，不是真实 viewport host、GPU lifecycle 与 picking 算法
- command 已支持 `record / replay / export`，但没有完整 `undo / redo`
- 同步请求路径仍可能落在 UI 线程

## 10. 推荐整改顺序

### P0：先消除“明显重复 + 明显肥大”问题

1. 提取 shared request parsing / validation helper  
   目标：controller、embedded Python、external Python 共享一套入口校验。

2. 提取 shared action helper  
   目标：统一 `reportProgress`、`cancellationResponse`、`equivalentPython` 生成模板。

3. 拆分 `OpenGeoLabController`  
   目标：保留通用 QML adapter，移出 request 执行与 operation feedback 细节。

4. 拆分 QML 巨石文件  
   优先级顺序建议：
   - `OperationLogPanel.qml`
   - `GeometryCreateFeaturePage.qml`
   - `ActionRegistry.qml`

### P1：再处理“扩展性与单一事实来源”

5. 让 action 元数据单点收敛  
   目标：`ActionRegistry` 成为 action 元数据唯一来源，`RibbonConfig` 只维护布局关系。

6. 把颜色映射收敛到 `AppTheme` 或共享 utility  
   目标：统一 accent 解析与 tint 角色。

7. 给 module registration 抽公共支撑层  
   目标：降低 `ComponentRegistration.cpp` 样板复制。

8. 为真实后端替换预留更窄边界  
   目标：在进入真实 OCC / render / selection 实现前，把 placeholder chain 的 concrete type coupling 缩到合理范围。

### P2：能力成熟后再推进

9. 把 command history 从 replay-only 推进到 undo / redo

10. 统一 external / embedded Python 的返回体验

11. 在 mesh / AI 等工作流中复用统一 action factory + command 路径

## 11. 新同学快速理解仓库的阅读顺序

如果要快速理解当前 OpenGeoLab，建议按下面顺序阅读：

1. `docs/architecture/current-architecture-snapshot-cn.md`  
   先了解当前阶段性架构判断与下一步规划。

2. `apps/OpenGeoLabApp/qml/Main.qml`
   了解壳层状态装配与主页面结构。

3. `apps/OpenGeoLabApp/qml/ActionRegistry.qml`  
   理解用户可见动作集合。

4. `apps/OpenGeoLabApp/src/OpenGeoLabController.cpp`
   理解 QML / Python 如何进入统一 request 路径。

5. `libs/core/include/ogl/core/IService.hpp`  
   看清 request / response 协议。

6. `libs/command/src/CommandService.cpp`
   理解 command 层如何调起 module service。

7. `libs/<module>/src/*ComponentRegistration.cpp`
   理解 module service 与 action factory 的注册和分发。

## 12. 结论

当前 OpenGeoLab 代码库已经有一条值得继续坚持的主线：

- **统一 request 协议**
- **统一 command 边界**
- **统一 module action 暴露方式**
- **统一 Activity / log 可观测性**

这几条主线使它具备继续演进成真正 CAE 前处理平台的基础。

但如果问“当前是否已经是最佳实现与最佳编码规范”，答案仍然是否定的。  
真正需要尽快整改的，不是推倒重来，而是围绕下面四件事继续收敛：

1. 消除重复 helper 与重复 request 解析
2. 拆分肥大的 controller 与 QML 页面
3. 让 action 元数据与主题映射拥有单一事实来源
4. 在真实 backend 接入前，提前收窄 placeholder pipeline 的 concrete type coupling

配合 `docs/architecture/current-architecture-snapshot-cn.md` 中的阶段规划来看，当前仓库已经从“搭骨架”进入“压实边界与可维护性”的阶段。
