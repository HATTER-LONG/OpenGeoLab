# 进度 UI 增强 + Sidebar Box 列表 + PySide6 Create Box

日期: 2026-03-24  
状态: Reviewed (Rev 3 — 2 轮审查通过)  
分支: `dev/v2-dev-03-22`

---

## 1. 目标

在现有分层服务架构上，完成三项增强：

1. **进度 UI**：将 ActivityOverlay 中 6px 薄进度条替换为弹出式进度卡片，展示图标、message、百分比、进度条
2. **Sidebar 数据列表**：SidebarPanel 展示 geometry box 列表，支持点击展开详情
3. **PySide6 Create Box**：demo_ui_plugin 增加 "Create Box" 按钮，异步调用 geometry 模块，完成后 QML 侧自动刷新

## 2. 已确认的设计决策

| 决策 | 结论 |
|------|------|
| 数据刷新模型 | **方案 A: JSON Query + SceneStore**（拉模型） |
| 进度 UI 样式 | **弹出式进度卡片**（Activity 按钮上方浮动卡片） |
| PySide6 线程模型 | **threading.Thread + pywrapper.process()** |
| Sidebar 列表样式 | 简洁 box 名称列表，点击展开详情 |

## 3. 架构概览

```
┌──────────────────────────────────────────────────────────────────┐
│                         QML Layer                                │
│                                                                  │
│  ProgressCard ◄── ProgressTracker (hasActiveTasks, progress,     │
│                   currentMessage, taskCompleted signal)           │
│  SidebarPanel ◄── onResponseReady(list_boxes) ──► boxListModel   │
│      ▲              (required property ListModel from Main.qml)  │
│      │                                              ▲            │
│  NotificationService ── "geometry.data_changed" ──► 触发 list 查询│
├──────────────────────────────────────────────────────────────────┤
│                     C++ App Services                             │
│  RequestService │ NotificationService │ ProgressTracker          │
├──────────────────────────────────────────────────────────────────┤
│                     C++ libs/geometry                            │
│  SceneStore ◄── GeometryModule::process(create_box) 写入         │
│  list_boxes action ──► SceneStore.allBoxes() ──► JSON 返回       │
│  "geometry.data_changed" 在 addBox 之后、response 返回之前发出    │
├──────────────────────────────────────────────────────────────────┤
│                     Python / PySide6                              │
│  demo_ui_plugin ── Create Box 按钮 ── threading.Thread           │
│       └── opengeolab_pywrapper.process(json) ──► C++ geometry    │
│           (pywrapper 在进入 C++ 前释放 GIL)                       │
└──────────────────────────────────────────────────────────────────┘
```

## 4. 组件设计

### 4.1 SceneStore（C++ libs/geometry）

新增 `SceneStore` 类，持有 geometry 数据。位于 `libs/geometry`。

```
文件:
  src/libs/geometry/include/opengeolab/geometry/scene_store.hpp
  src/libs/geometry/src/scene_store.cpp
```

接口:

```cpp
namespace OpenGeoLab::Geometry {

/// @brief Thread-safe in-memory store for geometry scene objects.
class OPENGEOLAB_GEOMETRY_EXPORT SceneStore {
public:
    /// @brief Add a box and return its auto-assigned integer ID.
    int addBox(BoxData box);

    /// @brief Return a snapshot of all stored boxes as (id, box) pairs.
    [[nodiscard]] std::vector<std::pair<int, BoxData>> allBoxes() const;

    /// @brief Return the number of stored boxes.
    [[nodiscard]] std::size_t boxCount() const;

    /// @brief Clear all stored boxes.
    void clear();

private:
    mutable std::mutex m_mutex;
    std::vector<std::pair<int, BoxData>> m_boxes;
    int m_nextId = 1;
};

} // namespace OpenGeoLab::Geometry
```

设计要点:
- 线程安全（mutex 保护），PySide6 后台线程和主线程可并发访问
- 返回值快照（vector copy），调用方拿到副本后无锁
- ID 单调递增，不回收

### 4.2 GeometryModule 改为有状态

