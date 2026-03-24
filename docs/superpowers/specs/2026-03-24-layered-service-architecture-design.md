# 分层服务架构设计

> **状态**：草案 — 待审阅
>
> **范围**：将单体 `ProcessService` 拆分为四个职责单一的服务：
> `RequestService`、`NotificationService`、`ProgressTracker`、`MainThreadExecutor`。

---

## 1. 问题陈述

当前 `ProcessService` 存在三个阻碍后续演进的问题：

1. **插件感知泄漏**：`process_service.cpp` 第 31–47 行硬编码了
   `module == "plugins" && action == "invoke_ui"` 以强制主线程执行。
   每个需要主线程的新模块都需要在此处改代码。

2. **缺少请求追踪**：响应中没有 `requestId`，QML 无法将响应关联到触发它的请求。
   busy 标志是所有并发任务共享的单个 bool。

3. **缺少推送通知通道**：库层（`libs/`）没有途径向 UI 发送主动推送消息
   （进度、流式 token、日志事件）。唯一的路径是请求→响应。

### 成功标准

| # | 标准 | 验证方式 |
|---|------|----------|
| 1 | `ProcessService` 的替代品中不包含任何 module/action 字符串检查 | 代码审查 |
| 2 | QML 可以通过 `requestId` 追踪单个请求的生命周期 | 单元测试 |
| 3 | C++ 库可以推送通知，且通知在 QML 中被正确接收 | 集成测试 |
| 4 | `ActivityOverlay` 中的进度条可以显示插件的真实 0–100% 进度 | 手动测试 |
| 5 | PySide6 插件 UI 仍然以非模态方式启动 | 手动测试 |
| 6 | Windows（RelWithDebInfo）构建通过 | CI / 本地构建 |

---

## 2. 架构总览

```
┌──────────────────────────────────────────────────────────────────┐
│                          QML 层                                  │
│  ┌──────────┐  ┌──────────────────┐  ┌─────────────────────┐    │
│  │ Request   │  │ Notification     │  │ ProgressTracker     │    │
│  │ Service   │  │ Service          │  │ (Q_PROPERTY 绑定)   │    │
│  │ singleton │  │ singleton        │  │ singleton           │    │
│  └─────┬─────┘  └────────┬─────────┘  └──────────┬──────────┘    │
├────────┼─────────────────┼────────────────────────┼──────────────┤
│        │    C++ App 层 (src/app)                  │              │
│  ┌─────▼─────┐  ┌────────▼─────────┐  ┌──────────▼──────────┐   │
│  │ Request   │  │ Notification     │  │ ProgressTracker     │   │
│  │ Service   │  │ Service          │  │                     │   │
│  │           │  │ (Kangaroo→Qt     │  │ mutex 保护的        │   │
│  │ QtConcur- │  │  桥接)           │  │ task map            │   │
│  │ rent 线程池│  │                  │  │                     │   │
│  └─────┬─────┘  └────────▲─────────┘  └──────────▲──────────┘   │
│        │                 │                       │              │
│  ┌─────▼─────┐           │                       │              │
│  │ MainThread│           │                       │              │
│  │ Executor  │           │                       │              │
│  └─────┬─────┘           │                       │              │
├────────┼─────────────────┼───────────────────────┼──────────────┤
│        │    C++ Libs 层 (src/libs)                │              │
│  ┌─────▼──────────────────────────────────────────┼──┐           │
│  │ EmbeddedPythonRuntime                         │  │           │
│  │   process(json, ProgressCallback)             │  │           │
│  └───────────────────────────────────────────────┘  │           │
│                                                     │           │
│  ┌──────────────────────────────────────────────────┘           │
│  │ INotificationSink (接口)                                     │
│  │   → libs 调用 sink->notify(channel, payload)                 │
│  └──────────────────────────────────────────────────────────────┘
├──────────────────────────────────────────────────────────────────┤
│                       Python 层                                  │
│  opengeolab_runtime.py                                           │
│    process(request_json, progress_callback=None)                 │
│    → 插件发现 / 执行 / invoke_ui                                  │
│    → plugin.execute(params, progress_callback=None)              │
└──────────────────────────────────────────────────────────────────┘
```

### 关键设计决策

