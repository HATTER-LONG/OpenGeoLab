# 进度 UI + Sidebar Box 列表 + PySide6 Create Box 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 在现有分层服务架构上增加弹出式进度卡片、sidebar box 列表、PySide6 Create Box 按钮，验证端到端数据流。

**架构：** C++ libs/geometry 新增 SceneStore（线程安全存储）+ GeometryModule 重构为有状态类。ProgressTracker 新增 `currentMessage` + `taskCompleted` signal。QML 侧新增 ProgressCard 和 BoxListItem 组件，通过 notification-driven query 刷新 sidebar。PySide6 通过 threading.Thread + pywrapper 异步调用 geometry 并触发 QML 刷新。

**技术栈：** C++20 / Qt 6 QML / pybind11 / PySide6 / nlohmann::json / doctest

**规格文档：** `docs/superpowers/specs/2026-03-24-progress-ui-sidebar-pyside6-design.md`

**构建验证命令：**
- 构建: `cmake --build build --config RelWithDebInfo --parallel 4`
- 测试: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- 格式化: `clang-format`、`cmake-format`、`clang-tidy`

---

## 文件结构总览

### 新增文件

| 文件 | 职责 |
|------|------|
| `src/libs/geometry/include/opengeolab/geometry/scene_store.hpp` | SceneStore 头文件 — 线程安全 box 存储 |
| `src/libs/geometry/src/scene_store.cpp` | SceneStore 实现 |
| `src/app/resource/qml/components/ProgressCard.qml` | 弹出式进度卡片组件 |
| `src/app/resource/qml/components/BoxListItem.qml` | 可展开 box 列表项组件 |
| `src/app/resource/icons/cubeOutline.svg` | ionicons box 图标 |
| `src/app/resource/icons/hourglassOutline.svg` | ionicons 进行中图标 |
| `src/app/resource/icons/checkmarkCircleOutline.svg` | ionicons 完成图标 |
| `src/app/resource/icons/closeCircleOutline.svg` | ionicons 失败图标 |

### 修改文件

| 文件 | 变更要点 |
|------|----------|
| `src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp` | 自由函数 → GeometryModule 类，process() 参数改 const& |
| `src/libs/geometry/src/geometry_module.cpp` | 有状态分发 + list_boxes + data_changed 通知 |
| `src/libs/geometry/include/opengeolab/geometry/create_box_action.hpp` | ProgressCallback 参数改 const& |
| `src/libs/geometry/src/create_box_action.cpp` | 匹配 const& 签名，移除 geometry.status completed 通知（交由 GeometryModule 管理） |
| `src/libs/geometry/CMakeLists.txt` | 新增 scene_store.cpp + 头文件 |
| `src/libs/geometry/tests/geometry_module_test.cpp` | 适配 GeometryModule 类 + 新增 SceneStore/list_boxes 测试 |
| `src/libs/python/python_wrapper/src/python_wrapper_module.cpp` | GeometryModule 实例 + GIL release/acquire |
| `src/app/include/opengeolab/app/progress_tracker.hpp` | 新增 currentMessage Q_PROPERTY + taskCompleted signal |
| `src/app/src/progress_tracker.cpp` | 实现 currentMessage() + emit taskCompleted |
| `src/app/resource/qml/sections/ActivityOverlay.qml` | 移除旧 progressBar，嵌入 ProgressCard |
| `src/app/resource/qml/sections/SidebarPanel.qml` | 空态 + ListView + required boxListModel |
| `src/app/resource/qml/Main.qml` | ListModel + data_changed handler + list_boxes 响应 + 简化 ActivityOverlay 绑定 |
| `src/app/resource/qml/components/qmldir` | 注册 ProgressCard, BoxListItem |
| `src/app/CMakeLists.txt` | QML_FILES + RESOURCES 注册新文件 |
| `plugins/demo_ui_plugin/__init__.py` | Create Box 按钮 + threading |

