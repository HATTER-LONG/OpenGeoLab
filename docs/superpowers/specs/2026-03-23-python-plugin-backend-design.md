# Python Plugin Backend — 设计规格

## 1. 目标

为 OpenGeoLabNew 引入嵌入式 Python 运行时和插件系统：

1. **QML → C++ → Python 异步请求通道**：不阻塞 UI
2. **插件发现与执行**：Ribbon 新增 Plugins Tab，自动列出插件
3. **PySide6 非模态 UI 支持**：插件可弹出独立窗口
4. **轻量级 pybind11 wrapper**：预留 C++ 模块扩展接口

## 2. JSON 协议格式

所有请求/响应通过统一 JSON envelope 传递。

### 请求

```json
{
  "module": "plugins",
  "action": "list",
  "param": {}
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `module` | `string` | 目标模块 |
| `action` | `string` | 模块内动作 |
| `param` | `object` | 动作参数 |

> **注意**：参考项目（OpenGeoLabBack）使用 `payload` 字段名，本项目有意改为 `param` 以更简洁。

### 路由规则

ProcessService 和 opengeolab_runtime.py **统一使用 `module` + `action` 两个独立字段** 做路由判断，不拼接点号：

- C++ ProcessService: `module == "plugins" && action == "invoke_ui"` → 主线程
- Python runtime: `module == "plugins" and action == "list"` → 插件列表
- QML 发请求时始终填写两个独立字段

### 响应

```json
{
  "protocolVersion": "1.0",
  "ok": true,
  "module": "plugins",
  "action": "list",
  "summary": "Plugins enumerated.",
  "result": {},
  "errors": []
}
```

## 3. 架构总览

```
┌──────────────────────────────────────────────────────────┐
│                        QML UI Layer                      │
│  Main.qml → AppHeader → Plugins Tab (动态 RibbonTile)   │
└──────────────┬───────────────────────────────────────────┘
               │ submitRequest(json)
               ↓
┌──────────────────────────────────────────────────────────┐
│              ProcessService  (C++ / Qt)                  │
│  • plugins.invoke_ui → 主线程同步执行 (PySide6 需要)     │
│  • 其它 action → QtConcurrent::run() 异步执行            │
│  • 信号: responseReady / errorOccurred / busyChanged     │
└──────────────┬───────────────────────────────────────────┘
               │ m_runtime.process(json)
               ↓
┌──────────────────────────────────────────────────────────┐
│        EmbeddedPythonRuntime  (pybind11 / C++)           │
│  • 初始化 Python 解释器 + 配置搜索路径                    │
│  • 调用 opengeolab_runtime.process()                     │
│  • GIL 管理 (主线程释放, 工作线程获取)                    │
└──────────────┬───────────────────────────────────────────┘
               ↓
┌──────────────────────────────────────────────────────────┐
│          opengeolab_runtime.py  (Python)                 │
│  • plugins.list → 扫描插件目录, 调用 describe_plugin()   │
│  • plugins.invoke_ui → import + launch_ui()              │
│  • plugins.execute → import + execute()                  │
│  • capabilities.query → 运行时信息                        │
│  • 其它 → opengeolab_pywrapper.process() (未来扩展)      │
└──────────────┬───────────────────────────────────────────┘
               ↓
┌──────────────────────────────────────────────────────────┐
│      opengeolab_pywrapper  (pybind11 模块)               │
│  • process(json) → 占位, 返回 "未实现" 响应               │
│  • protocol_version() → "1.0"                            │
│  • (预留: 未来接入 C++ ModuleDispatcher)                  │
└──────────────────────────────────────────────────────────┘
```

## 4. 新增文件清单

### 4.1 C++ Libraries

```
src/libs/python/
├── CMakeLists.txt                          # 父 CMakeLists, add_subdirectory 两个子模块
├── python_embedded/
│   ├── CMakeLists.txt
│   ├── include/opengeolab/python/
│   │   └── embedded_python_runtime.hpp
│   └── src/
│       └── embedded_python_runtime.cpp
└── python_wrapper/
    ├── CMakeLists.txt
    └── src/
        └── python_wrapper_module.cpp