| 决策 | 选择 | 理由 |
|------|------|------|
| 主线程调度 | 两个方法：`submitAsync()` + `executeOnMainThread()` | QML 已通过 `hasUI` 判断走哪条路径；不需要魔法 JSON 字段 |
| Kangaroo→Qt 桥接 | `QMetaObject::invokeMethod` + `Qt::QueuedConnection` | 标准 Qt 跨线程机制；无需自定义事件循环 |
| 进度回调 | `std::function` 传参，`process()` 内部在 GIL 范围内包装为 `py::cpp_function` | GIL 安全（包装在已持有 GIL 的上下文中）；公共 API 不暴露 pybind11 类型 |
| 高频流式推送 | NotificationService 带缓冲模式（16ms 窗口） | 防止 LLM token 流压垮事件队列 |
| 请求关联 | RequestService 生成 UUID `requestId`，响应中回显 | QML 可关联响应；支持多个并发请求 |

---

## 3. 模块 1：RequestService

> **文件**：`src/app/include/opengeolab/app/request_service.hpp` + `.cpp`
>
> **替代**：`ProcessService`
>
> **职责**：通用异步请求→响应桥接，支持请求追踪。

### 3.1 公共接口

```cpp
namespace OpenGeoLab::App {

class ProgressTracker;  // 前向声明

class RequestService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit RequestService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                            ProgressTracker& progress_tracker,
                            QObject* parent = nullptr);

    /// 析构函数等待所有 pending 的 QtConcurrent future 完成后返回，
    /// 与当前 ProcessService 行为一致，防止 worker 线程持有悬空引用。
    ~RequestService() override;

    /// 提交异步请求。返回生成的 requestId（UUID）。
    /// 请求通过 QtConcurrent 分发到 worker 线程。
    Q_INVOKABLE QString submitAsync(const QString& request_json);

    /// 在主线程上同步执行请求。
    /// 用于需要主线程上下文的操作（PySide6 UI）。
    /// 必须从主线程调用。返回 requestId。
    /// 注意：主线程请求不支持细粒度进度，直接从 begin 转到 complete。
    Q_INVOKABLE QString executeOnMainThread(const QString& request_json);

    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& requestId, const QString& responseJson);
    void errorOccurred(const QString& requestId, const QString& errorMessage);
    void busyChanged();

private:
    /// 解析 JSON，插入 "requestId" 字段，重新序列化。无效 JSON 时抛异常。
    static QString injectRequestId(const QString& json, const QString& request_id);

    /// 从 JSON 中提取 module + action 作为任务描述。
    static QString extractDescription(const QString& json);

    /// 根据 response JSON 中的 ok 字段分发 responseReady 或 errorOccurred。
    void emitResponse(const QString& request_id, const QString& response);

    OpenGeoLab::Python::EmbeddedPythonRuntime& runtime_;
    ProgressTracker& progress_tracker_;
    std::atomic<int> pending_count_{0};
    mutable std::mutex futures_mutex_;
    std::vector<QFuture<QString>> pending_futures_;
};

} // namespace OpenGeoLab::App
```

**设计说明**：`MainThreadExecutor` 不是 `RequestService` 的构造函数依赖。
QML 调用 `executeOnMainThread()` 时已经在主线程上，无需额外调度。
`MainThreadExecutor` 是独立服务，供库层代码按需使用。

### 3.2 请求生命周期

```
QML                    RequestService              Worker 线程
 │                          │                           │
 ├─ submitAsync(json) ──────►                           │
 │                          │ 生成 requestId (UUID)     │
 │  ◄── 返回 requestId ────┤                           │
 │                          │ 注入 requestId 到 JSON    │
 │                          │ pendingCount++            │
 │                          │ beginTask(id, desc)       │ → ProgressTracker
 │                          │                           │
 │                          ├── QtConcurrent::run ──────►
 │                          │                           │ runtime.process(json, cb)
 │                          │                           │   cb → ProgressTracker
 │                          │                           │ ◄── 响应 JSON
 │                          │  ◄── QFutureWatcher ──────┤
 │                          │ completeTask(id)          │ → ProgressTracker
 │                          │ pendingCount--            │
 │  ◄── responseReady ──────┤                           │
 │      (requestId, json)   │                           │
```

### 3.3 JSON 协议扩展

**请求**（形状不变，`requestId` 由 C++ 注入）：

```json
{
  "requestId": "a1b2c3d4-...",
  "module": "plugins",
  "action": "execute",
  "param": { "pluginName": "mesh_processor" }
}
```

**响应**（回显 `requestId`）：

```json
{
  "protocolVersion": "1.0",
  "requestId": "a1b2c3d4-...",
  "ok": true,
  "module": "plugins",
  "action": "execute",
  "summary": "Plugin 'mesh_processor' executed.",
  "result": {},
  "errors": []
}
```

### 3.4 行为说明