### 边界稳定要求

- `INotificationSink` 接口不变
- `NotificationRegistry` 静态 API 不变
- `RequestService` 对外 API 不变
- `opengeolab_runtime.py` 的 fallback 到 wrapper.process() 路径不变
- 现有 `progress_demo_plugin.py` 继续工作

---

## 任务 1：SceneStore + GeometryModule 重构（C++）

**文件：**
- 新增：`src/libs/geometry/include/opengeolab/geometry/scene_store.hpp`
- 新增：`src/libs/geometry/src/scene_store.cpp`
- 修改：`src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp`
- 修改：`src/libs/geometry/src/geometry_module.cpp`
- 修改：`src/libs/geometry/include/opengeolab/geometry/create_box_action.hpp`
- 修改：`src/libs/geometry/src/create_box_action.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`

**参考文件：**
- 规格 §4.1（SceneStore 接口）
- 规格 §4.2（GeometryModule 类 + list_boxes + data_changed 通知时序）

- [ ] 步骤 1：创建 `scene_store.hpp`
  - `SceneStore` 类：`addBox(BoxData) → int`、`allBoxes() → vector<pair<int, BoxData>>`、`boxCount()`、`clear()`
  - `mutable std::mutex m_mutex`、`vector<pair<int, BoxData>> m_boxes`、`int m_nextId = 1`
  - Doxygen 注释：@file、@brief、每个 public 方法

- [ ] 步骤 2：创建 `scene_store.cpp`
  - 实现所有方法：lock_guard + 操作 + 返回
  - addBox 返回 m_nextId++，存储 {id, box} pair
  - allBoxes 返回 m_boxes 的副本

- [ ] 步骤 3：修改 `create_box_action.hpp`
  - `ProgressCallback` 参数从 by-value 改为 `const ProgressCallback&`：
    ```cpp
    [[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT BoxData
    createBox(std::array<double, 3> center,
              std::array<double, 3> dimensions,
              int vertex_count,
              const ProgressCallback& progress_callback = {});
    ```

- [ ] 步骤 4：修改 `create_box_action.cpp`
  - 匹配 `const ProgressCallback&` 签名
  - 移除尾部的 `notify("geometry.status", completed)` — 改由 GeometryModule 统一发出
  - 保留 `notify("geometry.status", started)` 和 `notify("geometry.progress", ...)` 不变

- [ ] 步骤 5：重写 `geometry_module.hpp`
  - 移除自由函数 `processGeometry()`
  - 新增 `GeometryModule` 类：
    ```cpp
    class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule {
    public:
        explicit GeometryModule(SceneStore& store);
        [[nodiscard]] std::string process(std::string_view request_json,
                                          const ModuleProgressCallback& progress_callback = {});
    private:
        SceneStore& m_store;
    };
    ```
  - `ModuleProgressCallback` 也改为以 `const&` 形式使用（签名不变，但 process 接受 const&）

- [ ] 步骤 6：重写 `geometry_module.cpp`
  - 构造函数存储 `m_store` 引用
  - `process()` 方法：
    - `"create_box"` action：调用 createBox → `m_store.addBox(box)` → notify("geometry.data_changed") → 返回带 id 的 JSON
    - `"list_boxes"` action：调用 `m_store.allBoxes()` → 序列化为 JSON array → 返回
    - 未知 action：返回 error JSON
  - `"geometry.data_changed"` 通知在 addBox **之后**发出，payload 包含 `{"event":"data_changed","count":N}`
  - 统一 response 中 requestId 为 camelCase（修复现有 `"request_id"` 不一致）

- [ ] 步骤 7：更新 `src/libs/geometry/CMakeLists.txt`
  - `geometry_public_headers` 添加 `include/opengeolab/geometry/scene_store.hpp`
  - `geometry_sources` 添加 `src/scene_store.cpp`