**迁移策略**: 硬切换。移除旧自由函数 `processGeometry()`，替换为 `GeometryModule` 类。
所有调用方（python_wrapper、测试）同步更新。不提供兼容 wrapper。

```cpp
namespace OpenGeoLab::Geometry {

/// @brief Stateful JSON dispatcher for the geometry module.
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule {
public:
    explicit GeometryModule(SceneStore& store);

    /// @note callback is taken by const& (not by value) so that the caller
    ///       retains ownership of captured py::object and destroys it with GIL held.
    [[nodiscard]] std::string process(std::string_view request_json,
                                      const ModuleProgressCallback& progress_callback = {});

private:
    SceneStore& m_store;
};

} // namespace OpenGeoLab::Geometry
```

新增 action: `list_boxes`

```json
// 请求
{"module": "geometry", "action": "list_boxes"}

// 响应
{
  "ok": true,
  "module": "geometry",
  "action": "list_boxes",
  "result": {
    "boxes": [
      {"id": 1, "label": "Box(1x1x1)", "center": [0,0,0], "size": [1,1,1], "vertexCount": 8},
      {"id": 2, "label": "Box(2x3x1)", "center": [5,0,0], "size": [2,3,1], "vertexCount": 8}
    ],
    "count": 2
  }
}
```

**createBox 完成后的时序**（在 `GeometryModule::process()` 中，不在 `createBox()` 中）:

```
1. createBox() 返回 BoxData
2. GeometryModule::process() 调用 m_store.addBox(box) 存入 SceneStore
3. GeometryModule::process() 调用 NotificationRegistry::sink()->notify("geometry.data_changed", ...)
4. GeometryModule::process() 构建 JSON 响应并返回
```

**关键**: `"geometry.data_changed"` 在 `addBox` **之后**、函数返回**之前**发出。
这确保 QML 收到通知后查询 `list_boxes` 时，数据已存入 SceneStore。

**通知通道合理化**:
- `"geometry.status"` — 保留，用于 create_box 的 started/completed 事件（UI 状态文本）
- `"geometry.progress"` — 保留，由 createBox() 内部的 ProgressCallback 间接驱动
- `"geometry.data_changed"` — **新增**，由 GeometryModule::process() 发出，触发 sidebar 刷新
- 三个通道职责不同，不合并

### 4.3 python_wrapper 更新

`python_wrapper_module.cpp` 需要:
- 持有 `GeometryModule` 实例（内部持有 `SceneStore`）
- `process()` 将 geometry 请求委托给 `GeometryModule::process()`
- **在进入 C++ 计算前释放 GIL**（解决 PySide6 线程阻塞问题）

```cpp
// 模块级静态对象
static Geometry::SceneStore s_sceneStore;
static Geometry::GeometryModule s_geometryModule{s_sceneStore};

// process() 函数体
module.def("process",
    [](const std::string& request_json, py::object progress_callback) -> std::string {
        // 包装 Python callback（需持有 GIL）
        Geometry::ModuleProgressCallback cpp_callback;
        if (!progress_callback.is_none()) {
            cpp_callback = [cb = std::move(progress_callback)](
                               double progress, std::string_view message) {
                py::gil_scoped_acquire acquire;  // 回调时重新获取 GIL
                cb(progress, py::str(std::string(message)));
            };
        }

        // 释放 GIL 再进入 C++ 重计算
        // process() 接受 const& — cpp_callback 的 py::object 留在本作用域
        // 退出 gil_scoped_release 后 GIL 重新持有，此时 cpp_callback 析构安全
        std::string result;
        {
            py::gil_scoped_release release;
            result = s_geometryModule.process(request_json, cpp_callback);  // const&, 不 move
        }
        // GIL 已重新持有，cpp_callback (含 py::object) 在这里安全析构
        return result;
    },
    py::arg("request_json"), py::arg("progress_callback") = py::none());
```