- `submitAsync()` 生成 UUID，将其作为 `requestId` 注入 JSON，然后分发到 `QtConcurrent::run`。
- `executeOnMainThread()` 生成 UUID，在调用线程（**必须是主线程**）上直接调用 `runtime_.process()`。
  主线程请求不支持细粒度进度（直接从 begin 转到 complete）。
- 两个方法都在开始时调用 `ProgressTracker::beginTask()`，结束时调用 `ProgressTracker::completeTask()`。
- Worker 线程构造一个 `std::function<void(double, std::string_view)>` 回调传递给
  `runtime_.process()`；`process()` 在 `gil_scoped_acquire` 内部将其包装为 `py::cpp_function`。
  **这确保了 GIL 安全——`py::cpp_function` 的创建始终在持有 GIL 的上下文中进行。**
- `requestId` 传递到 Python，Python 将其从请求复制到响应（原封不动）。

---

## 4. 模块 2：NotificationService

> **文件**：`src/app/include/opengeolab/app/notification_service.hpp` + `.cpp`
>
> **接口**：`src/libs/base/include/opengeolab/base/notification_sink.hpp`（位于 `libs/base` 基础模块）
>
> **职责**：将来自任意线程的推送通知桥接到 QML。

### 4.1 libs/base 基础模块

新建 `src/libs/base/` 作为所有 libs 共享的基础模块。使用 `opengeolab_add_module`
构建为动态库（`OPENGEOLAB_BUILD_SHARED_LIBS` 默认 ON）。

**目录结构**：
```
src/libs/base/
├── CMakeLists.txt
└── include/opengeolab/base/
    ├── base_export.hpp          ← opengeolab_add_module 自动生成
    └── notification_sink.hpp    ← INotificationSink 接口
```

**CMakeLists.txt**：
```cmake
set(base_public_headers include/opengeolab/base/notification_sink.hpp)

opengeolab_add_module(
    opengeolab_base
    ALIAS_NAME Base
    PUBLIC_HEADERS ${base_public_headers})
```

注意：`libs/base` 当前只有头文件接口，无 `.cpp` 源文件。但仍使用
`opengeolab_add_module` 以保持与其他 libs 模块一致的构建模式，
自动生成 `OPENGEOLAB_BASE_EXPORT` 宏和安装配置。

**父级 `src/libs/CMakeLists.txt`** 添加：
```cmake
add_subdirectory(base)
```

### 4.2 通知接收接口

```cpp
// src/libs/base/include/opengeolab/base/notification_sink.hpp
#pragma once

#include <opengeolab/base/base_export.hpp>

#include <string_view>

namespace OpenGeoLab::Base {

/// 抽象接收器，接收来自任意线程的推送通知。
/// app 层提供桥接到 Qt signal 的具体实现。
class OPENGEOLAB_BASE_EXPORT INotificationSink {
public:
    virtual ~INotificationSink() = default;

    /// 推送通知。线程安全——可从任意线程调用。
    /// @param channel 点分主题名（如 "llm.stream"、"task.complete"）。
    /// @param payload_json JSON 序列化的通知数据。
    virtual void notify(std::string_view channel, std::string_view payload_json) = 0;
};

} // namespace OpenGeoLab::Base
```

**设计说明**：`INotificationSink` 放在 `libs/base` 而非 `app` 层，使得
任何 libs 模块都可以链接 `OpenGeoLab::Base` 并通过该接口向 UI 推送消息，
而无需依赖 Qt 或 app 层代码。libs 默认构建为动态库（DLL）。

### 4.2 NotificationService（app 层）

```cpp
namespace OpenGeoLab::App {

class NotificationService : public QObject, public OpenGeoLab::Base::INotificationSink {
    Q_OBJECT

public:
    explicit NotificationService(QObject* parent = nullptr);

    /// INotificationSink 实现。线程安全。
    void notify(std::string_view channel, std::string_view payload_json) override;

    /// 为指定 channel 前缀启用缓冲投递。
    /// 该前缀下的消息在 interval_ms 毫秒内累积，作为 JSON 数组一次性投递。
    void enableBuffering(const QString& channel_prefix, int interval_ms = 16);

signals:
    /// 在主线程上触发，传递所有通知。
    void notificationReceived(const QString& channel, const QString& payload);
};

} // namespace OpenGeoLab::App
```

### 4.3 线程安全机制

