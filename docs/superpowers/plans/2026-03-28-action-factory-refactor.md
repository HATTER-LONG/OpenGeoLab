# Action 工厂化重构 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 将 Action 注册从模块私有 map 迁移到全局 PluginComponentFactory，消除模块间重复代码

**架构：** IAction 获得 PluginComponentInterfaceId 特化，ModuleBase 从纯接口升级为带状态基类（持有 moduleName、description、factory 引用），提供 registerAction\<T\>() 模板和 process()/describe() 默认实现。各具体模块简化为仅在构造函数中调用 registerAction\<T\>()。

**技术栈：** C++20, Kangaroo PluginComponentFactory, nlohmann::json, doctest, CMake/Ninja

**规格文档：** `docs/superpowers/specs/2026-03-28-action-factory-design.md`

**验证命令：**
- 构建：`cmake --build build --config Debug --parallel 4`
- 测试：`ctest --test-dir build -C Debug --output-on-failure`

---

### 任务 0：基线验证

**目的：** 确认当前代码可以正常构建和通过所有测试

- [ ] 步骤 1：执行构建 `cmake --build build --config Debug --parallel 4`
- [ ] 步骤 2：执行测试 `ctest --test-dir build -C Debug --output-on-failure`
- [ ] 步骤 3：确认全部通过，记录测试数量作为基线

---

### 任务 1：IAction 添加 PluginComponentInterfaceId 特化

**文件：**
- 修改：`src/libs/core/include/opengeolab/core/action.hpp`

**步骤：**

- [ ] 步骤 1：在 action.hpp 中添加 `#include <kangaroo/util/plugin_component_factory.hpp>`
- [ ] 步骤 2：在文件末尾（namespace 关闭之后）添加特化：
```cpp
template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Core::IAction> {
    static constexpr std::string_view VALUE{"opengeolab.core.IAction"};
};
```
- [ ] 步骤 3：执行构建验证，确认编译通过
- [ ] 步骤 4：执行测试验证，确认无回归

---

### 任务 2：重构 ModuleBase 基类

**文件：**
- 修改：`src/libs/core/include/opengeolab/core/module.hpp`
- 修改：`src/libs/core/src/module.cpp`

**步骤：**

- [ ] 步骤 1：修改 `module.hpp`，将 ModuleBase 从纯抽象改为带状态基类：
  - 添加 `#include <opengeolab/core/action.hpp>` 和 `#include <fmt/format.h>`（用于 process 默认实现）
  - 构造函数改为 `explicit ModuleBase(std::string_view module_name, std::string_view description, Kangaroo::Util::PluginComponentFactory& factory)`
  - 添加 `[[nodiscard]] std::string_view moduleName() const;`
  - `describe()` 和 `process()` 从纯虚变为 virtual 带默认实现，`process()` 签名添加 `const`
  - 添加 protected `template <class ActionT> void registerAction();`
  - 添加 protected `[[nodiscard]] Kangaroo::Util::PluginComponentFactory& factory() const;`
  - 添加 private 成员：`std::string m_moduleName`, `std::string m_description`, `PluginComponentFactory& m_factory`
  - 在 hpp 内联实现 `registerAction<ActionT>()` 模板：
    ```cpp
    template <class ActionT>
    void ModuleBase::registerAction() {
        std::string key = m_moduleName + "." + std::string(ActionT::ACTION_NAME);
        m_factory.bindSingleton<IAction, ActionT>(key);
    }
    ```

- [ ] 步骤 2：修改 `module.cpp`，添加默认实现：
  - 构造函数：初始化 m_moduleName, m_description, m_factory
  - `moduleName()`：返回 m_moduleName
  - `factory()`：返回 m_factory
  - `describe()`：遍历工厂中所有 IAction 注册项，过滤 `m_moduleName + "."` 前缀，收集 describe()
  - `process()`：提取 action 字段 → 拼 key → try getSharedInstance → catch ComponentFactoryNotRegisteredEx → execute
  - 需要 `#include <opengeolab/core/logger.hpp>`、`#include <fmt/format.h>`、`#include <kangaroo/util/plugin_component_factory.hpp>`

- [ ] 步骤 3：执行构建（此时 GeometryModule/IOModule 会编译失败，这是预期的，因为它们还在用旧构造函数）

**注意：** 此任务完成后构建暂时不通过，需要任务 3、4、5 一起完成才能恢复。建议在单次会话中连续完成任务 2–5 后再验证构建。

---

### 任务 3：简化 GeometryModule

**文件：**
- 修改：`src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp`
- 修改：`src/libs/geometry/src/geometry_module.cpp`

**步骤：**