**GIL 安全设计**:
- `py::gil_scoped_release` 在调用 `GeometryModule::process()` 前释放 GIL
- `process()` 接受 `const ModuleProgressCallback&`（不是 by-value），确保 `py::object` 留在调用方
- `cpp_callback` 在 `py::gil_scoped_release` 块退出**之后**析构，此时 GIL 已重新持有，`Py_XDECREF` 安全
- progress_callback 内部使用 `py::gil_scoped_acquire` 重新获取 GIL 来调用 Python
- `createBox()` 同样改为 `const ProgressCallback&` 参数

**SceneStore 生命周期注意**:
- `s_sceneStore` 和 `s_geometryModule` 是 pywrapper 模块的静态对象
- 生命周期等同于进程（Python 嵌入式运行时不会重新初始化）
- 当前为测试阶段可接受。未来如需 C++ app 直接访问 SceneStore（如 3D 渲染），应将 SceneStore 提升到 app 层并通过注入传入 GeometryModule

### 4.4 进度卡片 ProgressCard（QML）

新增组件 `src/app/resource/qml/components/ProgressCard.qml`

视觉规格:

```
┌──────────────────────────────────────────┐
│  ⏳  Creating box…                  45%  │
│  ████████████████░░░░░░░░░░░░░░░░░░░░░  │
│  Processing vertex 45/100…               │
└──────────────────────────────────────────┘
```

- 宽度: 与 ActivityOverlay 同宽（max 320px）
- 高度: ~80px
- 背景: `theme.surface` + `theme.panel.border`
- 圆角: `theme.radiusSmall`
- 左上: 状态图标（ionicons SVG）
  - 运行中: `hourglassOutline`（`theme.accentA` 色）
  - 完成: `checkmarkCircleOutline`（`theme.success` 色）
  - 失败: `closeCircleOutline`（`theme.danger` 色）
- 右上: 百分比文本（`theme.textPrimary`，bold）
- 中间: 进度条（高 6px，圆角，`theme.accentA` 填充色）
- 底部: message 文本（`theme.textSecondary`，font 11px）
- 所有用户可见字符串使用 `qsTr()` 包裹

动画:
- 出现: Behavior on opacity (0→1, 180ms OutQuad)
- 进度条填充: Behavior on width (120ms InOutQuad)
- 隐藏: 完成后 3s / 失败后 6s 自动淡出

**ProgressCard 状态机**:

ProgressCard 需要管理自己的显示状态，不依赖 `statusText` 的文本值判断。
状态来源组合：

| 条件 | ProgressCard 状态 | 图标 | 进度条 |
|------|-------------------|------|--------|
| `hasActiveTasks && progress == 0` | indeterminate | hourglass | 滑动动画 |
| `hasActiveTasks && progress > 0` | determined | hourglass | 填充到 progress% |
| `taskCompleted(id, true)` 刚触发 | done | checkmark | 填满绿色 |
| `taskCompleted(id, false)` 刚触发 | failed | X | 填满红色 |
| 无任务且 hideTimer 未运行 | hidden | — | — |

属性绑定:

```qml
ProgressCard {
    id: progressCard

    required property AppTheme theme

    // 来自 ProgressTracker
    property bool active: ProgressTracker.hasActiveTasks
    property real progress: ProgressTracker.currentProgress
    property string description: ProgressTracker.statusText    // 标题行: "geometry.create_box: ..."
    property string message: ProgressTracker.currentMessage    // 详情行: "Processing vertex 45/100"

    // 内部状态（由 Connections 驱动）
    property string completionState: ""  // "" | "done" | "failed"

    // 修复 M1: 加入 completionState 判断防止完成瞬间闪烁
    visible: active || hideTimer.running || completionState !== ""

    Connections {
        target: ProgressTracker
        function onTaskCompleted(taskId, success) {
            progressCard.completionState = success ? "done" : "failed"
            hideTimer.interval = success ? 3000 : 6000
            hideTimer.restart()
        }
    }

    Timer {
        id: hideTimer
        repeat: false
        onTriggered: progressCard.completionState = ""
    }
}
```

**注**: `description` 绑定 `ProgressTracker.statusText`（现有属性，返回 `"desc: message"` 或 `"desc"`），
`message` 绑定新增的 `ProgressTracker.currentMessage`（只返回 message 部分）。
ProgressCard 内部用 `description` 做标题行、`message` 做详情行。