```cpp
void NotificationService::notify(std::string_view channel, std::string_view payload_json) {
    // 捕获数据为 Qt 值类型（隐式共享，跨线程安全）。
    auto ch = QString::fromUtf8(channel.data(), static_cast<qsizetype>(channel.size()));
    auto pl = QString::fromUtf8(payload_json.data(), static_cast<qsizetype>(payload_json.size()));

    // QueuedConnection 将投递序列化到主线程事件循环。
    QMetaObject::invokeMethod(
        this,
        [this, ch = std::move(ch), pl = std::move(pl)]() {
            emit notificationReceived(ch, pl);
        },
        Qt::QueuedConnection);
}
```

### 4.4 高频流式缓冲模式

对高频更新的 channel（如 `llm.stream`），NotificationService 提供可选的缓冲投递：

- 内部 `std::unordered_map<std::string, BufferState>` 持有待发负载。
- 每个 `BufferState` 包含 `std::mutex`、`std::vector<std::string>` 和 `bool timer_scheduled`。
- 当缓冲 channel 收到第一条消息时，调度 `QTimer::singleShot` 在 `interval_ms` 后 flush。
- flush 收集所有待发负载为 JSON 数组，触发一次 `notificationReceived`。

**默认行为**：立即投递（无缓冲）。缓冲按 channel 前缀可选启用。

### 4.5 QML 用法

```qml
Connections {
    target: NotificationService

    function onNotificationReceived(channel, payload) {
        if (channel === "llm.stream") {
            const tokens = JSON.parse(payload);
            chatView.appendTokens(tokens);
        }
    }
}
```

### 4.6 Kangaroo Signal 集成

库可以在内部使用 `Kangaroo::Util::Signal`，通过 Service 封装的 `connect` 接口
让 app 层订阅通知。**不直接暴露 Signal 成员变量**，而是提供类型安全的连接方法：

```cpp
// 在某个 lib 的 service 类中（链接 OpenGeoLab::Base）：
#include <opengeolab/base/notification_sink.hpp>
#include <kangaroo/util/signal.hpp>

class OPENGEOLAB_SOMELIB_EXPORT SomeService {
public:
    /// 连接通知回调。返回 ScopedConnection，RAII 自动断开。
    /// @param callback 接收 (channel, payload_json) 的回调函数。
    [[nodiscard]] Kangaroo::Util::ScopedConnection connectNotification(
        std::function<void(const std::string&, const std::string&)> callback) {
        return notification_signal_.connect(std::move(callback));
    }

private:
    Kangaroo::Util::Signal<std::string, std::string> notification_signal_;

    void doWork() {
        // ... 处理完成后推送通知
        notification_signal_.emit("data.ready", R"({"count": 42})");
    }
};
```

**app 层接线**（在 `main.cpp` 中）：

```cpp
// ScopedConnection 的生命周期与 main() 绑定，自动断开。
auto notification_connection = someService.connectNotification(
    [&notificationService](const std::string& channel, const std::string& payload) {
        notificationService.notify(channel, payload);
    });
```

**设计要点**：
- Signal 成员是 `private`，外部只能通过 `connectNotification()` 订阅。
- 返回 `Kangaroo::Util::ScopedConnection`（RAII），无需手动断开。
- 回调在发射者线程执行；`notificationService.notify()` 内部通过
  `QueuedConnection` 跨线程投递到主线程——整条链路线程安全。
- libs 模块不依赖 Qt——只依赖 `OpenGeoLab::Base` 和 Kangaroo 头文件。

Kangaroo slot 在发射者的线程上执行。由于 `notify()` 线程安全，`QueuedConnection` 处理跨线程跳转。

---

## 5. 模块 3：ProgressTracker

> **文件**：`src/app/include/opengeolab/app/progress_tracker.hpp` + `.cpp`
>
> **职责**：管理并发任务进度，向 QML 暴露聚合状态。

### 5.1 公共接口

```cpp
namespace OpenGeoLab::App {

class ProgressTracker : public QObject {
    Q_OBJECT

    /// 最近活跃任务的聚合进度。
    /// -1 = 无活跃任务，0 = 不确定进度，(0,1] = 确定进度。
    Q_PROPERTY(double currentProgress READ currentProgress NOTIFY progressChanged)

    /// 最近活跃任务的描述文本。
    Q_PROPERTY(QString statusText READ statusText NOTIFY progressChanged)

    /// 至少有一个任务活跃时为 true。
    Q_PROPERTY(bool hasActiveTasks READ hasActiveTasks NOTIFY progressChanged)

public:
    explicit ProgressTracker(QObject* parent = nullptr);

    /// 创建新的追踪任务。线程安全。
    /// @param task_id 唯一标识（通常是 RequestService 的 requestId）。
    /// @param description 人类可读的任务描述。
    void beginTask(const QString& task_id, const QString& description);

    /// 更新已追踪任务的进度。线程安全。
    /// @param task_id 任务标识。
    /// @param progress [0, 1] 为确定进度，0 为不确定进度。
    /// @param message 可选状态消息。
    void updateProgress(const QString& task_id, double progress,
                        const QString& message = {});

    /// 标记任务完成。线程安全。
    /// @param task_id 任务标识。
    /// @param success true 表示成功完成，false 表示失败。
    void completeTask(const QString& task_id, bool success = true);

    [[nodiscard]] double currentProgress() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] bool hasActiveTasks() const;

signals:
    void progressChanged();
};

} // namespace OpenGeoLab::App
```

