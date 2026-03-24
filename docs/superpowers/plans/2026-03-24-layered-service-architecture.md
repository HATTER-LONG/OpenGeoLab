# 分层服务架构 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 将单体 `ProcessService` 拆分为 `RequestService`、`NotificationService`、`ProgressTracker`、`MainThreadExecutor` 四个分层服务，支持请求追踪、进度回调和推送通知。

**架构：** 新建 `libs/base` 基础模块放置共享接口（`INotificationSink`）。App 层实现四个 QObject 服务，分别注册为 QML 单例。`EmbeddedPythonRuntime::process()` 扩展为接受 `std::function` 进度回调，内部在 GIL 范围内包装为 `py::cpp_function`。

**技术栈：** C++20、Qt 6.9、pybind11 3.0.2、QML、Python 3、CMake + Ninja、Windows MSVC RelWithDebInfo

**规格文档：** `docs/superpowers/specs/2026-03-24-layered-service-architecture-design.md`

**验证命令：**
```bash
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

---

## 文件结构总览

### 新增文件

| 文件 | 职责 |
|------|------|
| `src/libs/base/CMakeLists.txt` | base 基础模块构建配置 |
| `src/libs/base/include/opengeolab/base/notification_sink.hpp` | INotificationSink 接口 |
| `src/app/include/opengeolab/app/progress_tracker.hpp` | ProgressTracker 头文件 |
| `src/app/src/progress_tracker.cpp` | ProgressTracker 实现 |
| `src/app/include/opengeolab/app/main_thread_executor.hpp` | MainThreadExecutor 头文件 |
| `src/app/src/main_thread_executor.cpp` | MainThreadExecutor 实现 |
| `src/app/include/opengeolab/app/notification_service.hpp` | NotificationService 头文件 |
| `src/app/src/notification_service.cpp` | NotificationService 实现 |
| `src/app/include/opengeolab/app/request_service.hpp` | RequestService 头文件 |
| `src/app/src/request_service.cpp` | RequestService 实现 |

### 修改文件

| 文件 | 变更 |
|------|------|
| `CMakeLists.txt`（顶层） | 添加 `add_subdirectory(src/libs/base)` |
| `src/libs/python/python_embedded/include/opengeolab/python/embedded_python_runtime.hpp` | 添加 `ProgressCallback` 类型别名和重载签名 |
| `src/libs/python/python_embedded/src/embedded_python_runtime.cpp` | 实现带回调的 `process()`，GIL 内包装为 `py::cpp_function` |
| `src/app/resource/python/opengeolab_runtime.py` | `process()` 接受 `progress_callback`；`_make_response()` 添加 `request_id` |
| `src/app/CMakeLists.txt` | 替换 `process_service.cpp` → 新的 4 个源文件；链接 `OpenGeoLab::Base` |
| `src/app/src/main.cpp` | 创建 4 个服务实例并注册为 QML 单例 |
| `src/app/resource/qml/Main.qml` | 使用 `RequestService` 替代 `ProcessService`；绑定 `ProgressTracker` |
| `src/app/resource/qml/sections/ActivityOverlay.qml` | 绑定 `ProgressTracker` 属性到进度条 |

### 删除文件

| 文件 | 原因 |
|------|------|
| `src/app/include/opengeolab/app/process_service.hpp` | 被 RequestService 替代 |
| `src/app/src/process_service.cpp` | 被 RequestService 替代 |

---

## 任务 1：创建 libs/base 基础模块

**文件：**
- 新增：`src/libs/base/CMakeLists.txt`
- 新增：`src/libs/base/include/opengeolab/base/notification_sink.hpp`
- 修改：`CMakeLists.txt`（顶层，添加 `add_subdirectory(src/libs/base)`）

**参考：**
- 规格文档 §4.1
- 现有模式参考：`src/libs/python/python_embedded/CMakeLists.txt`
- 构建函数参考：`cmake/OpenGeoLabModule.cmake`

- [ ] 步骤 1：创建目录 `src/libs/base/include/opengeolab/base/`
- [ ] 步骤 2：创建 `src/libs/base/include/opengeolab/base/notification_sink.hpp`

  ```cpp
  #pragma once
  #include <opengeolab/base/base_export.hpp>
  #include <string_view>

  namespace OpenGeoLab::Base {

  class OPENGEOLAB_BASE_EXPORT INotificationSink {
  public:
      virtual ~INotificationSink() = default;
      virtual void notify(std::string_view channel, std::string_view payload_json) = 0;
  };

  } // namespace OpenGeoLab::Base
  ```

- [ ] 步骤 3：创建 `src/libs/base/CMakeLists.txt`

  ```cmake
  set(base_public_headers include/opengeolab/base/notification_sink.hpp)

  opengeolab_add_module(
      opengeolab_base
      ALIAS_NAME Base
      PUBLIC_HEADERS ${base_public_headers})
  ```

  注意：header-only 接口，`opengeolab_add_module` 没有 SOURCES 参数时 `add_library` 创建的是空目标（需要添加一个空的 `.cpp` 占位文件或使用 INTERFACE）。检查 `opengeolab_add_module` 是否需要至少一个源文件——如果需要，创建一个 `src/notification_sink.cpp` 仅包含 `// Placeholder for opengeolab_base library target.`。

