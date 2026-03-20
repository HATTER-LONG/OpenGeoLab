# FreeCAD 启发下的 Render / Selection / Recording 架构分析

## 目标

本文面向 OpenGeoLabNew 当前的轻量骨架，回答三个核心问题：

1. `render` 层与 `selection` 功能应如何分层组织。
2. 如何记录每次 UI 操作的结果，并支持导出 Python 脚本与回放。
3. 在没有 UI 的情况下，这些能力是否仍然可用，以及应如何设计边界。

## FreeCAD 中最值得借鉴的边界

### 1. 视图与相机状态是独立资产

- `FreeCAD\src\Gui\Camera.cpp` 提供了稳定的相机朝向定义，说明相机姿态本身应作为独立可序列化状态，而不是依附于某次鼠标拖拽。
- `FreeCAD\src\Gui\Navigation\NavigationStyle.h` 把 orbit、pan、zoom、box zoom、selection 等交互组织成明确的 viewer/navigation 行为，说明 UI 手势应先收敛成“视图状态变化”或“选择查询”。

### 2. 选择数据与选择可视化是两层

- `FreeCAD\src\Gui\Selection\Selection.h` 中的 `SelectionChanges` / `SelectionObserver` 体现了“选择注册表 + 观察者”的中心化思路。
- `FreeCAD\src\Gui\Selection\SoFCUnifiedSelection.h` 则把 pick、preselect、scene graph 中的可视化选择统一在渲染树里。
- 这意味着真正可复用、可回放的部分不是高亮节点本身，而是“选择语义”和“选择变化事件”。

### 3. 框选是 UI 手势，回放应落到语义查询

- `FreeCAD\src\Gui\MouseSelection.h` 中的 `RubberbandSelection`、`RectangleSelection` 证明矩形框选在实现上天然依赖视口和事件流。
- 但可回放时不应重新播放鼠标像素轨迹，而应记录：
  - 当时的显式视口/相机状态；
  - 选择矩形或对应视锥；
  - 过滤器（如 edge / face / vertex）；
  - 最终选择结果或语义查询参数。

### 4. 宏录制真正记录的是语义命令

- `FreeCAD\src\Gui\Macro.h` 的 `MacroManager` 管的是宏会话、Python 行输出和提交/取消。
- FreeCAD 的稳定回放边界不是原始事件，而是 Python 命令、属性变更、选择语义和文档操作。
- 对 OpenGeoLabNew 来说，这直接对应“导出 Python 脚本时优先导出语义动作序列，而不是鼠标事件日志”。

## 对 OpenGeoLabNew 的分层建议

建议把职责拆成五层，其中前三层应保持纯 C++、无 Qt 依赖：

### 1. `render`

职责：

- 持有视口标识、尺寸、投影类型、相机模型、相机姿态。
- 提供“显式视图状态”的规范化和快照描述。
- 为交互回放提供稳定的 view-state 边界。

不负责：

- 具体鼠标事件处理；
- 最终 selection registry；
- Python 脚本导出。

### 2. `selection`

职责：

- 定义 pick 与 box selection 的查询语义。
- 接收显式视口状态、屏幕框、过滤器等输入并归一化。
- 输出 headless-friendly 的查询描述，例如 ray cast / frustum query。

不负责：

- 记录整条用户会话；
- UI 高亮的瞬时表现。

### 3. `interaction`

职责：

- 把一次 UI 操作转换成可保存的语义记录。
- 区分稳定回放边界与不稳定的原始输入边界。
- 导出 Python 脚本与回放计划。

这层是 OpenGeoLabNew 对 FreeCAD `MacroManager` 思路的直接承接，但它不应该直接依赖 QML 或 Qt 事件对象。

### 4. `command`

职责：

- 继续充当统一 JSON 协议入口。
- 把 `render` / `selection` / `interaction` 的纯领域能力暴露给 QML、嵌入式 Python 和外部脚本。

### 5. `app` / QML