### 5.2 内部状态

```cpp
struct TaskState {
    QString description;
    QString message;
    double progress = 0.0;          // [0, 1] 或 0 表示不确定
    std::chrono::steady_clock::time_point last_update;
    bool completed = false;
    bool success = true;
};

// 由 mutable std::mutex 保护
std::unordered_map<QString, TaskState> tasks_;
```

### 5.3 线程安全

所有公共方法遵循相同模式：

```
1. 加锁 mutex_ → 更新 tasks_ map → 解锁
2. QMetaObject::invokeMethod(this, &ProgressTracker::progressChanged, Qt::QueuedConnection)
```

mutex 保护数据结构；`QueuedConnection` 确保 Qt signal 在主线程触发。

### 5.4 聚合策略

`currentProgress()` 返回**最近更新的活跃任务**（非完成任务中 `last_update` 最大的）的进度：

```cpp
double ProgressTracker::currentProgress() const {
    std::lock_guard lock(mutex_);

    const TaskState* latest = nullptr;
    for (const auto& [id, task] : tasks_) {
        if (task.completed) continue;
        if (!latest || task.last_update > latest->last_update) {
            latest = &task;
        }
    }

    return latest ? latest->progress : -1.0;
}
```

已完成的任务保留短时间（让 `ActivityOverlay` 显示 "Done" / "Failed" 状态），
然后由周期性 `QTimer` 清理（10 秒后）。

### 5.5 Python 进度回调

进度回调由 `RequestService` 在 worker 线程上构造为 `std::function`，传递给
`EmbeddedPythonRuntime::process()`。**关键**：C++ 回调类型是 `std::function`，
不是 `pybind11::object`——`py::cpp_function` 的包装在 `process()` 内部的
`gil_scoped_acquire` 范围内完成。

```cpp
// 在 RequestService 的 worker 线程中（QtConcurrent::run 内部）：
EmbeddedPythonRuntime::ProgressCallback progress_callback =
    [tracker = &progress_tracker_, task_id](double pct, std::string_view msg) {
        tracker->updateProgress(
            task_id,
            pct,
            QString::fromUtf8(msg.data(), static_cast<qsizetype>(msg.size())));
    };

const auto response = runtime_.process(json, std::move(progress_callback));
```

**GIL 安全保证**：
1. `std::function` 的构造不涉及 Python 对象——在 GIL 之外完全安全。
2. `process()` 内部先 `gil_scoped_acquire`，然后在 GIL 范围内将 `std::function`
   包装为 `py::cpp_function`。
3. Python 调用回调时仍在同一个 GIL scope 内——C++ lambda 本身不操作 Python 对象，
   只调用 `ProgressTracker::updateProgress()`（纯 C++/Qt）。

### 5.6 EmbeddedPythonRuntime 签名变更

```cpp
// 变更前：
[[nodiscard]] std::string process(std::string_view request_json);

// 变更后：
using ProgressCallback = std::function<void(double, std::string_view)>;

[[nodiscard]] std::string process(std::string_view request_json,
                                   ProgressCallback progress_callback = nullptr);
```

实现中在 GIL 范围内包装回调：

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

**关键设计**：`ProgressCallback` 是 `std::function`，不是 `pybind11::object`。
公共头文件不包含任何 pybind11 类型，保持了当前的封装性——pybind11 依赖完全隐藏在 `.cpp` 文件中。

### 5.7 Python Runtime 签名变更

```python
# 变更前：
def process(request_json: str) -> str:

# 变更后：
def process(request_json: str, progress_callback=None) -> str:
```

`_make_response()` 增加 `request_id` 参数：

