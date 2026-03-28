# Command 直接调度重构 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 让 C++ 模块（geometry、io）从 RequestService 直接调度，绕过 Python/GIL；同时保留 Python 路径给 plugins/system

**架构：** RequestService 持有 CommandDispatcher 和 EmbeddedPythonRuntime 双引用。根据 `hasModule(name)` 结果路由：C++ 模块走 dispatcher.dispatch()，其余走 runtime.process()。两个 dispatcher（app 层和 pybind11 层）共享同一 g_PluginComponentFactory，模块实例完全一致。

**技术栈：** C++20, CMake, Qt 6, nlohmann/json, Kangaroo PluginComponentFactory, pybind11, doctest

**规格文档：** `docs/superpowers/specs/2026-03-28-command-direct-dispatch-design.md`

---

## 文件变更概览

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/libs/command/src/module_registry.cpp` | 修改 | check-then-register 幂等化 |
| `src/libs/command/include/opengeolab/command/command_dispatcher.hpp` | 修改 | 新增 `findModule()` 公共接口 + 修正 `dispatch()` Doxygen |
| `src/libs/command/src/command_dispatcher.cpp` | 修改 | 实现 `findModule()` |
| `src/libs/command/test/command_dispatcher_test.cpp` | 修改 | 新增幂等性测试 + findModule 测试 |
| `src/app/CMakeLists.txt` | 修改 | 新增 `OpenGeoLab::Command` 链接 |
| `src/app/include/opengeolab/app/request_service.h` | 修改 | 新增 CommandDispatcher 引用 + 扩展 PreparedRequest |
| `src/app/src/request_service.cpp` | 修改 | 双路径路由逻辑 |
| `src/app/src/main.cpp` | 修改 | 注册模块 + 创建 dispatcher + 传入 RequestService |

**不修改的关键文件：**
- `src/libs/python/python_wrapper/src/python_wrapper_module.cpp` — pybind11 模块保持不变

---

### 任务 0：基线验证

**目标：** 确认当前代码构建和测试正常

- [ ] 步骤 1：执行构建
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```
- [ ] 步骤 2：执行测试
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```
- [ ] 步骤 3：确认全部测试通过（预期 8/8）

---

### 任务 1：registerBuiltinModules 幂等化

**文件：**
- 修改：`src/libs/command/src/module_registry.cpp`
- 测试：`src/libs/command/test/command_dispatcher_test.cpp`

**步骤：**

- [ ] 步骤 1：在 `command_dispatcher_test.cpp` 末尾添加幂等性测试

  ```cpp
  TEST_CASE("registerBuiltinModules is idempotent on same factory") {
      PluginComponentFactory factory;
      registerBuiltinModules(factory);
      // 第二次调用不抛异常
      CHECK_NOTHROW(registerBuiltinModules(factory));
      // 模块数量不变
      CommandDispatcher dispatcher(factory);
      CHECK(dispatcher.listModules().size() == 2);
  }
  ```

- [ ] 步骤 2：运行测试确认新测试**失败**（当前 `bindSingleton` 重复调用会抛异常）
  ```
  cmake --build build --target opengeolab_command_test --config RelWithDebInfo --parallel 4
  ctest --test-dir build -C RelWithDebInfo -R command --output-on-failure
  ```

- [ ] 步骤 3：修改 `module_registry.cpp`，将 `registerBuiltinModules` 改为 check-then-register 模式

  **当前代码（第 19–27 行）：**
  ```cpp
  void registerBuiltinModules(PluginComponentFactory& factory) {
      LOG_INFO("Registering built-in modules...");
      factory.bindSingleton<Core::ModuleBase, IO::IOModule>(IO::IOModule::MODULE_NAME,
                                                            std::ref(factory));
      LOG_INFO("Registered module '{}'", IO::IOModule::MODULE_NAME);
      factory.bindSingleton<Core::ModuleBase, Geometry::GeometryModule>(
          Geometry::GeometryModule::MODULE_NAME, std::ref(factory));
      LOG_INFO("Registered module '{}'", Geometry::GeometryModule::MODULE_NAME);
  }
  ```

  **替换为：**
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

  **新增 include：** `<algorithm>` （用于 `std::ranges::any_of`）

- [ ] 步骤 4：运行测试确认新测试和所有旧测试**通过**
  ```
  cmake --build build --target opengeolab_command_test --config RelWithDebInfo --parallel 4
  ctest --test-dir build -C RelWithDebInfo -R command --output-on-failure
  ```

- [ ] 步骤 5：运行 clang-format
  ```
  clang-format -i src/libs/command/src/module_registry.cpp src/libs/command/test/command_dispatcher_test.cpp
  ```

---

### 任务 2：CommandDispatcher 新增 findModule() + 修正 Doxygen

**文件：**
- 修改：`src/libs/command/include/opengeolab/command/command_dispatcher.hpp`
- 修改：`src/libs/command/src/command_dispatcher.cpp`
- 测试：`src/libs/command/test/command_dispatcher_test.cpp`

**步骤：**

- [ ] 步骤 1：在 `command_dispatcher_test.cpp` 末尾添加 findModule 测试

  ```cpp
  TEST_CASE("CommandDispatcher findModule returns shared_ptr for registered module") {
      PluginComponentFactory factory;
      registerBuiltinModules(factory);

      CommandDispatcher dispatcher(factory);
      auto io_module = dispatcher.findModule("io");
      CHECK(io_module != nullptr);

      auto geo_module = dispatcher.findModule("geometry");
      CHECK(geo_module != nullptr);
  }

  TEST_CASE("CommandDispatcher findModule returns nullptr for unknown module") {
      PluginComponentFactory factory;
      CommandDispatcher dispatcher(factory);
      auto result = dispatcher.findModule("nonexistent");
      CHECK(result == nullptr);
  }
  ```

- [ ] 步骤 2：运行测试确认新测试**编译失败**（`findModule` 尚不存在）

- [ ] 步骤 3：在 `command_dispatcher.hpp` 的 `describe()` 方法后、`private:` 之前，添加 `findModule` 声明

  **在 `describe()` 声明之后添加：**
  ```cpp
  /**
   * @brief Look up a registered module by name.
   * @param module_name Module name to look up
   * @return Shared pointer to the module; nullptr if not registered
   */
  [[nodiscard]] std::shared_ptr<Core::ModuleBase> findModule(const std::string& module_name) const;
  ```

- [ ] 步骤 4：修正 `dispatch()` 的 Doxygen 注释，移除不正确的 `@throws` 标注

  **当前（第 57–66 行）：**
  ```cpp
  /**
   * @brief Dispatch a request to the module named in request["module"].
   * @param request JSON with "module", "action", "param" fields
   * @param progress Callback for reporting progress
   * @return Response JSON from the module
   * @throws std::invalid_argument if "module" field is missing
   * @throws Kangaroo::Util::ComponentFactoryNotRegisteredEx if module not found
   */
  ```

  **替换为：**
  ```cpp
  /**
   * @brief Dispatch a request to the module named in request["module"].
   * @param request JSON with "module", "action", "param" fields
   * @param progress Callback for reporting progress
   * @return Response JSON; on error returns {"ok": false, "summary": "...", "errors": [...]}
   */
  ```

- [ ] 步骤 5：在 `command_dispatcher.cpp` 的 `describe()` 实现之后添加 `findModule` 实现

  ```cpp
  std::shared_ptr<Core::ModuleBase>
  CommandDispatcher::findModule(const std::string& module_name) const {
      if (!hasModule(module_name)) {
          return nullptr;
      }
      return getModule(module_name);
  }
  ```

- [ ] 步骤 6：运行测试确认全部通过
  ```
  cmake --build build --target opengeolab_command_test --config RelWithDebInfo --parallel 4
  ctest --test-dir build -C RelWithDebInfo -R command --output-on-failure
  ```

- [ ] 步骤 7：运行 clang-format
  ```
  clang-format -i src/libs/command/include/opengeolab/command/command_dispatcher.hpp src/libs/command/src/command_dispatcher.cpp src/libs/command/test/command_dispatcher_test.cpp
  ```

---

### 任务 3：CMake 链接 app → Command

**文件：**
- 修改：`src/app/CMakeLists.txt`

**步骤：**

- [ ] 步骤 1：在 `src/app/CMakeLists.txt` 的 `target_link_libraries` 中，在 `OpenGeoLab::Python_Embed)` 之后添加 `OpenGeoLab::Command`

  **当前（第 17–29 行）：**
  ```cmake
  target_link_libraries(
      opengeolab_app
      PRIVATE Qt6::Concurrent
              ...
              OpenGeoLab::Python_Embed)
  ```

  **替换为：**
  ```cmake
  target_link_libraries(
      opengeolab_app
      PRIVATE Qt6::Concurrent
              ...
              OpenGeoLab::Python_Embed
              OpenGeoLab::Command)
  ```

- [ ] 步骤 2：验证构建通过（无需代码变更，仅确认链接关系正确）
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

---

### 任务 4：RequestService 双路径路由 + main.cpp 初始化

**文件：**
- 修改：`src/app/include/opengeolab/app/request_service.h`
- 修改：`src/app/src/request_service.cpp`
- 修改：`src/app/src/main.cpp`

本任务是核心变更，涉及三个文件联动修改。不适用 TDD（无自动化 UI 集成测试框架），通过构建 + 现有测试回归 + 手动端到端验证。

**步骤：**

- [ ] 步骤 1：修改 `request_service.h`

  **1a. 新增前向声明（在已有的 `OpenGeoLab::PythonEmbed` 前向声明附近）：**
  ```cpp
  namespace OpenGeoLab::Command {
  class CommandDispatcher;
  } // namespace OpenGeoLab::Command
  ```

  **1b. 修改构造函数签名（第 41 行）：**

  当前：
  ```cpp
  explicit RequestService(OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                          QObject* parent = nullptr);
  ```

  替换为：
  ```cpp
  explicit RequestService(OpenGeoLab::Command::CommandDispatcher& dispatcher,
                          OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                          QObject* parent = nullptr);
  ```

  **1c. 修改 PreparedRequest 结构体（第 86–90 行），新增 moduleName：**

  当前：
  ```cpp
  struct PreparedRequest {
      QString description;
      QString processJson;
      bool muted = false;
  };
  ```

  替换为：
  ```cpp
  struct PreparedRequest {
      QString description;
      QString processJson;
      std::string moduleName;
      bool muted = false;
  };
  ```

  **1d. 新增 `#include <string>` 如果尚未存在**

  **1e. 在 private 成员区域新增 `m_dispatcher` 引用（在 `m_runtime` 之前）：**

  当前：
  ```cpp
  OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& m_runtime;
  ```

  替换为：
  ```cpp
  OpenGeoLab::Command::CommandDispatcher& m_dispatcher;
  OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& m_runtime;
  ```

