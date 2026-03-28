# Action 工厂化重构设计

## 目标

将 Action 的注册、查询和分发从模块私有 `unordered_map` 迁移到全局 `PluginComponentFactory`，与 Module 的注册模式统一。消除各模块中重复的 `registerAction()` / `process()` / `describe()` 代码。

## 约束

- 协议中 `"module"` 与 `"action"` 字段仍然分开，不合并
- `process()` 和 `describe()` 签名改为 `const` 成员函数（语义更准确，分发不修改模块状态）
- 工厂中 Action 的注册 key 使用 `"module.action"` 格式（如 `"geometry.create_box"`）
- Action 注册为 Singleton（与 Module 一致）
- `process()` 和 `describe()` 保持 `virtual`，`ModuleBase` 提供默认实现，子类可重写

## 变更范围

### 1. `IAction` (action.hpp)

在文件末尾添加 `PluginComponentInterfaceId` 特化：

```cpp
template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Core::IAction> {
    static constexpr std::string_view VALUE{"opengeolab.core.IAction"};
};
```

需要在 action.hpp 中 `#include <kangaroo/util/plugin_component_factory.hpp>`。

### 2. `ModuleBase` (module.hpp / module.cpp)

从纯抽象接口变为带状态和默认实现的基类。

#### 头文件

```cpp
class OPENGEOLAB_CORE_EXPORT ModuleBase : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit ModuleBase(std::string_view module_name,
                        std::string_view description,
                        Kangaroo::Util::PluginComponentFactory& factory);
    virtual ~ModuleBase();

    /// 模块名（与请求中 "module" 字段对应）
    [[nodiscard]] std::string_view moduleName() const;

    /// 返回模块描述 JSON，默认从工厂枚举本模块的 Action
    [[nodiscard]] virtual nlohmann::json describe() const;

    /// 分发请求到 Action，默认从工厂查找 "module.action" 对应的 IAction
    [[nodiscard]] virtual nlohmann::json process(const nlohmann::json& request,
                                                  const ProgressCallback& progress) const;

protected:
    /// 注册 ActionT 到工厂，key = "moduleName.ActionT::ACTION_NAME"
    /// ActionT 必须提供 static constexpr std::string_view ACTION_NAME
    template <class ActionT>
    void registerAction();

    /// 子类如需自定义分发可访问工厂
    [[nodiscard]] Kangaroo::Util::PluginComponentFactory& factory() const;

private:
    std::string m_moduleName;
    std::string m_description;
    Kangaroo::Util::PluginComponentFactory& m_factory;
};
```

#### registerAction 模板实现（头文件内联）

```cpp
template <class ActionT>
void ModuleBase::registerAction() {
    std::string key = m_moduleName + "." + std::string(ActionT::ACTION_NAME);
    m_factory.bindSingleton<IAction, ActionT>(key);
}
```

#### module.cpp 默认实现

```cpp
ModuleBase::ModuleBase(std::string_view module_name,
                       std::string_view description,
                       Kangaroo::Util::PluginComponentFactory& factory)
    : m_moduleName(module_name), m_description(description), m_factory(factory) {}

ModuleBase::~ModuleBase() = default;

std::string_view ModuleBase::moduleName() const { return m_moduleName; }

Kangaroo::Util::PluginComponentFactory& ModuleBase::factory() const { return m_factory; }

nlohmann::json ModuleBase::describe() const {
    const std::string prefix = m_moduleName + ".";
    auto all = m_factory.listFactories<IAction>();

    nlohmann::json actions = nlohmann::json::array();
    for (const auto& info : all) {
        if (info.m_moduleName.starts_with(prefix)) {
            auto action = m_factory.getSharedInstance<IAction>(info.m_moduleName);
            actions.push_back(action->describe());
        }
    }

    return {{"name", m_moduleName},
            {"description", m_description},
            {"actions", std::move(actions)}};
}

nlohmann::json ModuleBase::process(const nlohmann::json& request,
                                    const ProgressCallback& progress) const {
    if (!request.contains("action") || !request["action"].is_string()) {
        throw std::invalid_argument(
            fmt::format("{} request must contain a string \"action\" field", m_moduleName));
    }

    const auto action_name = request["action"].get<std::string>();
    const std::string key = m_moduleName + "." + action_name;

    std::shared_ptr<IAction> action;
    try {
        action = m_factory.getSharedInstance<IAction>(key);
    } catch (const Kangaroo::Util::ComponentFactoryNotRegisteredEx&) {
        throw std::invalid_argument(
            fmt::format("{} module: unknown action '{}'", m_moduleName, action_name));
    }

    const auto param = request.contains("param") ? request["param"] : nlohmann::json::object();
    LOG_INFO("{}::process: executing action '{}'", m_moduleName, action_name);
    return action->execute(param, progress);
}
```

### 3. GeometryModule

#### 头文件

```cpp
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule final : public Core::ModuleBase {
public:
    explicit GeometryModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~GeometryModule() override;

    static constexpr std::string_view MODULE_NAME{"geometry"};
};
```

删除：`registerAction(unique_ptr)` 方法、`m_actions` 成员、`describe()`/`process()` 重写。

#### 实现

```cpp
GeometryModule::GeometryModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "Geometry creation and manipulation module.", factory) {
    registerAction<CreateBoxAction>();
}
GeometryModule::~GeometryModule() = default;
```

### 4. IOModule

#### 头文件

```cpp
class OPENGEOLAB_IO_EXPORT IOModule final : public Core::ModuleBase {
public:
    explicit IOModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~IOModule() override;

    static constexpr std::string_view MODULE_NAME{"io"};
};
```

删除：`registerAction(unique_ptr)` 方法、`m_actions` 成员、`describe()`/`process()` 重写、`registerModule()` 静态方法。

#### 实现

```cpp
IOModule::IOModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "I/O module for reading and writing geometry files.", factory) {
    registerAction<ReadBrepAction>();
}
IOModule::~IOModule() = default;
```

### 5. registerBuiltinModules (module_registry.cpp)

```cpp
void registerBuiltinModules(Kangaroo::Util::PluginComponentFactory& factory) {
    factory.bindSingleton<Core::ModuleBase, IO::IOModule>(
        IO::IOModule::MODULE_NAME, std::ref(factory));
    factory.bindSingleton<Core::ModuleBase, Geometry::GeometryModule>(
        Geometry::GeometryModule::MODULE_NAME, std::ref(factory));
}
```

`bindSingleton` 内部会将额外参数拷贝到 `std::tuple` 中。由于 `PluginComponentFactory`
继承自 `NonCopyMoveable`，必须使用 `std::ref(factory)` 传递引用包装，
避免触发 deleted copy constructor。

### 6. 测试更新

所有直接构造模块的测试需要改为传入 `PluginComponentFactory` 实例：

```cpp
// Before:
GeometryModule mod;

// After:
Kangaroo::Util::PluginComponentFactory factory;
Geometry::GeometryModule mod(factory);
```

行为断言保持不变。

### 7. 不变的部分

- `CommandDispatcher` — 接口和实现无变化
- 各 Action 类（`CreateBoxAction`、`ReadBrepAction`）— 无变化
- 请求/响应 JSON 协议 — 无变化
- Python wrapper — 无变化（仍通过 `registerBuiltinModules` 初始化）