```python
def _make_response(
    module: str,
    action: str,
    ok: bool,
    summary: str,
    result: dict[str, Any] | None = None,
    errors: list[str] | None = None,
    request_id: str = "",
) -> str:
    """构建 JSON 响应信封。"""
    response: dict[str, Any] = {
        "protocolVersion": PROTOCOL_VERSION,
        "ok": ok,
        "module": module,
        "action": action,
        "summary": summary,
        "result": result or {},
        "errors": errors or [],
    }
    if request_id:
        response["requestId"] = request_id
    return json.dumps(response)
```

`process()` 提取 `requestId` 并传递给所有 handler：

```python
def process(request_json: str, progress_callback=None) -> str:
    try:
        request = json.loads(request_json)
    except json.JSONDecodeError as exc:
        return _make_response("unknown", "unknown", False, f"Invalid JSON: {exc}")

    module = request.get("module", "")
    action = request.get("action", "")
    request_id = request.get("requestId", "")

    if module == "plugins":
        if action == "list":
            return _plugins_response(request_id)
        if action == "execute":
            return _execute_plugin(request, request_id, progress_callback)
        if action == "invoke_ui":
            return _launch_plugin_ui(request, request_id)
        return _make_response(module, action, False,
                              f"Unknown plugins action: {action}",
                              request_id=request_id)
    # ... 其余路由类似
```

插件 `execute()` 函数接收回调作为可选参数：

```python
def _execute_plugin(request: dict[str, Any], request_id: str,
                    progress_callback=None) -> str:
    param = request.get("param", {})
    plugin_name = param.get("pluginName", "")
    # ...
    mod = importlib.import_module(plugin_name)
    sig = inspect.signature(mod.execute)
    if progress_callback and "progress_callback" in sig.parameters:
        result = mod.execute(param, progress_callback=progress_callback)
    else:
        result = mod.execute(param)
    return _make_response("plugins", "execute", True,
                          f"Plugin '{plugin_name}' executed.", result,
                          request_id=request_id)
```

### 5.8 ActivityOverlay 集成

```qml
// 在 Main.qml 中 — 将 ActivityOverlay 绑定到 ProgressTracker
ActivityOverlay {
    // ...
    progress: ProgressTracker.hasActiveTasks
        ? ProgressTracker.currentProgress : -1
    progressStatus: ProgressTracker.statusText
}
```

---

## 6. 模块 4：MainThreadExecutor

> **文件**：`src/app/include/opengeolab/app/main_thread_executor.hpp` + `.cpp`
>
> **职责**：在主线程上执行操作（PySide6 窗口创建、OpenGL 上下文访问等）。

### 6.1 公共接口

```cpp
namespace OpenGeoLab::App {

class MainThreadExecutor : public QObject {
    Q_OBJECT

public:
    explicit MainThreadExecutor(QObject* parent = nullptr);

    /// 在主线程上执行 callable。线程安全。
    /// 如果已在主线程上，立即执行。
    /// 否则通过 QMetaObject::invokeMethod 排队。
    void execute(std::function<void()> task);

    /// 在主线程上执行 callable 并阻塞直到完成。线程安全。
    /// 如果已在主线程上，立即执行。
    /// 否则使用 BlockingQueuedConnection。
    ///
    /// **约束**：调用者在调用 executeBlocking() 时不得持有 GIL——
    /// 如果 task 需要获取 GIL，会导致死锁：
    ///   调用线程：持有 GIL → 等待主线程
    ///   主线程：尝试获取 GIL → 被调用线程阻塞
    /// 如果不确定，使用非阻塞的 execute()。
    void executeBlocking(std::function<void()> task);
};

} // namespace OpenGeoLab::App
```

### 6.2 实现

```cpp
void MainThreadExecutor::execute(std::function<void()> task) {
    if (QThread::currentThread() == thread()) {
        task();
        return;
    }

    QMetaObject::invokeMethod(
        this,
        [task = std::move(task)]() { task(); },
        Qt::QueuedConnection);
}

void MainThreadExecutor::executeBlocking(std::function<void()> task) {
    if (QThread::currentThread() == thread()) {
        task();
        return;
    }

    // Debug 构建中断言调用者未持有 GIL，防止死锁。
    Q_ASSERT(PyGILState_Check() == 0);

    QMetaObject::invokeMethod(
        this,
        [task = std::move(task)]() { task(); },
        Qt::BlockingQueuedConnection);
}
```

### 6.3 独立服务说明

`MainThreadExecutor` 是独立服务，**不是** `RequestService` 的依赖。

- `RequestService::executeOnMainThread()` 从 QML 调用，已在主线程上，
  直接调用 `runtime_.process()` 即可。
- `MainThreadExecutor` 的用户是**库层代码**——当 worker 线程需要在主线程上
  执行操作时使用。

### 6.4 库层用法（未来）