- [ ] 步骤 4：修改顶层 `CMakeLists.txt`，在 `add_subdirectory(src/libs/python)` 之前添加 `add_subdirectory(src/libs/base)`
- [ ] 步骤 5：运行 `cmake -S . -B build -G Ninja` 验证配置通过
- [ ] 步骤 6：运行 `cmake --build build --config RelWithDebInfo --parallel 4` 验证构建通过
- [ ] 步骤 7：确认 `build/` 下生成了 `opengeolab_base.lib`（或 `.dll`）和 `base_export.hpp`
- [ ] 步骤 8：提交 `feat(base): add libs/base module with INotificationSink interface`

---

## 任务 2：实现 ProgressTracker

**文件：**
- 新增：`src/app/include/opengeolab/app/progress_tracker.hpp`
- 新增：`src/app/src/progress_tracker.cpp`

**参考：**
- 规格文档 §5.1–§5.4

**不修改 CMakeLists.txt**——后续任务 6 统一修改。

- [ ] 步骤 1：创建 `progress_tracker.hpp`

  按规格 §5.1 实现完整头文件：
  - `Q_PROPERTY(double currentProgress ...)`
  - `Q_PROPERTY(QString statusText ...)`
  - `Q_PROPERTY(bool hasActiveTasks ...)`
  - 公共方法：`beginTask()`, `updateProgress()`, `completeTask()`
  - 私有成员：`mutable std::mutex mutex_`、`std::unordered_map<QString, TaskState> tasks_`、`QTimer* prune_timer_`
  - 内部 `struct TaskState`

- [ ] 步骤 2：创建 `progress_tracker.cpp`

  实现要点：
  - 构造函数启动 10s 间隔的 `QTimer` 清理已完成任务
  - `beginTask()`：加锁 → 插入 TaskState → 解锁 → `QMetaObject::invokeMethod` 触发 `progressChanged`
  - `updateProgress()`：加锁 → 更新 progress/message/last_update → 解锁 → 同上
  - `completeTask()`：加锁 → 设 completed=true/success → 解锁 → 同上
  - `currentProgress()`：加锁 → 遍历找最近更新的活跃任务 → 返回 progress 或 -1.0
  - `statusText()`：同理返回最近活跃任务的 description + message
  - `hasActiveTasks()`：加锁 → 检查是否存在非 completed 任务
  - 清理函数：移除 `completed == true` 且 `last_update` 超过 10s 的条目

- [ ] 步骤 3：在本地文件上运行 clang-format 和 clang-tidy 验证代码规范
- [ ] 步骤 4：提交 `feat(app): implement ProgressTracker with thread-safe task tracking`

---

## 任务 3：实现 MainThreadExecutor