### 4.5 ActivityOverlay 改造

修改 `ActivityOverlay.qml`:
- 移除旧的 6px progressBar Rectangle 及其关联的 progressHideTimer
- 移除 `progressColor`、`onProgressStatusChanged` 等旧逻辑
- 在 activityPanel 和 activityButton 之间嵌入 ProgressCard
- ProgressCard 拥有自己的 hideTimer（替代旧的 progressHideTimer）

层级:
```
ActivityOverlay
  ├── ProgressCard     ← 新增（自带隐藏定时器）
  ├── ActivityPanel    （展开日志）
  └── activityButton   （Activity 按钮，始终可见）
```

ActivityOverlay 的对外属性简化:
- 移除 `property real progress`（ProgressCard 直接绑定 ProgressTracker）
- 移除 `property string progressStatus`（由 ProgressCard 内部管理）
- 保留 `property bool activityOpen`、`hasNewErrors`、`hasNewLogs`

Main.qml 绑定简化:
```qml
// 旧:
ActivityOverlay {
    progress: ProgressTracker.hasActiveTasks ? ProgressTracker.currentProgress : -1
    progressStatus: ProgressTracker.statusText
}

// 新:
ActivityOverlay {
    theme: appTheme
    // ProgressCard 内部直接绑定 ProgressTracker，不需要外部传入
}
```

### 4.6 SidebarPanel 改造

修改 `SidebarPanel.qml`:

**boxListModel 所有权和传递**:
- `ListModel` 在 `Main.qml` 中声明为 `property alias boxListModel: _boxListModel`
- 通过 `required property ListModel boxListModel` 传入 SidebarPanel
- Main.qml 在 `onResponseReady` 中填充模型

```qml
// Main.qml
ListModel { id: _boxListModel }

SidebarPanel {
    ...
    theme: appTheme
    boxListModel: _boxListModel
}

// SidebarPanel.qml
Item {
    required property AppTheme theme
    required property ListModel boxListModel

    SectionCard {
        title: qsTr("Scene")
        subtitle: qsTr("Explorer")

        Text {
            visible: root.boxListModel.count === 0
            text: qsTr("No geometry objects yet.\nUse Ribbon → Create Box to add one.")
            font.pixelSize: 13
            color: root.theme.textTertiary
            wrapMode: Text.WordWrap
        }

        ListView {
            visible: root.boxListModel.count > 0
            model: root.boxListModel

            delegate: BoxListItem {
                required property string label
                required property int boxId
                required property var center
                required property var size
                required property int vertexCount
                theme: root.theme
            }
        }
    }
}
```

**BoxListItem.qml** — 新增可展开列表项组件:

折叠态: `[🧊 cube-outline] Box(1x1x1)`  
展开态:
```
[🧊 cube-outline] Box(1x1x1)
  Center: (0, 0, 0)
  Size: 1 × 1 × 1
  Vertices: 8
```

数据来源: `boxListModel` 是 QML `ListModel`，通过 `onResponseReady` 填充。

### 4.7 Box 列表刷新流程

```
createBox() 完成
  → NotificationRegistry::sink()->notify("geometry.data_changed", "{}")
  → NotificationService.notificationReceived("geometry.data_changed", "{}")
  → Main.qml onNotificationReceived 中:
      if (channel === "geometry.data_changed") {
          RequestService.submitAsync({module:"geometry", action:"list_boxes"})
      }
  → onResponseReady 中:
      if (resp.module === "geometry" && resp.action === "list_boxes" && resp.ok) {
          boxListModel.clear()
          for (let box of resp.result.boxes) {
              boxListModel.append(box)
          }
      }
```

PySide6 创建的 box 走同样的路径:
```
PySide6 Create Box click
  → threading.Thread → pywrapper.process({create_box})
    → GeometryModule::process() → SceneStore.addBox()
    → notify("geometry.data_changed")
  → NotificationService → QML onNotificationReceived → 自动 list_boxes
```

### 4.8 PySide6 demo_ui_plugin 增强

修改 `plugins/demo_ui_plugin/__init__.py`:

