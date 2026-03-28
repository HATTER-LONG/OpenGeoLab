# Command 直接调度重构设计规格

## 背景与问题

当前所有请求（含纯 C++ 模块如 geometry、io）从 QML 层发起后，必须经过完整的 Python 路径：

```
QML → RequestService → EmbeddedPythonRuntime (acquire GIL)
    → Python opengeolab_runtime.process()
        → lazy-import opengeolab_pywrapper
            → pybind11 process() → CommandDispatcher → C++ Module
```

**问题：**

1. **GIL 开销**：纯 C++ 模块无需 Python，却被迫经过 GIL acquire/release，产生不必要的线程竞争
2. **模块不可达**：`CommandDispatcher` 是 pybind11 模块内的 `static` 变量，app 层无法访问。Sidebar Shape Explorer 需要直接获取 `ShapeStore` 引用来建立通知桥接，当前架构无法实现
3. **分层耦合**：QML/app 层的纯 C++ 请求被迫依赖 Python 运行时初始化成功，增加故障面

## 目标

1. C++ 模块请求（geometry、io）从 app 层直接调度，不经过 Python/GIL
2. Python 路径保留给 plugins、system 等需要 Python 运行时的请求
3. app 层可以获取 C++ 模块实例（如 `GeometryModule → ShapeStore`），为后续 Sidebar 通知提供基础
4. pybind11 模块继续正常工作（Python 脚本、AI agent、外部插件仍可通过 Python 桥接调用 C++ 模块）
5. 变更不破坏已有 QML、Python 或测试代码

## 非目标

- 不删除 Python 桥接路径（它对 AI agent、外部脚本、Python 插件仍有价值）
- 不改变 request/response JSON 协议格式
- 不引入新的序列化或 RPC 机制
- 不在本规格中实现 Sidebar（由 sidebar 规格覆盖）

## 重构后架构

```
QML → RequestService
  ├─ hasModule(name)==true → CommandDispatcher.dispatch()  [直接 C++, 无 GIL]
  └─ hasModule(name)==false → EmbeddedPythonRuntime.process()  [Python 路径]
```

两条路径共存于同一 `RequestService` 中，由请求 JSON 中的 `module` 字段决定路由。

**两个 CommandDispatcher 实例共存：**

| 实例 | 所在位置 | 用途 |
|------|---------|------|
| app 层 dispatcher | main.cpp 栈变量 | RequestService 直接调用 |
| pybind11 dispatcher | python_wrapper_module.cpp static | Python 脚本/AI agent 调用 |

两者都引用同一个 `g_PluginComponentFactory` 全局单例。由于模块通过 `bindSingleton` 注册，`getSharedInstance` 始终返回同一个 `shared_ptr`，因此两个 dispatcher 缓存的模块实例完全一致，不存在状态分裂。

## 详细变更

### 5.1 registerBuiltinModules 幂等化

**文件：** `src/libs/command/src/module_registry.cpp`

**原因：** app 层 main.cpp 在启动时调用 `registerBuiltinModules`。Python 路径的 pybind11 模块也会在首次 `process()` 调用时通过 `std::call_once` 调用同一函数。两次调用 `bindSingleton` 同名模块会抛出 `ComponentFactoryAlreadyRegisteredEx`。

**方案：** 使用 check-then-register 模式，查询工厂现有注册状态来跳过已注册模块：

```cpp
void registerBuiltinModules(PluginComponentFactory& factory) {
    auto existing = factory.listFactories<Core::ModuleBase>();
    auto is_registered = [&](std::string_view name) {
        return std::ranges::any_of(existing,
                                   [&](const auto& info) { return info.m_moduleName == name; });
    };

    if (!is_registered(IO::IOModule::MODULE_NAME)) {
        factory.bindSingleton<Core::ModuleBase, IO::IOModule>(
            IO::IOModule::MODULE_NAME, std::ref(factory));
        LOG_INFO("Registered module '{}'", IO::IOModule::MODULE_NAME);
    }
    if (!is_registered(Geometry::GeometryModule::MODULE_NAME)) {
        factory.bindSingleton<Core::ModuleBase, Geometry::GeometryModule>(
            Geometry::GeometryModule::MODULE_NAME, std::ref(factory));
        LOG_INFO("Registered module '{}'", Geometry::GeometryModule::MODULE_NAME);
    }
}
```

**为什么不用 `std::once_flag`：** 静态库构建时，`opengeolab_command` 会被链接到 `opengeolab_app.exe` 和 `opengeolab_pywrapper.pyd` 两个二进制中，各自拥有独立的 `static std::once_flag` 副本，两者都会触发注册，导致 `bindSingleton` 对同一全局工厂重复注册而抛异常。check-then-register 模式查询的是共享的工厂状态，无论从哪里调用都安全。

**线程安全注意：** `registerBuiltinModules` 必须在 `EmbeddedPythonRuntime` 构造之前从 main.cpp 调用。本设计保证 main.cpp 的调用先于 pybind11 模块加载，不存在并发注册。pybind11 模块内的 `std::call_once` 可保留（额外保护层）或移除（无害）。