**文件：**
- 新增：`src/app/include/opengeolab/app/main_thread_executor.hpp`
- 新增：`src/app/src/main_thread_executor.cpp`

**参考：**
- 规格文档 §6.1–§6.2

- [ ] 步骤 1：创建 `main_thread_executor.hpp`

  按规格 §6.1：
  - `QObject` 子类
  - `void execute(std::function<void()> task)`
  - `void executeBlocking(std::function<void()> task)`，附 GIL 死锁约束文档

- [ ] 步骤 2：创建 `main_thread_executor.cpp`

  实现要点：
  - `execute()`：`QThread::currentThread() == thread()` 检查 → 直接执行或 `Qt::QueuedConnection`
  - `executeBlocking()`：同上检查 → 直接执行或 `Qt::BlockingQueuedConnection`
  - Debug 构建中 `Q_ASSERT(PyGILState_Check() == 0)` 防止 GIL 死锁
  - 注意：需要 `#include <pybind11/pybind11.h>` 才能调用 `PyGILState_Check()`，或者使用 `#ifdef Py_PYTHON_H` 条件编译

- [ ] 步骤 3：clang-format + clang-tidy 验证
- [ ] 步骤 4：提交 `feat(app): implement MainThreadExecutor for cross-thread dispatch`

---

## 任务 4：实现 NotificationService

**文件：**
- 新增：`src/app/include/opengeolab/app/notification_service.hpp`
- 新增：`src/app/src/notification_service.cpp`

**参考：**
- 规格文档 §4.2–§4.4

- [ ] 步骤 1：创建 `notification_service.hpp`

  按规格 §4.2：
  - 继承 `QObject` 和 `OpenGeoLab::Base::INotificationSink`
  - `void notify(std::string_view channel, std::string_view payload_json) override`
  - `void enableBuffering(const QString& channel_prefix, int interval_ms = 16)`
  - `signal: notificationReceived(const QString& channel, const QString& payload)`
  - 私有成员：缓冲相关结构体和 map

- [ ] 步骤 2：创建 `notification_service.cpp`

  实现要点：
  - `notify()` 默认路径：`QString::fromUtf8` 转换 → `QMetaObject::invokeMethod` + `QueuedConnection`
  - `notify()` 缓冲路径：检查 channel 是否匹配已注册的缓冲前缀 → 加锁 → push 到 pending vector → 如果是首条则 `QTimer::singleShot` 调度 flush
  - `flush()`：加锁 → 取出所有 pending → 解锁 → 组装 JSON 数组字符串 → emit `notificationReceived`
  - `enableBuffering()`：记录前缀和间隔到内部 map

- [ ] 步骤 3：clang-format + clang-tidy 验证
- [ ] 步骤 4：提交 `feat(app): implement NotificationService with buffered delivery`

---

## 任务 5：扩展 EmbeddedPythonRuntime 签名

**文件：**
- 修改：`src/libs/python/python_embedded/include/opengeolab/python/embedded_python_runtime.hpp`
- 修改：`src/libs/python/python_embedded/src/embedded_python_runtime.cpp`

**参考：**
- 规格文档 §5.6

- [ ] 步骤 1：修改 `embedded_python_runtime.hpp`

  在类外添加类型别名：
  ```cpp
  using ProgressCallback = std::function<void(double, std::string_view)>;
  ```
  修改 `process()` 签名：
  ```cpp
  [[nodiscard]] std::string process(std::string_view request_json,
                                     ProgressCallback progress_callback = nullptr);
  ```
  添加 `#include <functional>` 和 `#include <string_view>`