职责：

- 采集用户意图；
- 展示响应结果；
- 触发示例请求与调试入口。

不应承担录制格式设计、Python 脚本生成和选择算法本体。

## 为什么“先旋转模型，再框选 edge 与 face”不能录原始鼠标轨迹

这个场景最容易踩坑：

1. 鼠标旋转修改了相机姿态。
2. 相同像素坐标矩形，在另一视图姿态下对应不同视锥。
3. 因此单独记录“拖拽从 `(120, 100)` 到 `(600, 420)`”不可稳定回放。

正确做法是记录两步：

### 第一步：记录显式视图状态

- `cameraModel`
- `target`
- `distance`
- `azimuth / elevation / roll`
- `viewport width / height`

### 第二步：记录语义框选

- `rectangle`
- `entityKinds = ["edge", "face"]`
- `replace / append`
- `visibleOnly / hiddenAllowed`

回放时不是重放“鼠标按下/移动/抬起”，而是：

1. `restore_viewport(...)`
2. `box_select(...)`

这也是本文新增骨架里 `render.viewport.describe`、`selection.box.describe`、`interaction.export.python` 的设计依据。

## 记录与回放的稳定边界

### 应记录的内容

- 视口状态快照
- 选择查询语义
- 过滤器
- 操作结果快照
- 脚本可还原的命令序列

### 不应作为唯一回放依据的内容

- 鼠标像素增量
- 高频 hover / preselect
- 临时 highlight
- 依赖瞬时窗口布局的局部坐标

## 无 UI 时是否可用

可以，但前提是把 UI 依赖与领域逻辑分开：

### 无 UI 仍应可用的部分

- 视口状态规范化
- 选择查询描述
- 交互记录项生成
- Python 脚本导出
- 回放计划生成

### 需要真实 UI 或渲染上下文的部分

- 实时鼠标事件采集
- 悬停高亮 / 预选显示
- 真正的 GPU picking / scene overlay

因此推荐的实现策略是：

- `render` / `selection` / `interaction` 默认做成纯 C++ 领域库；
- QML 只是输入和展示层；
- 未来若接入实际渲染器，可以把实时 picking 放在 UI/renderer adapter 中，再把结果喂给 `selection`。

## 本次骨架实现如何映射这个分析

本次在 OpenGeoLabNew 中新增了三个模块：

- `src/libs/render`
- `src/libs/selection`
- `src/libs/interaction`

对应职责如下：

### `render`

- `RenderService::describeViewport()`
- `RenderService::captureSnapshot()`

用于显式归一化视图状态和构造可回放快照合同。

### `selection`

- `SelectionService::describePick()`
- `SelectionService::describeBoxSelection()`

用于把 pick / box select 表达成可脚本化、可 headless 复用的选择查询。

### `interaction`

- `InteractionRecorder::recordOperation()`
- `InteractionRecorder::exportPythonScript()`
- `InteractionRecorder::describeReplayPlan()`

用于把 UI 操作结果转换成稳定记录项、Python 导出文本和回放计划。

## 新增协议动作

- `render.viewport.describe`
- `render.snapshot.capture`
- `selection.pick.describe`
- `selection.box.describe`
- `interaction.record.operation`
- `interaction.export.python`
- `interaction.replay.describe`

其中最关键的是 `interaction.export.python`：它把“恢复视图状态 + 执行语义选择”导出成具名 Python API 调用，而不是导出鼠标轨迹。

## 推荐的长期落地顺序

1. 先把视图状态、选择语义、记录结构稳定下来。
2. 再接入真实渲染器与 picking backend。
3. 最后再补 UI 层的高亮、预选、录制面板和脚本管理界面。

这样做的好处是：

- 领域层可以先在无 UI 环境下测试；
- Python 导出与回放无需等待完整渲染器；
- 未来无论使用 Qt Quick 3D、原生 OpenGL 还是其他渲染方案，协议与录制格式都更稳定。