```python
class DemoWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Demo UI Plugin")
        self.setMinimumSize(360, 240)
        self.setAttribute(Qt.WA_DeleteOnClose)

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("Geometry Tool"))

        # Create Box 按钮
        self.create_btn = QPushButton("Create Box")
        self.create_btn.clicked.connect(self.on_create_box)
        layout.addWidget(self.create_btn)

        # 状态标签
        self.status_label = QLabel("Ready")
        layout.addWidget(self.status_label)

        # Close 按钮
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.close)
        layout.addWidget(close_btn)

    def on_create_box(self):
        self.create_btn.setEnabled(False)
        self.status_label.setText("Creating box…")

        def run():
            import json
            import opengeolab_pywrapper as wrapper

            req = json.dumps({
                "module": "geometry",
                "action": "create_box",
                "param": {"vertexCount": 20, "center": [0, 0, 0], "size": [2, 2, 2]}
            })
            result_json = wrapper.process(req)
            result = json.loads(result_json)

            # 线程安全更新 PySide6 UI
            from PySide6.QtCore import QMetaObject, Qt as QtCore_Qt, Q_ARG
            QMetaObject.invokeMethod(
                self.status_label, "setText",
                QtCore_Qt.QueuedConnection,
                Q_ARG(str, f"Created: {result.get('summary', 'done')}")
            )
            QMetaObject.invokeMethod(
                self.create_btn, "setEnabled",
                QtCore_Qt.QueuedConnection,
                Q_ARG(bool, True)
            )

        import threading
        threading.Thread(target=run, daemon=True).start()
```

线程安全:
- `threading.Thread` 在后台运行 C++ geometry 调用
- C++ 内部 `NotificationRegistry` 发送 `"geometry.data_changed"` → QML 自动刷新 sidebar
- PySide6 UI 更新通过 `QMetaObject.invokeMethod(QueuedConnection)` 回到主线程
- `vertexCount=20` 控制延时约 200ms，不影响体验

### 4.9 Ionicons 图标迁移

从 `C:\Users\layton\Desktop\WorkSpace\Project\ionicons-8.0.13\src\svg` 复制以下图标到 `src/app/resource/icons/`:

| 源文件 | 目标文件名 | 用途 |
|--------|-----------|------|
| `cube-outline.svg` | `cubeOutline.svg` | BoxListItem 图标 |
| `hourglass-outline.svg` | `hourglassOutline.svg` | ProgressCard 运行中 |
| `checkmark-circle-outline.svg` | `checkmarkCircleOutline.svg` | ProgressCard 完成 |
| `close-circle-outline.svg` | `closeCircleOutline.svg` | ProgressCard 失败 |

命名规则: 去掉连字符，转为 camelCase（与现有 `AppIcon.qml` 的 `iconKind + ".svg"` 解析一致）。

需要在 `src/app/CMakeLists.txt` 的 QML_FILES 或 resources 中注册这些图标。

## 5. ProgressTracker 增强

### 5.1 现状分析

当前 `ProgressTracker` 的 Q_PROPERTY:
- `hasActiveTasks` (bool) — 是否有活跃任务
- `currentProgress` (double) — -1=无任务, 0=不确定, (0,1]=确定进度
- `statusText` (QString) — **实际返回** `"description: message"` 或 `description`，**不是** "Done"/"Failed"

`statusText()` 实现只遍历非 completed 的任务，因此任务完成后立即变为 `""`。
不存在 "Done"/"Failed" 的返回值——这是旧 ActivityOverlay 中 progressStatus 检查的已知 bug。

### 5.2 新增内容

**新增 Q_PROPERTY:**
- `currentMessage` (QString): 当前活跃任务的 message 文本（如 "Processing vertex 45/100"）
  - 实现: 与 statusText() 类似，但只返回 `message` 部分（不含 description 前缀）

**新增 signal:**
- `taskCompleted(QString taskId, bool success)`: 在 `completeTask()` 中发出
  - 通过 `QueuedConnection` 发到主线程
  - ProgressCard 监听此 signal 来显示 done/failed 状态