- [ ] 步骤 8：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```
  预期：编译通过（测试会失败因为还没更新）

---

## 任务 2：Geometry 单元测试更新

**文件：**
- 修改：`src/libs/geometry/tests/geometry_module_test.cpp`

**参考文件：**
- 现有测试（3 个 TEST_CASE，使用旧的 `processGeometry()` 自由函数）

- [ ] 步骤 1：更新现有 createBox 测试
  - 无需改动（createBox 仍然是自由函数，只改了 callback 为 const&）

- [ ] 步骤 2：改写 `processGeometry handles create_box` 测试
  - 改为使用 `GeometryModule` 类：
    ```cpp
    Geometry::SceneStore store;
    Geometry::GeometryModule module(store);
    const auto response_str = module.process(request.dump());
    ```
  - 验证 response 中新增 `result.id` 字段（int > 0）
  - 验证 `store.boxCount() == 1`

- [ ] 步骤 3：改写 `processGeometry rejects unknown action` 测试
  - 同样使用 GeometryModule 实例

- [ ] 步骤 4：新增 `SceneStore addBox and allBoxes` 测试
  - addBox 3 次 → allBoxes 返回 3 个 → ID 递增 → clear → boxCount == 0

- [ ] 步骤 5：新增 `GeometryModule list_boxes after create_box` 测试
  - create_box 2 次 → list_boxes → 验证 count == 2，boxes 数组有 2 个元素

- [ ] 步骤 6：运行测试
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```
  预期：所有测试通过

- [ ] 步骤 7：提交
  ```
  git add -A && git commit -m "feat(geometry): add SceneStore and refactor GeometryModule to stateful class"
  ```

---

## 任务 3：python_wrapper GIL 安全更新

**文件：**
- 修改：`src/libs/python/python_wrapper/src/python_wrapper_module.cpp`
- 修改：`src/libs/python/python_wrapper/CMakeLists.txt`（如需更新链接）

**参考文件：**
- 规格 §4.3（GIL release/acquire 代码示例）

- [ ] 步骤 1：修改 `python_wrapper_module.cpp`
  - 在模块顶部添加静态对象：
    ```cpp
    #include <opengeolab/geometry/scene_store.hpp>
    static OpenGeoLab::Geometry::SceneStore s_sceneStore;
    static OpenGeoLab::Geometry::GeometryModule s_geometryModule{s_sceneStore};
    ```
  - 移除旧的 `processGeometry()` 调用
  - 在 process() lambda 中：
    - 包装 py::object callback 时在 lambda 内用 `py::gil_scoped_acquire`
    - 调用 `s_geometryModule.process()` 前用 `py::gil_scoped_release`
    - `process()` 接受 `const&`，确保 py::object 在 GIL 持有时析构
  - 移除旧的 string-based module routing，改用 nlohmann::json parse 获取 module 字段

- [ ] 步骤 2：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 3：提交
  ```
  git add -A && git commit -m "feat(python): update pywrapper for GeometryModule instance with GIL safety"
  ```

---

## 任务 4：ProgressTracker 增强

**文件：**
- 修改：`src/app/include/opengeolab/app/progress_tracker.hpp`
- 修改：`src/app/src/progress_tracker.cpp`

**参考文件：**
- 规格 §5.2（currentMessage + taskCompleted signal）

- [ ] 步骤 1：修改 `progress_tracker.hpp`
  - 新增 Q_PROPERTY：
    ```cpp
    Q_PROPERTY(QString currentMessage READ currentMessage NOTIFY progressChanged)
    ```
  - 新增 signal：
    ```cpp
    void taskCompleted(const QString& task_id, bool success);
    ```
  - 新增声明：
    ```cpp
    [[nodiscard]] QString currentMessage() const;
    ```