- [ ] 步骤 2：修改 `request_service.cpp`

  **2a. 新增 include（在已有 include 区域）：**
  ```cpp
  #include <opengeolab/command/command_dispatcher.hpp>
  #include <nlohmann/json.hpp>
  ```

  **2b. 修改构造函数（第 19–21 行）：**

  当前：
  ```cpp
  RequestService::RequestService(OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                                 QObject* parent)
      : QObject(parent), m_runtime(runtime) {}
  ```

  替换为：
  ```cpp
  RequestService::RequestService(OpenGeoLab::Command::CommandDispatcher& dispatcher,
                                 OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                                 QObject* parent)
      : QObject(parent), m_dispatcher(dispatcher), m_runtime(runtime) {}
  ```

  **2c. 修改 `submitAsync` 方法（第 30–92 行）的 worker lambda。** 关键变更：在 `prepareRequest` 的解构绑定中添加 `module_name`，并在 lambda 中根据 `hasModule` 分流。

  完整替换 `submitAsync` 方法体：
  ```cpp
  void RequestService::submitAsync(const QString& request_json) {
      auto [description, process_json, module_name, muted] = prepareRequest(request_json);
      if (!muted) {
          LOG_INFO("RequestService: submitting async [{}]", description.toStdString());
      }

      emit requestSent(description, request_json, muted);

      m_pendingCount.fetch_add(1, std::memory_order_relaxed);
      emit busyChanged();

      auto* watcher = new QFutureWatcher<QString>(this);

      Core::ProgressCallback progress_cb = [this](double progress,
                                                   const std::string& message) -> bool {
          QMetaObject::invokeMethod(
              this,
              [this, progress, msg = QString::fromStdString(message)]() {
                  emit progressUpdated(progress, msg);
              },
              Qt::QueuedConnection);
          return true;
      };

      const bool use_cpp_path = m_dispatcher.hasModule(module_name);

      auto future = QtConcurrent::run(
          [this, json = process_json.toStdString(), cb = std::move(progress_cb),
           muted, desc = description.toStdString(), use_cpp_path]() -> QString {
              std::optional<Kangaroo::Util::Stopwatch> sw;
              if (!muted) {
                  sw.emplace(desc, Core::getLoggerShared());
              }
              if (use_cpp_path) {
                  nlohmann::json request;
                  try {
                      request = nlohmann::json::parse(json);
                  } catch (const nlohmann::json::parse_error& e) {
                      nlohmann::json err = {
                          {"ok", false},
                          {"summary", "Invalid JSON in request"},
                          {"errors", nlohmann::json::array({std::string(e.what())})}};
                      return QString::fromStdString(err.dump());
                  }
                  auto result = m_dispatcher.dispatch(request, cb);
                  return QString::fromStdString(result.dump());
              }
              return QString::fromStdString(m_runtime.process(json, cb));
          });

      {
          const std::lock_guard lock(m_futuresMutex);
          m_pendingFutures.push_back(future);
      }

      connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, muted]() {
          m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
          emit busyChanged();

          try {
              const QString response = watcher->result();
              emitResponse(response, muted);
          } catch (const std::exception& exception) {
              LOG_ERROR("RequestService: async request threw exception: {}{}", exception.what(),
                        muted ? " (muted)" : "");
              emit errorOccurred(QString::fromStdString(exception.what()), muted);
          }

          {
              const std::lock_guard lock(m_futuresMutex);
              std::erase_if(m_pendingFutures,
                            [](const QFuture<QString>& f) { return f.isFinished(); });
          }

          watcher->deleteLater();
      });

      watcher->setFuture(future);
  }
  ```

  **2d. 修改 `executeOnMainThread` 方法（第 94–119 行）：**

  完整替换方法体：
  ```cpp
  void RequestService::executeOnMainThread(const QString& request_json) {
      auto [description, process_json, module_name, muted] = prepareRequest(request_json);

      if (!muted) {
          LOG_INFO("RequestService: executing on main thread [{}]", description.toStdString());
      }

      emit requestSent(description, request_json, muted);

      try {
          std::optional<Kangaroo::Util::Stopwatch> sw;
          if (!muted) {
              sw.emplace(description.toStdString(), Core::getLoggerShared());
          }

          if (m_dispatcher.hasModule(module_name)) {
              auto request = nlohmann::json::parse(process_json.toStdString());
              auto result = m_dispatcher.dispatch(request, nullptr);
              emitResponse(QString::fromStdString(result.dump()), muted);
          } else {
              const auto response =
                  QString::fromStdString(m_runtime.process(process_json.toStdString(), nullptr));
              emitResponse(response, muted);
          }
      } catch (const std::exception& exception) {
          LOG_ERROR("RequestService: main-thread request threw exception: {}{}", exception.what(),
                    muted ? " (muted)" : "");
          emit errorOccurred(QString::fromStdString(exception.what()), muted);
      }
  }
  ```

  **2e. 修改 `prepareRequest` 方法（第 123–136 行），返回 moduleName：**

  当前：
  ```cpp
  return {
      QStringLiteral("%1.%2").arg(module, action),
      json,
      muted,
  };
  ```

  替换为：
  ```cpp
  return {
      QStringLiteral("%1.%2").arg(module, action),
      json,
      module.toStdString(),
      muted,
  };
  ```