### 5.2 CommandDispatcher 新增 findModule() 公共接口

**文件：** `src/libs/command/include/opengeolab/command/command_dispatcher.hpp`

**新增方法：**

```cpp
/// @brief 按名称获取已注册的模块实例。
/// @param module_name 模块名
/// @return 模块的 shared_ptr；如未注册则返回 nullptr
[[nodiscard]] std::shared_ptr<Core::ModuleBase> findModule(const std::string& module_name) const;
```

**实现：** 先调用 `hasModule()` 检查注册状态，如已注册则委托 `getModule()`，否则返回 `nullptr`。不直接委托 `getModule()` 是因为 Kangaroo 的 `getSharedInstance` 对未注册名会抛 `ComponentFactoryNotRegisteredEx`，`findModule` 语义应为安全查找。

```cpp
std::shared_ptr<Core::ModuleBase> CommandDispatcher::findModule(const std::string& module_name) const {
    if (!hasModule(module_name)) {
        return nullptr;
    }
    return getModule(module_name);
}
```

### 5.3 main.cpp 变更

**文件：** `src/app/src/main.cpp`

**变更顺序：**

```cpp
// 1. 注册内建模块（在 Python runtime 初始化之前）
OpenGeoLab::Command::registerBuiltinModules(g_PluginComponentFactory);

// 2. 创建 app 层 CommandDispatcher
OpenGeoLab::Command::CommandDispatcher dispatcher(g_PluginComponentFactory);

// 3. 创建 Python runtime（同之前）
OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime python_runtime(app_dir, runtime_dir, plugin_dir);

// 4. RequestService 接受双引用
OpenGeoLab::App::RequestService request_service(dispatcher, python_runtime);
```

**关键约束：**
- `registerBuiltinModules` 必须在 `EmbeddedPythonRuntime` 构造之前调用，确保 pybind11 模块首次被 Python 使用时模块已注册
- `dispatcher` 的生命周期由 main 函数栈管理，与 `python_runtime` 相同

### 5.4 RequestService 双路径路由

**文件：** `src/app/include/opengeolab/app/request_service.h`、`src/app/src/request_service.cpp`

**头文件变更：**

```cpp
// 新增前向声明
namespace OpenGeoLab::Command {
class CommandDispatcher;
}

class RequestService : public QObject {
public:
    // 构造函数新增 dispatcher 参数
    explicit RequestService(OpenGeoLab::Command::CommandDispatcher& dispatcher,
                            OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                            QObject* parent = nullptr);
    // ... 其余不变 ...

private:
    OpenGeoLab::Command::CommandDispatcher& m_dispatcher;
    OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& m_runtime;
};
```

**submitAsync 路由逻辑：**

```cpp
void RequestService::submitAsync(const QString& request_json) {
    auto [description, process_json, module_name, muted] = prepareRequest(request_json);
    // ... emit requestSent, busyChanged (同之前) ...

    if (m_dispatcher.hasModule(module_name)) {
        // === C++ 直接路径 ===
        auto future = QtConcurrent::run([this, json = process_json.toStdString(),
                                          cb = std::move(progress_cb), muted, desc]() -> QString {
            nlohmann::json request;
            try {
                request = nlohmann::json::parse(json);
            } catch (const nlohmann::json::parse_error& e) {
                nlohmann::json err = {{"ok", false},
                                      {"summary", "Invalid JSON in request"},
                                      {"errors", nlohmann::json::array({std::string(e.what())})}};
                return QString::fromStdString(err.dump());
            }
            auto result = m_dispatcher.dispatch(request, cb);
            return QString::fromStdString(result.dump());
        });
        // ... watcher 连接同之前 ...
    } else {
        // === Python 路径（plugins, system, ...） ===
        auto future = QtConcurrent::run([this, json = process_json.toStdString(),
                                          cb = std::move(progress_cb), muted, desc]() -> QString {
            return QString::fromStdString(m_runtime.process(json, cb));
        });
        // ... watcher 连接同之前 ...
    }
}
```

**executeOnMainThread 路由逻辑：**

```cpp
void RequestService::executeOnMainThread(const QString& request_json) {
    auto [description, process_json, module_name, muted] = prepareRequest(request_json);
    // ... emit requestSent (同之前) ...

    try {
        std::optional<Kangaroo::Util::Stopwatch> sw;
        if (!muted) { sw.emplace(description.toStdString(), Core::getLoggerShared()); }

        if (m_dispatcher.hasModule(module_name)) {
            // C++ 直接路径，无需 GIL
            auto request = nlohmann::json::parse(process_json.toStdString());
            auto result = m_dispatcher.dispatch(request, nullptr);
            emitResponse(QString::fromStdString(result.dump()), muted);
        } else {
            // Python 路径：process() 内部 acquire GIL
            const auto response = QString::fromStdString(
                m_runtime.process(process_json.toStdString(), nullptr));
            emitResponse(response, muted);
        }
    } catch (const std::exception& exception) {
        LOG_ERROR("RequestService: main-thread request threw exception: {}{}", exception.what(),
                  muted ? " (muted)" : "");
        emit errorOccurred(QString::fromStdString(exception.what()), muted);
    }
}
```