- [ ] 步骤 1：修改 `geometry_module.hpp`：
  - 添加 `namespace Kangaroo::Util { class PluginComponentFactory; }` 前置声明（或 include）
  - 构造函数改为 `explicit GeometryModule(Kangaroo::Util::PluginComponentFactory& factory);`
  - 删除 `registerAction(std::unique_ptr<Core::IAction>)` 方法
  - 删除 `describe()` 和 `process()` 声明（使用基类默认实现）
  - 删除 `m_actions` 成员
  - 保留 `MODULE_NAME` 和析构函数

- [ ] 步骤 2：修改 `geometry_module.cpp`：
  - 构造函数改为调用 `ModuleBase(MODULE_NAME, "Geometry creation and manipulation module.", factory)` 并在体内调用 `registerAction<CreateBoxAction>()`
  - 删除 `describe()`、`process()`、`registerAction()` 实现
  - 清理不再需要的 includes

---

### 任务 4：简化 IOModule

**文件：**
- 修改：`src/libs/io/include/opengeolab/io/io_module.hpp`
- 修改：`src/libs/io/src/io_module.cpp`

**步骤：**

- [ ] 步骤 1：修改 `io_module.hpp`：
  - 添加 `namespace Kangaroo::Util { class PluginComponentFactory; }` 前置声明
  - 构造函数改为 `explicit IOModule(Kangaroo::Util::PluginComponentFactory& factory);`
  - 删除 `registerAction(std::unique_ptr<Core::IAction>)` 方法
  - 删除 `describe()` 和 `process()` 声明
  - 删除 `registerModule()` 静态方法
  - 删除 `m_actions` 成员
  - 保留 `MODULE_NAME` 和析构函数

- [ ] 步骤 2：修改 `io_module.cpp`：
  - 构造函数改为调用 `ModuleBase(MODULE_NAME, "I/O module for reading and writing geometry files.", factory)` 并在体内调用 `registerAction<ReadBrepAction>()`
  - 删除 `describe()`、`process()`、`registerAction()`、`registerModule()` 实现
  - 清理不再需要的 includes

---

### 任务 5：更新 registerBuiltinModules

**文件：**
- 修改：`src/libs/command/src/module_registry.cpp`

**步骤：**

- [ ] 步骤 1：在 `registerBuiltinModules` 中给 `bindSingleton` 添加 `std::ref(factory)` 参数：
  ```cpp
  factory.bindSingleton<Core::ModuleBase, IO::IOModule>(
      IO::IOModule::MODULE_NAME, std::ref(factory));
  factory.bindSingleton<Core::ModuleBase, Geometry::GeometryModule>(
      Geometry::GeometryModule::MODULE_NAME, std::ref(factory));
  ```
- [ ] 步骤 2：添加 `#include <functional>` 以支持 `std::ref`
- [ ] 步骤 3：执行构建验证（此时主代码应全部编译通过）

---

### 任务 6：更新测试

**文件：**
- 修改：`src/libs/geometry/test/geometry_module_test.cpp`
- 修改：`src/libs/io/test/io_module_test.cpp`
- 修改：`src/libs/command/test/command_dispatcher_test.cpp`（可能无需修改，需确认）

**步骤：**

- [ ] 步骤 1：修改 `geometry_module_test.cpp`：
  - 添加 `#include <kangaroo/util/plugin_component_factory.hpp>`
  - 所有构造 `GeometryModule mod;` 改为 `Kangaroo::Util::PluginComponentFactory factory; GeometryModule mod(factory);`
  - 保持所有断言不变

- [ ] 步骤 2：修改 `io_module_test.cpp`：
  - 添加 `#include <kangaroo/util/plugin_component_factory.hpp>`
  - 所有构造 `IOModule mod;` 改为 `Kangaroo::Util::PluginComponentFactory factory; IO::IOModule mod(factory);`
  - 保持所有断言不变

- [ ] 步骤 3：检查 `command_dispatcher_test.cpp`：
  - 该文件已经通过 `registerBuiltinModules(factory)` 使用，理论上不需要修改
  - 确认编译通过

- [ ] 步骤 4：执行全量构建 `cmake --build build --config Debug --parallel 4`
- [ ] 步骤 5：执行全量测试 `ctest --test-dir build -C Debug --output-on-failure`
- [ ] 步骤 6：确认所有测试通过，数量与基线一致

---

### 任务 7：格式化和提交

- [ ] 步骤 1：对所有修改的文件运行 clang-format
- [ ] 步骤 2：`git add` 所有修改文件
- [ ] 步骤 3：`git diff --cached --stat` 确认变更范围
- [ ] 步骤 4：**询问用户确认后**提交，commit message：
  ```
  refactor(core): register actions via PluginComponentFactory

  Move action registration from per-module private maps to the global
  PluginComponentFactory with "module.action" keys. ModuleBase now holds
  module name, description and factory reference, providing default
  process()/describe() implementations that delegate to factory-managed
  IAction singletons. Concrete modules only call registerAction<T>() in
  their constructors.

  Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
  ```
