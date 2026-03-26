---
description: 'OpenGeoLab Command / Module / Action 框架开发规范'
applyTo: '**/libs/{core,io,command,python}/**/*.{h,hpp,cpp,cxx}'
---

# OpenGeoLab Command Framework Instructions

本文档定义了 Command / Module / Action 框架的设计约定，所有新模块和 Action 必须遵循。

## 1. Request / Response 协议

### 1.1 Request 格式

所有请求必须为 JSON 对象，包含以下字段：

```json
{
  "module": "<module_name>",
  "action": "<action_name>",
  "param":  { ... }
}
```

| 字段     | 类型   | 必需 | 说明                      |
|----------|--------|------|---------------------------|
| `module` | string | 是   | 目标模块名（如 `"io"`）    |
| `action` | string | 是   | 模块内的 action 名（如 `"read_brep"`） |
| `param`  | object | 否   | action 所需参数，缺省为 `{}` |

### 1.2 Response 格式

Response 为 JSON 对象，由 Action 自行定义。必须包含 `"status"` 字段表示成功/失败：

```json
{ "status": "ok", ... }
{ "status": "error", "message": "..." }
```

### 1.3 错误处理

- `module` 字段缺失或非字符串 → `std::invalid_argument`
- `action` 字段缺失或非字符串 → `std::invalid_argument`
- 未知 module → `ComponentFactoryNotRegisteredEx`
- 未知 action → `std::invalid_argument`
- Action 内部错误 → 由 Action 自行决定抛异常或返回 error 状态

---

## 2. ModuleBase 子类规范

### 2.1 继承与注册

- 继承 `OpenGeoLab::Core::ModuleBase`（不可拷贝/移动，继承自 `NonCopyMoveable`）
- 通过 `PluginComponentFactory::bindSingleton<ModuleBase, YourModule>(MODULE_NAME)` 注册
- 声明 `static constexpr std::string_view MODULE_NAME{"your_module"}`
- 模块名全小写、用下划线分隔

### 2.2 describe() 返回格式

```json
{
  "name": "io",
  "description": "I/O module for reading and writing geometry files.",
  "actions": [ <action.describe()>, ... ]
}
```

| 字段          | 类型   | 必需 | 说明 |
|---------------|--------|------|------|
| `name`        | string | 是   | 必须与 `MODULE_NAME` 一致 |
| `description` | string | 是   | 人类 / LLM 可读的模块功能描述 |
| `actions`     | array  | 是   | 所有注册 action 的 `describe()` 输出 |

### 2.3 process() 实现约定

- 从 `request["action"]` 提取 action 名
- 在内部 action map 中查找并委派
- `param` 缺失时使用 `nlohmann::json::object()` 默认值（值拷贝，不要 const ref 临时对象）
- 将完整 `request` 传入前先提取 `param`，只把 `param` 传给 `IAction::execute()`

### 2.4 Action 注册

- 在构造函数中调用 `registerAction(std::make_unique<YourAction>())`
- `registerAction()` 通过 `describe()["name"]` 获取 action 名作为 key
- `registerAction()` 必须校验：非空指针、describe() 包含 string `"name"`、名字非空、无重复

---

## 3. IAction 子类规范

### 3.1 继承

- 继承 `OpenGeoLab::Core::IAction`
- 声明 `static constexpr std::string_view ACTION_NAME{"your_action"}`
- Action 名全小写、用下划线分隔

### 3.2 describe() 返回格式

```json
{
  "name": "read_brep",
  "description": "Read a BRep geometry file from the given path.",
  "params": {
    "path": {
      "type": "string",
      "required": true,
      "description": "File path to the BRep file"
    }
  },
  "returns": {
    "status": { "type": "string", "description": "Result status: ok / error" },
    "action": { "type": "string", "description": "Echo of the action name" }
  }
}
```

| 字段          | 类型   | 必需 | 说明 |
|---------------|--------|------|------|
| `name`        | string | 是   | 必须与 `ACTION_NAME` 一致 |
| `description` | string | 是   | 人类 / LLM 可读的 action 功能描述 |
| `params`      | object | 是   | 参数 schema，每个 key 对应一个参数 |
| `returns`     | object | 是   | 返回值 schema |

参数/返回值 schema 的每个字段至少包含 `"type"` 和 `"description"`。参数还应包含 `"required"` 布尔值。

### 3.3 execute() 实现约定

- 接收的 `param` 是 `request["param"]`（或默认空 object）
- 必须校验必需参数存在且类型正确
- 返回 JSON 对象，其结构应与 `describe()["returns"]` 一致

---

## 4. CommandDispatcher 约定

- 持有 `PluginComponentFactory&` 引用，不拥有
- `dispatch()` 提取 `module` 字段 → `factory.getSharedInstance<ModuleBase>(name)` → `module->process()`
- `describe()` 返回完整系统描述（`request_schema` + 所有模块的 `describe()` 汇总），供 LLM / UI 自动发现

---

## 5. 模块注册流程

### 5.1 内建模块注册

- `module_registry.cpp` 中的 `registerBuiltinModules(factory)` 集中注册所有内建模块
- 新增模块时，在此函数中追加 `factory.bindSingleton<ModuleBase, YourModule>(YourModule::MODULE_NAME)`
- 不要在多处分散注册

### 5.2 PluginComponentFactory 接口 ID

- `ModuleBase` 的 interface ID 为 `"opengeolab.core.ModuleBase"`
- 通过 `PluginComponentInterfaceId<ModuleBase>` trait 特化定义
- 新增 interface 类型时，必须在对应头文件中添加 trait 特化

---

## 6. Python Wrapper 约定

- Python 端通过 `opengeolab_python_wrapper` 模块暴露 C++ 功能
- `process(request_json, progress_callback=None)` — 转发 JSON 请求到 CommandDispatcher
- `describe()` — 返回完整系统描述 JSON 字符串
- `has_module(name)` / `list_modules()` — 查询注册模块
- Dispatcher 使用 `std::call_once` 确保线程安全的单次初始化
- Python progress callback 通过 `gil_scoped_acquire` 获取 GIL，并用 try/catch 处理 `py::error_already_set`

---

## 7. DLL 边界注意事项

- 所有跨 DLL 的基类必须使用 `OPENGEOLAB_*_EXPORT` 宏
- 虚析构函数和构造函数必须在 `.cpp` 中 out-of-line 定义（不能 inline `= default`）
- `generate_export_header` 自动生成 `*_export.hpp`

---

## 8. 测试约定

- 每个模块必须有对应的 doctest 测试文件
- 对 `[[nodiscard]]` 函数在 `CHECK_THROWS_AS` 中使用 `(void)` 强制转换避免警告
- 测试应覆盖：正常路径、缺失字段、未知 action/module、describe() 结构完整性

---

## 9. 新增 Module 步骤清单

1. 在 `src/libs/<name>/` 下创建目录结构
2. 编写 `CMakeLists.txt`，调用 `opengeolab_add_module()`，链接 `OpenGeoLab::Core`
3. 创建 `YourModule : public ModuleBase`，实现 `describe()` / `process()`
4. 创建 `YourAction : public IAction`，实现 `describe()` / `execute()`
5. 在 `module_registry.cpp` 中注册
6. 在顶层 `CMakeLists.txt` 中 `add_subdirectory()`（放在 core 之后、python 之前）
7. 编写测试，运行 `ctest --test-dir build --output-on-failure`
8. 运行 `clang-format` + `clang-tidy` 验证