```

### 4.2 App 层变更

```
src/app/
├── include/opengeolab/app/
│   └── process_service.hpp                 # 新增
├── src/
│   ├── main.cpp                            # 修改: 初始化 Python, 注册 ProcessService
│   └── process_service.cpp                 # 新增
└── resource/
    ├── python/
    │   └── opengeolab_runtime.py           # 新增
    └── icons/
        ├── pluginA.svg ~ pluginJ.svg       # 10 个通用插件图标 (ionicons-outline)
```

### 4.3 QML 变更

```
src/app/resource/qml/
├── RibbonConfig.qml                        # 修改: 新增第 4 个 "Plugins" Tab
├── Main.qml                                # 修改: 连接 processService 信号, 传递 pluginGroups
├── sections/
│   └── AppHeader.qml                       # 修改: 支持动态 plugin ribbon groups
└── components/
    └── PluginRibbonGroup.qml               # 新增: 专用于动态渲染插件按钮
```

### 4.4 插件目录

```
plugins/
├── hello_plugin.py                         # 示例: 纯功能插件
└── demo_ui_plugin/
    └── __init__.py                         # 示例: PySide6 非模态 UI 插件
```

### 4.5 CMake 变更

| 文件 | 变更 |
|------|------|
| `CMakeLists.txt` (顶层) | 添加 `find_package(Python3)`, `find_package(pybind11)`, `include(SetupPySideVenv)`, `add_subdirectory(src/libs/python)` |
| `src/libs/python/CMakeLists.txt` | 新增父 CMakeLists |
| `src/libs/python/python_embedded/CMakeLists.txt` | `opengeolab_add_module` 构建 embed 库 |
| `src/libs/python/python_wrapper/CMakeLists.txt` | `pybind11_add_module` 构建 pyd |
| `src/app/CMakeLists.txt` | 链接 `OpenGeoLab::Python`, `Qt6::Concurrent`, 添加 `process_service.cpp`, post-build 拷贝 `python/` 和 `plugins/` |

## 5. 关键组件详细设计

### 5.1 EmbeddedPythonRuntime

与参考项目完全一致。核心流程：

1. 解析 Python Home（环境变量 > 编译时默认值）
2. 构建模块搜索路径（stdlib, DLLs, Lib, site-packages, app root, runtime root, plugin root, pyvenv site-packages）
3. 通过 `PyConfig` 配置初始化 `pybind11::scoped_interpreter`
4. 设置 `os.environ` 环境变量
5. 加载 `opengeolab_runtime.process` 函数引用
6. `process()` 方法在调用时获取 GIL

### 5.2 ProcessService

```cpp
class ProcessService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit ProcessService(EmbeddedPythonRuntime& runtime, QObject* parent = nullptr);
    Q_INVOKABLE void submitRequest(const QString& requestJson);
    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& responseJson);
    void errorOccurred(const QString& errorMessage);
    void busyChanged();
};
```

**路由逻辑**（基于独立 `module` + `action` 字段）：
- `module == "plugins" && action == "invoke_ui"` → 主线程同步（PySide6 widget 必须在 GUI 线程创建）
- 其它 → `QtConcurrent::run()` 异步

### 5.3 opengeolab_runtime.py

支持的 action：

| module | action | 说明 |
|--------|--------|------|
| `plugins` | `list` | 扫描插件目录，返回 `describe_plugin()` 结果 |
| `plugins` | `invoke_ui` | 调用插件 `launch_ui()`，需要 `param.plugin` |
| `plugins` | `execute` | 调用插件 `execute(param)`，需要 `param.plugin` |
| `system` | `capabilities` | 返回运行时能力信息 |
| 其它 | * | 委托给 `opengeolab_pywrapper.process()` |

路由分发使用 `module` + `action` 独立字段，不拼接点号。

### 5.4 插件约定

每个插件必须提供 `describe_plugin()` 函数：

```python
def describe_plugin() -> dict:
    return {
        "name": "Hello Plugin",
        "description": "A simple demo plugin",
        "hasUI": False,          # True 表示有 PySide6 UI
    }
```

可选接口：
- `execute(param: dict) -> dict`：纯功能执行
- `launch_ui() -> dict`：打开 PySide6 非模态窗口

### 5.5 PySide6 非模态窗口

```python
# 模块级变量持有窗口引用，防止函数返回后被 GC 回收
_active_windows: list = []