```cpp
// progress_tracker.hpp 新增:
Q_PROPERTY(QString currentMessage READ currentMessage NOTIFY progressChanged)

signals:
    void progressChanged();
    void taskCompleted(const QString& task_id, bool success);  // 新增

// progress_tracker.cpp completeTask() 新增:
void ProgressTracker::completeTask(const QString& task_id, bool success) {
    {
        const std::lock_guard lock(m_mutex);
        const auto it = m_tasks.find(task_id);
        if (it == m_tasks.end()) return;
        it->second.completed = true;
        it->second.success = success;
        if (success) it->second.progress = 1.0;
        it->second.lastUpdate = std::chrono::steady_clock::now();
    }
    emitProgressChanged();
    // 新增: 在主线程发出 taskCompleted
    QMetaObject::invokeMethod(this, [this, task_id, success]() {
        emit taskCompleted(task_id, success);
    }, Qt::QueuedConnection);
}
```

### 5.3 PySide6 路径的 progress 说明

PySide6 demo 调用 `pywrapper.process()` 时**不传** `progress_callback`。
因此 PySide6 发起的 create_box **不会**在 ProgressCard 上显示进度——这是有意的设计:
- PySide6 窗口自己显示状态 ("Creating box…" / "Created: Box(2x2x2)")
- QML ProgressCard 只反映通过 RequestService 发起的任务
- sidebar 的 box 列表仍然通过 `"geometry.data_changed"` 通知自动刷新

## 6. 数据流总览

### 6.1 Create Box (从 QML Ribbon)

```
1. User clicks "Create Box" on Ribbon
2. Main.qml → RequestService.submitAsync({module:"geometry", action:"create_box", ...})
3.   → RequestService 内部调用 ProgressTracker.beginTask(requestId, "geometry.create_box")
4.   → EmbeddedPythonRuntime.process(json, cpp_progress_callback)
5.     → cpp_progress_callback 在被调用时 → ProgressTracker.updateProgress(requestId, %, msg)
6.   → Python runtime → pywrapper.process(json, py_callback)
7.     → py::gil_scoped_release → GeometryModule::process()
8.       → createBox(center, size, vertexCount, callback) — 模拟延时 + progress
9.       → m_store.addBox(boxData)
10.      → notify("geometry.data_changed", ...) — SceneStore 已写入后发出
11.    → py::gil_scoped_acquire (implicit on return)
12.  → 返回 JSON response
13. → RequestService 内部调用 ProgressTracker.completeTask(requestId, true)
14.   → emit taskCompleted(requestId, true) → ProgressCard 显示 ✓ done
15. → RequestService.responseReady → Main.qml onResponseReady
16. (并行) NotificationService.notificationReceived("geometry.data_changed")
17.  → Main.qml → submitAsync({action:"list_boxes"})
18.  → onResponseReady → boxListModel.clear() + append → SidebarPanel 更新
```

### 6.2 Create Box (从 PySide6)

```
1. User clicks "Create Box" in PySide6 window
2. on_create_box() → btn.setEnabled(False) → status = "Creating box…"
3. threading.Thread(target=run).start()
4.   → pywrapper.process({create_box, vertexCount:20})
5.     → py::gil_scoped_release → GeometryModule::process()
6.       → createBox() — 无 ProgressCallback（PySide6 不传）
7.       → m_store.addBox(boxData)
8.       → notify("geometry.data_changed")
9.     → 返回 JSON result
10.  → QMetaObject.invokeMethod → PySide6 UI 更新 (QueuedConnection)
11. (并行) NotificationService.notificationReceived("geometry.data_changed")
12.  → Main.qml → submitAsync({action:"list_boxes"}) → SidebarPanel 自动刷新
```

**注意**: PySide6 路径不经过 RequestService/ProgressTracker，因此 ProgressCard 不显示进度。
sidebar 刷新完全由 notification 驱动，两条路径统一。