- [ ] 步骤 2：修改 `progress_tracker.cpp`
  - 实现 `currentMessage()`：与 statusText() 类似，但只返回最近活跃任务的 `message` 部分（不含 description 前缀）
  - 在 `completeTask()` 末尾新增：
    ```cpp
    QMetaObject::invokeMethod(this, [this, task_id, success]() {
        emit taskCompleted(task_id, success);
    }, Qt::QueuedConnection);
    ```

- [ ] 步骤 3：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 4：提交
  ```
  git add -A && git commit -m "feat(app): add currentMessage property and taskCompleted signal to ProgressTracker"
  ```

---

## 任务 5：复制 Ionicons SVG 图标

**文件：**
- 新增：`src/app/resource/icons/cubeOutline.svg`
- 新增：`src/app/resource/icons/hourglassOutline.svg`
- 新增：`src/app/resource/icons/checkmarkCircleOutline.svg`
- 新增：`src/app/resource/icons/closeCircleOutline.svg`
- 修改：`src/app/CMakeLists.txt`（RESOURCES 段新增 4 行）

**源路径：** `C:\Users\layton\Desktop\WorkSpace\Project\ionicons-8.0.13\src\svg\`

- [ ] 步骤 1：复制并重命名 4 个 SVG
  ```powershell
  Copy-Item "C:\Users\layton\Desktop\WorkSpace\Project\ionicons-8.0.13\src\svg\cube-outline.svg" `
    "src\app\resource\icons\cubeOutline.svg"
  Copy-Item "...\hourglass-outline.svg" "src\app\resource\icons\hourglassOutline.svg"
  Copy-Item "...\checkmark-circle-outline.svg" "src\app\resource\icons\checkmarkCircleOutline.svg"
  Copy-Item "...\close-circle-outline.svg" "src\app\resource\icons\closeCircleOutline.svg"
  ```

- [ ] 步骤 2：修改 `src/app/CMakeLists.txt`
  - 在 RESOURCES 段末尾（pluginJ.svg 之后）添加：
    ```cmake
    resource/icons/cubeOutline.svg
    resource/icons/hourglassOutline.svg
    resource/icons/checkmarkCircleOutline.svg
    resource/icons/closeCircleOutline.svg
    ```

- [ ] 步骤 3：提交
  ```
  git add -A && git commit -m "feat(app): add ionicons SVGs for progress and geometry UI"
  ```

---

## 任务 6：ProgressCard.qml 组件

**文件：**
- 新增：`src/app/resource/qml/components/ProgressCard.qml`
- 修改：`src/app/resource/qml/components/qmldir`
- 修改：`src/app/CMakeLists.txt`（QML_FILES 段新增）

**参考文件：**
- 规格 §4.4（视觉规格 + 状态机 + 属性绑定）
- `src/app/resource/qml/theme/AppTheme.qml`（主题色彩）
- `src/app/resource/qml/components/AppIcon.qml`（图标用法）

- [ ] 步骤 1：创建 `ProgressCard.qml`
  - 组件结构：
    ```
    Rectangle (card background, theme.surface, radiusSmall, border)
      RowLayout (top: icon + description + percentage)
        AppIcon (hourglassOutline / checkmarkCircleOutline / closeCircleOutline)
        Text (description — binds ProgressTracker.statusText)
        Text (percentage — Math.round(progress * 100) + "%")
      Rectangle (progress bar, 6px height)
        Rectangle (fill, behavior on width 120ms)
      Text (message — binds ProgressTracker.currentMessage)
    ```
  - 属性：`required property AppTheme theme`
  - 内部属性：`active`, `progress`, `description`, `message`, `completionState`
  - 图标选择：completionState === "done" → checkmark, "failed" → close, default → hourglass
  - 颜色选择：completionState === "done" → theme.success, "failed" → theme.danger, default → theme.accentA
  - visible 绑定：`active || hideTimer.running || completionState !== ""`
  - Connections 到 ProgressTracker.onTaskCompleted → 设置 completionState + hideTimer
  - 所有用户可见文本用 `qsTr()`
  - 不确定进度（progress == 0）：滑动动画（复用旧 ActivityOverlay 的 NumberAnimation 模式）
  - Behavior on opacity: 0→1, 180ms OutQuad