- [ ] 步骤 2：修改 `embedded_python_runtime.cpp`

  更新 `process()` 实现：
  ```cpp
  std::string EmbeddedPythonRuntime::process(std::string_view request_json,
                                              ProgressCallback progress_callback) {
      try {
          const Py::gil_scoped_acquire acquire;
          Py::object py_cb = Py::none();
          if (progress_callback) {
              py_cb = Py::cpp_function(
                  [cb = std::move(progress_callback)](double p, const std::string& m) {
                      cb(p, m);
                  });
          }
          return Py::cast<std::string>(
              m_impl->processFunction(Py::str(request_json), py_cb));
      } catch (const Py::error_already_set& error) {
          throw std::runtime_error(error.what());
      }
  }
  ```
  关键：`py::cpp_function` 在 `gil_scoped_acquire` 范围内创建——GIL 安全。

- [ ] 步骤 3：`cmake --build build --target opengeolab_python_embed --config RelWithDebInfo --parallel 4` 验证编译通过
- [ ] 步骤 4：clang-format + clang-tidy 验证
- [ ] 步骤 5：提交 `feat(python): add ProgressCallback parameter to EmbeddedPythonRuntime::process()`

---

## 任务 6：实现 RequestService

**文件：**
- 新增：`src/app/include/opengeolab/app/request_service.hpp`
- 新增：`src/app/src/request_service.cpp`

**参考：**
- 规格文档 §3.1–§3.4

- [ ] 步骤 1：创建 `request_service.hpp`

  按规格 §3.1：
  - `Q_PROPERTY(bool busy ...)`
  - 构造函数接受 `EmbeddedPythonRuntime&` + `ProgressTracker&`
  - `Q_INVOKABLE QString submitAsync(const QString& request_json)`
  - `Q_INVOKABLE QString executeOnMainThread(const QString& request_json)`
  - `signal: responseReady(requestId, responseJson)`, `errorOccurred(requestId, errorMessage)`, `busyChanged()`
  - 私有：`injectRequestId()`, `extractDescription()`, `emitResponse()`
  - 私有成员：`runtime_`, `progress_tracker_`, `pending_count_`, `futures_mutex_`, `pending_futures_`

- [ ] 步骤 2：创建 `request_service.cpp`

  实现要点：

  **`submitAsync()`**：
  1. `QUuid::createUuid().toString(QUuid::WithoutBraces)` 生成 requestId
  2. `injectRequestId()` 注入 JSON
  3. `pending_count_++` + `busyChanged()`
  4. `progress_tracker_.beginTask(requestId, description)`
  5. `QtConcurrent::run` 内部构造 `ProgressCallback` lambda → 调用 `runtime_.process(json, cb)`
  6. `QFutureWatcher` 监听完成 → `completeTask()` + `pending_count_--` + `emitResponse()`

  **`executeOnMainThread()`**：
  1. 生成 requestId + 注入
  2. `beginTask()` → 直接调用 `runtime_.process(json)` → `completeTask()`
  3. `emitResponse()` 或 catch → `errorOccurred()`

  **`injectRequestId()`**：
  `QJsonDocument::fromJson` → `QJsonObject` → `insert("requestId", ...)` → `QJsonDocument(obj).toJson(QJsonDocument::Compact)`

  **`extractDescription()`**：
  解析 JSON → 返回 `"module.action"` 字符串

  **`emitResponse()`**：
  解析 response JSON → 检查 `ok` 字段 → emit `responseReady` 或 `errorOccurred`

  **析构函数**：
  `lock_guard(futures_mutex_)` → 遍历 `pending_futures_` → `waitForFinished()`

- [ ] 步骤 3：clang-format + clang-tidy 验证
- [ ] 步骤 4：提交 `feat(app): implement RequestService with async request tracking`

---

## 任务 7：更新 Python Runtime 协议

**文件：**
- 修改：`src/app/resource/python/opengeolab_runtime.py`

**参考：**
- 规格文档 §5.7

- [ ] 步骤 1：修改 `_make_response()` 签名

  添加 `request_id: str = ""` 参数。当 `request_id` 非空时，在返回的 dict 中插入 `"requestId": request_id`。

- [ ] 步骤 2：修改 `process()` 签名

  添加 `progress_callback=None` 参数。提取 `request_id = request.get("requestId", "")`。将 `request_id` 和 `progress_callback` 传递给所有 handler 函数。