def launch_ui() -> dict:
    from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel
    from PySide6.QtCore import Qt
    # QApplication 已经由 C++ 主线程创建，PySide6 共享同一实例
    window = QWidget()
    window.setWindowTitle("Demo Plugin")
    window.setAttribute(Qt.WA_DeleteOnClose)
    window.destroyed.connect(lambda: _active_windows.remove(window))
    _active_windows.append(window)  # 持有引用防止 GC
    layout = QVBoxLayout(window)
    layout.addWidget(QLabel("Hello from PySide6!"))
    window.show()  # 非模态 .show()，不阻塞主窗口
    return {"ok": True, "message": "UI launched."}
```

**关键约束**：
- Windows DLL 搜索路径必须优先使用宿主 Qt（`os.add_dll_directory`），否则 PySide6 自带的 Qt DLL 会与宿主冲突
- 因为宿主已有 `QApplication`，PySide6 不能再创建新 `QApplication`
- 窗口通过 `.show()` 非模态显示，不冻结主窗口

### 5.6 Plugins Tab QML

Plugins Tab 是**动态**的：

1. 启动时或用户切换到 Plugins Tab 时，自动发送 `{ "module": "plugins", "action": "list" }` 请求
2. 收到响应后，解析插件列表，生成 ribbon groups
3. 每个插件对应一个 `RibbonTile`：
   - `title`：来自 `describe_plugin().name`
   - `icon`：从 10 个通用图标中循环选取 (pluginA ~ pluginJ)
   - 点击行为：
     - `hasUI == true` → 发送 `plugins.invoke_ui` 请求
     - `hasUI == false` → 发送 `plugins.execute` 请求

### 5.7 GIL 管理

```
main():
    EmbeddedPythonRuntime runtime(...)  // 构造时获取 GIL
    // ... 设置 QML engine ...
    {
        pybind11::gil_scoped_release release;  // 释放 GIL
        app.exec();                            // 主事件循环
    }  // release 析构，重新获取 GIL
    // runtime 析构，清理 Python 解释器
```

工作线程：
```
QtConcurrent::run([&runtime, json]() {
    // runtime.process() 内部调用 gil_scoped_acquire
    return runtime.process(json);
});
```

### 5.8 插件图标

从 ionicons-8.0.13 选取 10 个 outline 风格图标，复制到 `src/app/resource/icons/`：

| 文件名 | 来源 |
|--------|------|
| `pluginA.svg` | extension-puzzle-outline.svg |
| `pluginB.svg` | diamond-outline.svg |
| `pluginC.svg` | sparkles-outline.svg |
| `pluginD.svg` | rocket-outline.svg |
| `pluginE.svg` | flash-outline.svg |
| `pluginF.svg` | compass-outline.svg |
| `pluginG.svg` | prism-outline.svg |
| `pluginH.svg` | planet-outline.svg |
| `pluginI.svg` | cube-outline.svg |
| `pluginJ.svg` | flask-outline.svg |

## 6. 构建流程变更

### 6.1 依赖新增

```cmake
# 顶层 CMakeLists.txt
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
# pybind11 通过 CPM 引入，与项目其他依赖保持一致
opengeolab_resolve_package(pybind11 pybind11::pybind11
    VERSION 3.0.2
    GITHUB_REPOSITORY pybind/pybind11
    GIT_TAG v3.0.2)
include(cmake/SetupPySideVenv.cmake)
```

### 6.2 Plugin 拷贝

```cmake
# src/app/CMakeLists.txt 中添加 post-build
add_custom_command(TARGET opengeolab_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resource/python"
        "$<TARGET_FILE_DIR:opengeolab_app>/python"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/plugins"
        "$<TARGET_FILE_DIR:opengeolab_app>/plugins")
```

## 7. 测试验证

- [ ] 构建成功（cmake --build）
- [ ] 启动后 Plugins Tab 显示（至少含两个示例插件按钮）
- [ ] 点击 hello_plugin → ResponsePanel 或 statusNote 显示结果
- [ ] 点击 demo_ui_plugin → PySide6 窗口弹出，主窗口不冻结
- [ ] UI busy 状态正确（异步执行时有指示）