```cpp
// 一个需要创建 OpenGL 资源的库 service：
void SomeService::initializeResources(MainThreadExecutor& executor) {
    // 先释放 GIL（如果持有）
    pybind11::gil_scoped_release release;
    executor.executeBlocking([this]() {
        // 在主线程上运行，OpenGL 上下文可用。
        createGLTexture();
    });
}
```

---

## 7. QML 集成变更

### 7.1 服务注册（main.cpp）

```cpp
// 创建服务
OpenGeoLab::App::ProgressTracker progress_tracker;
OpenGeoLab::App::NotificationService notification_service;
OpenGeoLab::App::MainThreadExecutor main_thread_executor;
OpenGeoLab::App::RequestService request_service(python_runtime, progress_tracker);

// 注册为 QML 单例
qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0,
    "RequestService", &request_service);
qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0,
    "NotificationService", &notification_service);
qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0,
    "ProgressTracker", &progress_tracker);
qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0,
    "MainThreadExecutor", &main_thread_executor);
```

### 7.2 Main.qml 变更

**变更前**（ProcessService）：

```qml
import OpenGeoLab.Services

Connections {
    target: ProcessService
    function onResponseReady(responseJson) { /* ... */ }
    function onErrorOccurred(errorMessage) { /* ... */ }
}

Component.onCompleted: {
    ProcessService.submitRequest(JSON.stringify({
        module: "plugins", action: "list", param: {}
    }));
}

// 插件 UI 启动：
ProcessService.submitRequest(JSON.stringify({
    module: "plugins", action: "invoke_ui",
    param: { pluginName: name }
}));
```

**变更后**（RequestService）：

```qml
import OpenGeoLab.Services

Connections {
    target: RequestService
    function onResponseReady(requestId, responseJson) {
        const resp = JSON.parse(responseJson);
        if (resp.module === "plugins" && resp.action === "list" && resp.ok) {
            root.pluginList = resp.result.plugins || [];
        }
    }
    function onErrorOccurred(requestId, errorMessage) {
        root.statusNote = qsTr("Error: %1").arg(errorMessage);
    }
}

Component.onCompleted: {
    RequestService.submitAsync(JSON.stringify({
        module: "plugins", action: "list", param: {}
    }));
}

// 插件 UI 启动（需要主线程）：
RequestService.executeOnMainThread(JSON.stringify({
    module: "plugins", action: "invoke_ui",
    param: { pluginName: name }
}));

// 插件执行（异步）：
RequestService.submitAsync(JSON.stringify({
    module: "plugins", action: "execute",
    param: { pluginName: name }
}));
```

### 7.3 ActivityOverlay 绑定

```qml
ActivityOverlay {
    // ...
    progress: ProgressTracker.hasActiveTasks
        ? ProgressTracker.currentProgress : -1
    progressStatus: ProgressTracker.statusText
}
```

---

## 8. 文件清单

### 新增文件

| 文件 | 层 | 用途 |
|------|----|------|
| `src/libs/base/CMakeLists.txt` | libs | base 基础模块构建配置 |
| `src/libs/base/include/opengeolab/base/notification_sink.hpp` | libs | INotificationSink 接口 |
| `src/app/include/opengeolab/app/request_service.hpp` | app | RequestService 头文件 |
| `src/app/src/request_service.cpp` | app | RequestService 实现 |
| `src/app/include/opengeolab/app/notification_service.hpp` | app | NotificationService 头文件 |
| `src/app/src/notification_service.cpp` | app | NotificationService 实现 |
| `src/app/include/opengeolab/app/progress_tracker.hpp` | app | ProgressTracker 头文件 |
| `src/app/src/progress_tracker.cpp` | app | ProgressTracker 实现 |
| `src/app/include/opengeolab/app/main_thread_executor.hpp` | app | MainThreadExecutor 头文件 |
| `src/app/src/main_thread_executor.cpp` | app | MainThreadExecutor 实现 |

### 修改文件

| 文件 | 变更 |
|------|------|
| `src/libs/CMakeLists.txt` | 添加 `add_subdirectory(base)` |
| `src/app/src/main.cpp` | 用 4 个服务实例 + 注册替换 ProcessService |
| `src/app/resource/qml/Main.qml` | 使用 RequestService + ProgressTracker 绑定 |
| `src/app/resource/qml/sections/ActivityOverlay.qml` | 绑定 progress 到 ProgressTracker |
| `src/app/CMakeLists.txt` | 添加新源文件 |
| `src/libs/python/python_embedded/include/.../embedded_python_runtime.hpp` | 添加 `ProgressCallback` 参数 |
| `src/libs/python/python_embedded/src/embedded_python_runtime.cpp` | 在 GIL 范围内包装回调 |
| `src/app/resource/python/opengeolab_runtime.py` | 接受 + 转发 `progress_callback`；`_make_response` 添加 `request_id` |