## 7. 文件变更清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `src/libs/geometry/include/opengeolab/geometry/scene_store.hpp` | SceneStore 头文件 |
| `src/libs/geometry/src/scene_store.cpp` | SceneStore 实现 |
| `src/app/resource/qml/components/ProgressCard.qml` | 弹出式进度卡片 |
| `src/app/resource/qml/components/BoxListItem.qml` | 可展开 box 列表项 |
| `src/app/resource/icons/cubeOutline.svg` | Box 图标（ionicons MIT） |
| `src/app/resource/icons/hourglassOutline.svg` | 进行中图标（ionicons MIT） |
| `src/app/resource/icons/checkmarkCircleOutline.svg` | 完成图标（ionicons MIT） |
| `src/app/resource/icons/closeCircleOutline.svg` | 失败图标（ionicons MIT） |

### 修改文件

| 文件 | 变更 |
|------|------|
| `src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp` | 自由函数 → GeometryModule 类（硬切换） |
| `src/libs/geometry/src/geometry_module.cpp` | 有状态分发 + list_boxes action + data_changed 通知 |
| `src/libs/geometry/src/create_box_action.cpp` | 移除直接 notify (交由 GeometryModule 统一管理) |
| `src/libs/geometry/CMakeLists.txt` | 新增 scene_store.cpp |
| `src/libs/geometry/tests/geometry_module_test.cpp` | 适配 GeometryModule 类 + 新增 SceneStore/list_boxes 测试 |
| `src/libs/python/python_wrapper/src/python_wrapper_module.cpp` | GeometryModule 实例 + GIL release/acquire |
| `src/app/include/opengeolab/app/progress_tracker.hpp` | 新增 currentMessage Q_PROPERTY + taskCompleted signal |
| `src/app/src/progress_tracker.cpp` | 实现 currentMessage() + emit taskCompleted |
| `src/app/resource/qml/sections/ActivityOverlay.qml` | 移除旧 progressBar + timer，嵌入 ProgressCard |
| `src/app/resource/qml/sections/SidebarPanel.qml` | 空态 + ListView + required boxListModel |
| `src/app/resource/qml/Main.qml` | ListModel + geometry.data_changed + list_boxes + 移除旧 progress 属性传递 |
| `src/app/resource/qml/components/qmldir` | 注册 ProgressCard, BoxListItem |
| `src/app/CMakeLists.txt` | 新增 QML/icon 资源注册 |
| `plugins/demo_ui_plugin/__init__.py` | Create Box 按钮 + threading |

## 8. 测试策略

### 单元测试（C++）

- **SceneStore**: addBox 返回递增 ID、allBoxes 返回快照、clear 清空、线程安全并发 addBox
- **GeometryModule**: create_box 后 list_boxes 能查到、create_box 返回带 ID 的结果
- **ProgressTracker**: currentMessage Q_PROPERTY 正确更新、taskCompleted signal 发出

### 集成验证（手动）

- QML Ribbon "Create Box" → ProgressCard 显示进度 → 完成后 ✓ 图标 → 3s 淡出 → sidebar 列表出现 box
- 连续创建多个 box → 列表正确累积、ID 递增
- PySide6 "Create Box" → QML sidebar 自动刷新（ProgressCard 不显示——符合预期）
- 暗色/亮色主题切换 → ProgressCard 和 BoxListItem 颜色正确
- requestId 字段名统一使用 camelCase `"requestId"`（修复现有 snake_case 不一致）

## 9. 约束与风险

| 风险 | 缓解措施 |
|------|----------|
| GIL 与 PySide6 事件循环交互 | pywrapper 在 C++ 计算前 `gil_scoped_release`；callback 内 `gil_scoped_acquire` |
| SceneStore 生命周期绑定 pywrapper 模块 | 进程级生命周期可接受；未来提升到 app 层注入 |
| SceneStore 内存增长（大量 box） | 当前为测试场景，可后续加 clear/限制 |
| list_boxes JSON 序列化开销 | 测试阶段数据量小；未来可增量通知或分页 |
| 进度卡片与 ActivityPanel 同时展开的布局 | ProgressCard 在 ActivityPanel 和 activityButton 之间 |
| Ionicons 来源为本地路径 | 图标为 MIT 协议；实现时从本地复制并在 CMakeLists 注册 |