**PreparedRequest 扩展（避免双重 JSON 解析）：**

`prepareRequest()` 已经解析 JSON 提取 `module` 和 `action`，将 `moduleName` 一并放入返回结构，避免路由时再次解析：

```cpp
struct PreparedRequest {
    QString description;
    QString processJson;
    std::string moduleName;  // 新增：供路由判断使用
    bool muted = false;
};

PreparedRequest RequestService::prepareRequest(const QString& json) {
    auto document = QJsonDocument::fromJson(json.toUtf8());
    auto object = document.object();
    const auto module = object.value("module").toString(QStringLiteral("unknown"));
    const auto action = object.value("action").toString(QStringLiteral("unknown"));
    const bool muted = object.value("mute").toBool(false);
    return {
        QStringLiteral("%1.%2").arg(module, action),
        json,
        module.toStdString(),
        muted,
    };
}
```

### 5.5 CMake 链接变更

**文件：** `src/app/CMakeLists.txt`

```cmake
target_link_libraries(
    opengeolab_app
    PRIVATE ...
            OpenGeoLab::Python_Embed
            OpenGeoLab::Command)        # 新增
```

注意：`OpenGeoLab::Command` 传递性链接了 `OpenGeoLab::Core`、`OpenGeoLab::IO`、`OpenGeoLab::Geometry`，因此 app 层自动获得这些依赖。

### 5.6 pybind11 模块：无破坏性变更

**文件：** `src/libs/python/python_wrapper/src/python_wrapper_module.cpp`

**无需修改。** 由于 `registerBuiltinModules` 已改为幂等，pybind11 模块内的 `std::call_once` + `registerBuiltinModules` 继续正常工作：
- 如果 main.cpp 先调用了注册，pybind11 内的调用无操作
- 如果以独立 Python 脚本形式使用（无 app 层），pybind11 仍会完成注册

## 请求路由决策表

| 请求示例 module 字段 | hasModule 结果 | 路径 | 备注 |
|---------------------|---------------|------|------|
| `"geometry"` | true | C++ 直接 | 无 GIL |
| `"io"` | true | C++ 直接 | 无 GIL |
| `"plugins.xxx"` | false | Python | 插件管理 |
| `"system"` | false | Python | 系统能力 |
| `""` 或缺失 | false | Python | Python 返回结构化错误 |
| 未来新 C++ 模块 | true | C++ 直接 | 自动路由 |

## 线程安全

| 组件 | 线程安全策略 | 变更 |
|------|------------|------|
| `CommandDispatcher.dispatch()` | 已有 `m_cacheMutex` 保护模块缓存；模块自身线程安全 | 无变更 |
| `RequestService` 并发 | QtConcurrent worker 线程；`m_futuresMutex` 保护 pending 列表 | 无变更 |
| `registerBuiltinModules` | check-then-register 查询共享工厂状态；main.cpp 在单线程启动阶段调用，先于 pybind11 模块加载 | 新增 |
| `g_PluginComponentFactory` | Kangaroo 内部线程安全 | 无变更 |

## 已知文档不一致

`command_dispatcher.hpp` 中 `dispatch()` 的 Doxygen 注释声明会抛出 `std::invalid_argument` 和 `ComponentFactoryNotRegisteredEx`，但实际实现（`command_dispatcher.cpp:33–65`）对这些情况返回错误 JSON 而非抛异常。本次重构中 RequestService 的路由逻辑依赖 `dispatch()` 不抛异常的实际行为。建议在实现时一并修正 Doxygen 注释使其与实际行为一致。

## 向后兼容

1. **QML 层**：无任何变更。RequestService 的 QML API（`submitAsync`、`executeOnMainThread`、signals）完全不变
2. **Python 脚本**：`opengeolab_pywrapper.process()` 行为完全不变
3. **JSON 协议**：request/response 格式不变
4. **测试**：现有 `command_dispatcher_test` 不受影响（使用独立 factory）

## 测试策略

1. **现有测试**：8 个 geometry/command 测试必须继续通过
2. **路由验证**：通过日志确认 C++ 模块走直接路径（`CommandDispatcher: dispatching`）、Python 模块走 Python 路径
3. **端到端**：在 app 中执行 `create_box` → 确认响应正常、日志中无 Python/GIL 相关输出
4. **Python 兼容**：在 Python 控制台执行 `opengeolab_pywrapper.process(...)` → 确认仍正常工作

## 后续工作

本重构完成后，将为 Sidebar Shape Explorer 提供基础：

- main.cpp 可通过 `dispatcher.findModule("geometry")` 获取 `GeometryModule`
- 从 `GeometryModule::shapeStore()` 获取 `ShapeStore&`
- 创建 `ShapeStoreNotifier(store)` 桥接 Kangaroo 信号到 Qt 信号
- 详见 `2026-03-28-sidebar-shape-explorer-design.md`