- [ ] 步骤 2：更新 `qmldir`
  - 添加：`ProgressCard 1.0 ProgressCard.qml`

- [ ] 步骤 3：更新 `src/app/CMakeLists.txt`
  - QML_FILES 段添加：`resource/qml/components/ProgressCard.qml`

- [ ] 步骤 4：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

---

## 任务 7：BoxListItem.qml 组件

**文件：**
- 新增：`src/app/resource/qml/components/BoxListItem.qml`
- 修改：`src/app/resource/qml/components/qmldir`
- 修改：`src/app/CMakeLists.txt`

**参考文件：**
- 规格 §4.6（BoxListItem 折叠态/展开态）

- [ ] 步骤 1：创建 `BoxListItem.qml`
  - 属性：`required property AppTheme theme`、`required property string label`、`required property int boxId`、`required property var center`、`required property var size`、`required property int vertexCount`
  - 内部 `property bool expanded: false`
  - 折叠态：`[cubeOutline icon] label`（单行，点击切换 expanded）
  - 展开态：增加 3 行详情（Center / Size / Vertices），缩进显示
  - 文本用 `qsTr()`，格式如 `qsTr("Center: (%1, %2, %3)").arg(...)`
  - 背景用 theme.surfaceMuted（hover 时 surfaceStrong），圆角 radiusSmall
  - Behavior on height (展开/折叠动画 150ms)

- [ ] 步骤 2：更新 `qmldir`
  - 添加：`BoxListItem 1.0 BoxListItem.qml`

- [ ] 步骤 3：更新 `src/app/CMakeLists.txt`
  - QML_FILES 段添加：`resource/qml/components/BoxListItem.qml`

- [ ] 步骤 4：构建验证

---

## 任务 8：ActivityOverlay + SidebarPanel + Main.qml 改造

**文件：**
- 修改：`src/app/resource/qml/sections/ActivityOverlay.qml`
- 修改：`src/app/resource/qml/sections/SidebarPanel.qml`
- 修改：`src/app/resource/qml/Main.qml`

**参考文件：**
- 规格 §4.5（ActivityOverlay 改造）
- 规格 §4.6（SidebarPanel 改造 + boxListModel 传递）
- 规格 §4.7（刷新流程）

- [ ] 步骤 1：改造 `ActivityOverlay.qml`
  - 移除旧 `progressBar` Rectangle（第 47-76 行区域）
  - 移除旧 `progressHideTimer` Timer（第 29-33 行）
  - 移除旧 `progressColor` readonly property（第 26-27 行）
  - 移除旧 `onProgressStatusChanged` handler（第 35-45 行）
  - 移除旧的 `property real progress` 和 `property string progressStatus`
  - 在 activityButton 上方、activityPanel 下方嵌入 ProgressCard
  - ProgressCard 的 anchors: right=parent.right, bottom=activityPanel/activityButton.top
  - 调整 height 计算：去掉旧 progressBar 高度，加入 ProgressCard 条件高度

- [ ] 步骤 2：改造 `SidebarPanel.qml`
  - 新增 `required property ListModel boxListModel`
  - SectionCard 内：
    - 空态 Text（visible: boxListModel.count === 0）
    - ListView（visible: boxListModel.count > 0, model: boxListModel）
    - delegate: BoxListItem，使用 required properties 从 model 取值
  - ListView 的 clip: true, spacing: 4