- [ ] 步骤 3：修改所有 handler 函数

  - `_plugins_response(request_id)` → `_make_response(..., request_id=request_id)`
  - `_execute_plugin(request, request_id, progress_callback)` → 使用 `inspect.signature` 检查插件是否接受 `progress_callback` → 传递或不传递
  - `_launch_plugin_ui(request, request_id)` → 同理传递 `request_id`
  - `_capabilities_response(request_id)` → 同理
  - fallback 路径传递 `request_id`

- [ ] 步骤 4：添加 `import inspect` 到文件头部
- [ ] 步骤 5：提交 `feat(python): support requestId echo and progress_callback forwarding`

---

## 任务 8：更新 CMakeLists.txt 和 main.cpp 接线

**文件：**
- 修改：`src/app/CMakeLists.txt`
- 修改：`src/app/src/main.cpp`

**参考：**
- 规格文档 §7.1

- [ ] 步骤 1：修改 `src/app/CMakeLists.txt`

  替换 `qt_add_executable` 中的 `src/process_service.cpp` 为：
  ```
  src/progress_tracker.cpp
  src/main_thread_executor.cpp
  src/notification_service.cpp
  src/request_service.cpp
  ```

  在 `qt_add_qml_module` 的 SOURCES 中替换 `include/opengeolab/app/process_service.hpp` 为：
  ```
  include/opengeolab/app/progress_tracker.hpp
  include/opengeolab/app/main_thread_executor.hpp
  include/opengeolab/app/notification_service.hpp
  include/opengeolab/app/request_service.hpp
  ```

  在 `target_link_libraries` 中添加 `OpenGeoLab::Base`。

- [ ] 步骤 2：修改 `src/app/src/main.cpp`

  替换 `#include <opengeolab/app/process_service.hpp>` 为四个新头文件。

  在 `QApplication app(...)` 之后、`QQmlApplicationEngine engine` 之前：
  ```cpp
  OpenGeoLab::App::ProgressTracker progress_tracker;
  OpenGeoLab::App::NotificationService notification_service;
  OpenGeoLab::App::MainThreadExecutor main_thread_executor;
  OpenGeoLab::App::RequestService request_service(python_runtime, progress_tracker);
  ```

  替换 `qmlRegisterSingletonInstance` 的 `ProcessService` 为四个新注册：
  ```cpp
  qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "RequestService", &request_service);
  qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "NotificationService", &notification_service);
  qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "ProgressTracker", &progress_tracker);
  qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "MainThreadExecutor", &main_thread_executor);
  ```

- [ ] 步骤 3：运行 `cmake --build build --config RelWithDebInfo --parallel 4` 验证编译通过
- [ ] 步骤 4：提交 `refactor(app): wire four layered services replacing ProcessService`

---

## 任务 9：更新 QML 层

**文件：**
- 修改：`src/app/resource/qml/Main.qml`
- 修改：`src/app/resource/qml/sections/ActivityOverlay.qml`

**参考：**
- 规格文档 §7.2–§7.3

- [ ] 步骤 1：修改 `Main.qml`

  **Connections 部分**：
  - 替换 `target: ProcessService` → `target: RequestService`
  - `onResponseReady` 增加 `requestId` 参数：`function onResponseReady(requestId, responseJson)`
  - `onErrorOccurred` 增加 `requestId` 参数：`function onErrorOccurred(requestId, errorMessage)`

  **Component.onCompleted**：
  - 替换 `ProcessService.submitRequest(...)` → `RequestService.submitAsync(...)`

  **openActionPage 函数**：
  - `pluginUI_` 路径：替换 `ProcessService.submitRequest(...)` → `RequestService.executeOnMainThread(...)`
  - `plugin_` 路径：替换 `ProcessService.submitRequest(...)` → `RequestService.submitAsync(...)`

  **ActivityOverlay 绑定**：
  - 添加 `progress` 和 `progressStatus` 属性绑定：
    ```qml
    progress: ProgressTracker.hasActiveTasks ? ProgressTracker.currentProgress : -1
    progressStatus: ProgressTracker.statusText
    ```