### 删除文件

| 文件 | 原因 |
|------|------|
| `src/app/include/opengeolab/app/process_service.hpp` | 被 RequestService 替代 |
| `src/app/src/process_service.cpp` | 被 RequestService 替代 |

---

## 9. 迁移路径

替换**不是增量的**——在一个分支中移除 `ProcessService`，替换为四个新服务。理由：

1. `ProcessService` 只被 `main.cpp` 和 `Main.qml` 消费。
2. 协议不变（JSON 信封）；只有 QML API 表面变化。
3. 所有测试（当前 ProcessService 无测试）将针对新服务。

### 迁移步骤

1. 创建 `src/libs/base/` 模块，包含 `INotificationSink` 接口
2. 实现 `ProgressTracker`（无依赖）
3. 实现 `MainThreadExecutor`（无依赖）
4. 实现 `NotificationService`（依赖 `OpenGeoLab::Base`）
5. 实现 `RequestService`（依赖 ProgressTracker）
6. 更新 `EmbeddedPythonRuntime::process()` 签名
7. 更新 `opengeolab_runtime.py` 接受 + 转发 `progress_callback` 和 `requestId`
8. 更新 `main.cpp` 服务接线
9. 更新 QML 文件（Main.qml、ActivityOverlay.qml）
10. 删除 `ProcessService` 文件
11. 构建验证 + 手动测试

---

## 10. 错误处理

### RequestService

- Python 异常 → `pybind11::error_already_set` → C++ 捕获 →
  `errorOccurred(requestId, message)` 信号。
- JSON 解析失败 → Python runtime 检测 → 返回 `{ "ok": false }` 信封。

### NotificationService

- `QueuedConnection` 投递是 fire-and-forget；如果接收者已销毁，Qt 静默丢弃。
- 缓冲模式：如果 flush 失败（如 JSON 序列化错误），清空缓冲并记录错误日志。

### ProgressTracker

- `updateProgress()` / `completeTask()` 中的未知 `task_id` → 静默忽略
  （任务可能已被清理）。
- 已完成的任务在 10 秒后清理。

### MainThreadExecutor

- 如果 task 抛异常，异常在主线程传播。调用者需自行 try-catch。
- `executeBlocking()` + GIL 死锁在 Debug 构建中由 `Q_ASSERT` 检测。

---

## 11. 测试策略

| 测试 | 类型 | 验证内容 |
|------|------|----------|
| `RequestService::submitAsync` 完成并触发 `responseReady` | 单元 | 异步生命周期 |
| `RequestService::executeOnMainThread` 同步返回响应 | 单元 | 主线程路径 |
| `ProgressTracker` 从多线程接收更新 | 单元 | 线程安全 |
| `ProgressTracker::currentProgress` 聚合正确 | 单元 | 聚合逻辑 |
| `NotificationService::notify` 从 worker 线程投递到主线程 | 单元 | 跨线程桥接 |
| `NotificationService` 缓冲模式合并消息 | 单元 | 缓冲逻辑 |
| 带进度回调的插件执行更新 ProgressTracker | 集成 | 端到端进度 |
| PySide6 插件 UI 通过 `executeOnMainThread` 非模态启动 | 手动 | PySide6 兼容 |
| Windows RelWithDebInfo 构建通过 | CI | 无回归 |

---

## 12. 约束与非目标

### 约束

- **pybind11 3.0.2**：`py::cpp_function` 必须与此版本兼容。
- **Windows + MSVC**：所有线程安全机制必须在 Windows 上工作。
- **GIL**：Worker 线程调用 Python 前必须获取 GIL；进度回调在此范围内执行。
- **PySide6 Release-only**：PySide6 UI 的主线程要求不变；
  Debug 构建通过 `OPENGEOLAB_ENABLE_PYSIDE6` 禁用 PySide6。

### 非目标

- **流式响应协议**：不实现 `process()` 的分块/流式响应。流式数据通过 `NotificationService`。
- **任务列表的 QAbstractListModel**：YAGNI — 只有 `ActivityOverlay` 消费进度；
  如果需要任务列表视图再升级。
- **取消任务**：不在本次范围内。可作为后续通过传递 cancellation token 实现。
- **持久化请求历史**：完成的请求在进度清理窗口之后不保留。