- [ ] 步骤 3：修改 `main.cpp`

  **3a. 新增 include（在已有 include 区域之后）：**
  ```cpp
  #include <opengeolab/command/command_dispatcher.hpp>
  #include <opengeolab/command/module_registry.hpp>
  #include <kangaroo/util/plugin_component_factory.hpp>
  ```

  **3b. 在 Python runtime 创建之前，添加模块注册和 dispatcher 创建（在第 47 行之后、第 48 行之前）：**

  在 `plugin_dir` 定义之后、`python_runtime` 创建之前插入：
  ```cpp
  OpenGeoLab::Command::registerBuiltinModules(g_PluginComponentFactory);
  OpenGeoLab::Command::CommandDispatcher dispatcher(g_PluginComponentFactory);
  ```

  **3c. 修改 RequestService 构造（第 50 行）：**

  当前：
  ```cpp
  OpenGeoLab::App::RequestService request_service(python_runtime);
  ```

  替换为：
  ```cpp
  OpenGeoLab::App::RequestService request_service(dispatcher, python_runtime);
  ```

- [ ] 步骤 4：运行构建
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 5：运行全部测试确认回归通过
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```

- [ ] 步骤 6：运行 clang-format
  ```
  clang-format -i src/app/include/opengeolab/app/request_service.h src/app/src/request_service.cpp src/app/src/main.cpp
  ```

---

### 任务 5：最终验证 + 提交

- [ ] 步骤 1：完整构建
  ```
  cmake --build build --config RelWithDebInfo --parallel 4
  ```

- [ ] 步骤 2：完整测试
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```

- [ ] 步骤 3：确认 8+ 测试全部通过（原有 8 个 + 新增 3 个 = 11 个）

- [ ] 步骤 4：检查 git diff，确认变更范围与规格一致

- [ ] 步骤 5：提交（需用户确认）

  建议 commit message：
  ```
  refactor(command): enable direct C++ dispatch from app layer

  RequestService now routes C++ module requests (geometry, io) directly
  through CommandDispatcher without going through Python/GIL. Python path
  is preserved for plugins and system modules.

  Key changes:
  - registerBuiltinModules uses check-then-register for idempotency
  - CommandDispatcher exposes findModule() public API
  - RequestService accepts both CommandDispatcher and EmbeddedPythonRuntime
  - main.cpp registers modules and creates dispatcher before Python init
  - App links to OpenGeoLab::Command library
  ```