- [ ] 步骤 2：修改 `ActivityOverlay.qml`

  目前 `progress` 和 `progressStatus` 已经是 `property`（由外部传入），无需修改组件内部。仅确认 Main.qml 正确传入即可。

- [ ] 步骤 3：运行 `cmake --build build --config RelWithDebInfo --parallel 4` 验证编译通过
- [ ] 步骤 4：提交 `refactor(qml): migrate Main.qml from ProcessService to RequestService`

---

## 任务 10：删除 ProcessService 并最终验证

**文件：**
- 删除：`src/app/include/opengeolab/app/process_service.hpp`
- 删除：`src/app/src/process_service.cpp`

- [ ] 步骤 1：确认 `process_service.hpp` 和 `process_service.cpp` 在项目中无其他引用
  ```bash
  grep -r "process_service" src/ --include="*.hpp" --include="*.cpp" --include="*.qml" --include="*.txt"
  ```
- [ ] 步骤 2：删除两个文件
- [ ] 步骤 3：运行 `cmake --build build --config RelWithDebInfo --parallel 4` 验证构建通过
- [ ] 步骤 4：运行 `ctest --test-dir build -C RelWithDebInfo --output-on-failure` 验证回归测试通过
- [ ] 步骤 5：对所有新增/修改的 C++ 文件运行 clang-format + clang-tidy

  ```bash
  clang-format -i src/app/include/opengeolab/app/request_service.hpp \
    src/app/include/opengeolab/app/notification_service.hpp \
    src/app/include/opengeolab/app/progress_tracker.hpp \
    src/app/include/opengeolab/app/main_thread_executor.hpp \
    src/app/src/request_service.cpp \
    src/app/src/notification_service.cpp \
    src/app/src/progress_tracker.cpp \
    src/app/src/main_thread_executor.cpp \
    src/libs/base/include/opengeolab/base/notification_sink.hpp \
    src/libs/python/python_embedded/include/opengeolab/python/embedded_python_runtime.hpp \
    src/libs/python/python_embedded/src/embedded_python_runtime.cpp \
    src/app/src/main.cpp
  ```

- [ ] 步骤 6：对修改的 CMake 文件运行 cmake-format

  ```bash
  cmake-format -i src/libs/base/CMakeLists.txt \
    src/app/CMakeLists.txt \
    CMakeLists.txt
  ```

- [ ] 步骤 7：提交 `refactor(app): remove legacy ProcessService`
- [ ] 步骤 8：手动验证应用启动、插件列表加载、插件执行、PySide6 UI 非模态弹出
- [ ] 步骤 9：确认 `ActivityOverlay` 进度条在插件执行期间显示进度（如果插件支持 `progress_callback`）

---

## 任务依赖关系

```
任务 1 (libs/base) ──────────→ 任务 4 (NotificationService) ─────┐
                                                                  │
任务 2 (ProgressTracker) ─┐                                       │
                          ├──→ 任务 6 (RequestService) ───────────┤
任务 5 (EmbeddedPythonRuntime 签名) ─┘                            │
                                                                  │
任务 3 (MainThreadExecutor) ──────────────────────────────────────┤
                                                                  │
任务 7 (Python Runtime 协议) ── 无 C++ 依赖 ─────────────────────┤
                                                                  │
                                                    任务 8 (CMake + main.cpp) ←─┘
                                                              │
                                                    任务 9 (QML)
                                                              │
                                                    任务 10 (清理 + 验证)
```

**可并行的任务**：任务 1、2、3、5、7 彼此独立可并行。任务 4 只依赖任务 1。任务 6 依赖任务 2 和 5。

> **测试说明**：规格 §11 列出了 7 项单元/集成测试。鉴于现有 ProcessService 无测试基线，测试编写作为后续独立计划安排，不在本次迁移范围内。本计划以构建验证 + 手动验证为主。