- [ ] 步骤 3：改造 `Main.qml`
  - 顶部新增 `ListModel { id: boxListModel }`
  - SidebarPanel 传入 `boxListModel: boxListModel`
  - ActivityOverlay：移除旧的 progress/progressStatus 属性传递（ProgressCard 内部直接绑定 ProgressTracker）
  - NotificationService handler 增加 `"geometry.data_changed"` 分支：
    ```javascript
    if (channel === "geometry.data_changed") {
        RequestService.submitAsync(JSON.stringify({
            module: "geometry", action: "list_boxes", param: {}
        }));
    }
    ```
  - RequestService handler 增加 list_boxes 响应处理：
    ```javascript
    if (resp.module === "geometry" && resp.action === "list_boxes" && resp.ok) {
        boxListModel.clear();
        const boxes = resp.result.boxes || [];
        for (let i = 0; i < boxes.length; ++i) {
            boxListModel.append(boxes[i]);
        }
    }
    ```

- [ ] 步骤 4：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 5：提交
  ```
  git add -A && git commit -m "feat(app): add ProgressCard, BoxListItem, sidebar box list, and notification-driven refresh"
  ```

---

## 任务 9：PySide6 demo_ui_plugin 增强

**文件：**
- 修改：`plugins/demo_ui_plugin/__init__.py`

**参考文件：**
- 规格 §4.8（DemoWindow + threading + QMetaObject）

- [ ] 步骤 1：重写 `demo_ui_plugin/__init__.py`
  - 保留 `describe_plugin()` 不变
  - `launch_ui()` 改为创建 `DemoWindow(QWidget)` 实例
  - DemoWindow 包含：
    - QLabel "Geometry Tool" 标题
    - QPushButton "Create Box" — 点击触发 `on_create_box()`
    - QLabel 状态标签（"Ready" / "Creating box…" / "Created: Box(2x2x2)"）
    - QPushButton "Close"
  - `on_create_box()`：
    - 禁用按钮 + 设置状态
    - `threading.Thread(target=run, daemon=True).start()`
    - `run()` 内：`import opengeolab_pywrapper as wrapper`、构建 JSON、`wrapper.process(req)`
    - 完成后通过 `QMetaObject.invokeMethod(QueuedConnection)` 更新 UI
  - `vertexCount=20` 控制延时

- [ ] 步骤 2：构建验证（copy plugins 到 output）
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 3：提交
  ```
  git add -A && git commit -m "feat(plugins): add Create Box button to demo UI plugin with async geometry call"
  ```

---

## 任务 10：全量构建 + 测试 + 代码质量

**文件：** 所有已变更文件

- [ ] 步骤 1：全量构建
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 2：运行测试
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```
  预期：所有测试通过（包括新增的 SceneStore/list_boxes 测试）

- [ ] 步骤 3：clang-format
  - 对所有新增/修改的 .hpp/.cpp 文件运行 clang-format

- [ ] 步骤 4：cmake-format
  - 对 `src/libs/geometry/CMakeLists.txt`、`src/app/CMakeLists.txt` 运行 cmake-format

- [ ] 步骤 5：clang-tidy
  - 对所有新增/修改的 .hpp/.cpp 文件运行 clang-tidy
  - 确认 0 warnings（尤其关注命名规范）

- [ ] 步骤 6：最终提交（如有格式修复）
  ```
  git add -A && git commit -m "style: apply clang-format, cmake-format, and clang-tidy fixes"
  ```

---

## 依赖关系

```
任务 1 (SceneStore + GeometryModule)
  └── 任务 2 (测试更新) ──┐
  └── 任务 3 (pywrapper)  ├── 任务 8 (QML 集成)
任务 4 (ProgressTracker) ──┤     └── 任务 9 (PySide6)
任务 5 (Icons) ────────────┤          └── 任务 10 (验证)
任务 6 (ProgressCard) ─────┤
任务 7 (BoxListItem) ──────┘
```

- 任务 1 是所有后续的基础
- 任务 2-7 可以在任务 1 完成后并行（但建议 2 紧跟 1 做验证）
- 任务 8 依赖 4/5/6/7 的 QML 组件和 ProgressTracker 改动
- 任务 9 依赖 8（QML 的 data_changed 刷新路径要先就位）
- 任务 10 是全量验证
