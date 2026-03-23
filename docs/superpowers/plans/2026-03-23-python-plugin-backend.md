# Python Plugin Backend 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 为 OpenGeoLabNew 引入 QML → C++ → Python 异步请求通道和插件系统，支持 PySide6 非模态 UI。

**架构：** C++ 层提供 EmbeddedPythonRuntime（pybind11::embed）和 ProcessService（Qt 异步桥接），Python 层提供 opengeolab_runtime.py 做请求路由和插件发现，QML 层在 Ribbon 新增动态 Plugins Tab。轻量级 pybind11 wrapper 模块预留未来 C++ 模块扩展。

**技术栈：** C++20, Qt6 (QML/Quick/Concurrent), pybind11 3.0.2, Python3, PySide6, CMake 3.25+

**规格文档：** `docs/superpowers/specs/2026-03-23-python-plugin-backend-design.md`

**参考项目：** `C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLabBack\src\`（以下简称 REF）

---

## 文件结构总览

### 新增文件

| 文件路径 | 职责 |
|----------|------|
| `src/libs/python/CMakeLists.txt` | Python 子模块父 CMakeLists |
| `src/libs/python/python_embedded/CMakeLists.txt` | embed 库构建 |
| `src/libs/python/python_embedded/include/opengeolab/python/embedded_python_runtime.hpp` | 嵌入式运行时头文件 |
| `src/libs/python/python_embedded/src/embedded_python_runtime.cpp` | 嵌入式运行时实现 |
| `src/libs/python/python_wrapper/CMakeLists.txt` | pybind11 wrapper 构建 |
| `src/libs/python/python_wrapper/src/python_wrapper_module.cpp` | 轻量级 pybind11 桥接 |
| `src/app/include/opengeolab/app/process_service.hpp` | ProcessService 头文件 |
| `src/app/src/process_service.cpp` | ProcessService 实现 |
| `src/app/resource/python/opengeolab_runtime.py` | Python 运行时路由 |
| `src/app/resource/qml/components/PluginRibbonGroup.qml` | 动态插件 ribbon 组件 |
| `plugins/hello_plugin.py` | 示例纯功能插件 |
| `plugins/demo_ui_plugin/__init__.py` | 示例 PySide6 UI 插件 |
| `src/app/resource/icons/pluginA.svg` ~ `pluginJ.svg` | 10 个通用插件图标 |

### 修改文件

| 文件路径 | 变更 |
|----------|------|
| `CMakeLists.txt` (顶层) | 添加 Python3, pybind11, SetupPySideVenv, add_subdirectory |
| `src/app/CMakeLists.txt` | 链接 Python 库, 添加新源文件, post-build 拷贝 |
| `src/app/src/main.cpp` | 初始化 Python 运行时, 注册 ProcessService, GIL 释放 |
| `src/app/resource/qml/RibbonConfig.qml` | 新增第 4 个 "Plugins" Tab |
| `src/app/resource/qml/Main.qml` | 连接 processService, 管理插件列表状态 |
| `src/app/resource/qml/sections/AppHeader.qml` | 支持动态 plugin ribbon groups |

---

## 任务 1：CMake 基础设施 — Python3 + pybind11 + PySide6 venv

**文件：**
- 修改：`CMakeLists.txt` (顶层)

**参考：** REF `CMakeLists.txt:177-185` (pybind11 resolve), REF `CMakeLists.txt:187-206` (Python3 find)

- [ ] 步骤 1：在顶层 `CMakeLists.txt` 的 `nlohmann_json` resolve 之后、Testing 段之前，添加：
  ```cmake
  # ---------------------------------------------------------------------------
  # Python: embedded runtime + pybind11 bridge
  # ---------------------------------------------------------------------------
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
  
  set(OPENGEOLAB_PYBIND11_VERSION 3.0.2)
  opengeolab_resolve_package(
      pybind11
      pybind11::pybind11
      VERSION
      ${OPENGEOLAB_PYBIND11_VERSION}
      GITHUB_REPOSITORY
      pybind/pybind11
      GIT_TAG
      v${OPENGEOLAB_PYBIND11_VERSION})
  
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SetupPySideVenv.cmake)
  ```
- [ ] 步骤 2：在 `add_subdirectory(src/app)` 之前添加：
  ```cmake
  add_subdirectory(src/libs/python)
  ```
- [ ] 步骤 3：运行 `cmake -S . -B build` 验证配置通过（Python3 找到、pybind11 解析、PySide6 venv 创建）
- [ ] 步骤 4：确认 `build/bin/python3.dll` 被复制（Windows）
- [ ] 步骤 5：提交：`build(cmake): add Python3, pybind11, and PySide6 venv setup`

**验证命令：** `cmake -S . -B build 2>&1 | Select-String "Python3|pybind11|PySide6"`

**预期结果：** 配置成功，Python3 被发现，pybind11 从 CPM 缓存加载，PySide6 安装到 pyvenv。

---

## 任务 2：python_embedded 库

**文件：**
- 新增：`src/libs/python/CMakeLists.txt`
- 新增：`src/libs/python/python_embedded/CMakeLists.txt`
- 新增：`src/libs/python/python_embedded/include/opengeolab/python/embedded_python_runtime.hpp`
- 新增：`src/libs/python/python_embedded/src/embedded_python_runtime.cpp`

**参考：**
- REF `src/libs/python/python_embedded/` 全部文件（直接复制并适配）

- [ ] 步骤 1：创建 `src/libs/python/CMakeLists.txt`，内容：
  ```cmake
  add_subdirectory(python_embedded)
  add_subdirectory(python_wrapper)
  ```
- [ ] 步骤 2：创建 `src/libs/python/python_embedded/CMakeLists.txt`，使用 `opengeolab_add_module`，参考 REF 同名文件。关键：
  - target: `opengeolab_python_embed`, alias: `Python`
  - 链接 `pybind11::embed` 和 `Python3::Python`
  - Windows compile definitions: `MS_NO_COREDLL`, `Py_ENABLE_SHARED`
  - PRIVATE definitions: `OPENGEOLAB_PYTHON_HOME`, `OPENGEOLAB_PYTHON_EXECUTABLE`
  - 如果定义了 `OPENGEOLAB_PYVENV_SITE_PACKAGES`，添加对应 definition
  - `opengeolab_copy_runtime_dlls`
- [ ] 步骤 3：创建 `embedded_python_runtime.hpp`，参考 REF 同名文件。Pimpl 模式，3 个 path 参数构造，`process(string_view) -> string`
- [ ] 步骤 4：创建 `embedded_python_runtime.cpp`，参考 REF 同名文件。关键函数：
  - `resolvePythonHome()` — 环境变量 > 编译时默认
  - `resolvePythonExecutable()` — 同上
  - `buildModuleSearchPaths()` — 9 路径包含 pyvenv
  - `initialize()` — PyConfig + scoped_interpreter + os.environ + import process
  - `process()` — gil_scoped_acquire + 调用 Python
- [ ] 步骤 5：运行 `cmake -S . -B build && cmake --build build --target opengeolab_python_embed --config RelWithDebInfo --parallel 4`
- [ ] 步骤 6：确认编译无错误无警告
- [ ] 步骤 7：提交：`feat(python): add embedded Python runtime library`

**验证命令：** `cmake --build build --target opengeolab_python_embed --config RelWithDebInfo --parallel 4`

**预期结果：** `opengeolab_python_embed.dll`（或 `.lib`）生成在 `build/bin/`。

---

## 任务 3：python_wrapper pybind11 模块

**文件：**
- 新增：`src/libs/python/python_wrapper/CMakeLists.txt`
- 新增：`src/libs/python/python_wrapper/src/python_wrapper_module.cpp`

**参考：** REF `src/libs/python/python_wrapper/` 全部文件（简化版，无 Command 依赖）

- [ ] 步骤 1：创建 `src/libs/python/python_wrapper/CMakeLists.txt`：
  ```cmake
  pybind11_add_module(opengeolab_pywrapper src/python_wrapper_module.cpp)
  target_compile_features(opengeolab_pywrapper PRIVATE cxx_std_20)
  target_compile_definitions(
      opengeolab_pywrapper
      PRIVATE $<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>
              $<$<PLATFORM_ID:Windows>:Py_ENABLE_SHARED>)
  target_link_libraries(opengeolab_pywrapper PRIVATE pybind11::pybind11)
  set_target_properties(
      opengeolab_pywrapper
      PROPERTIES FOLDER "python"
                 LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
                 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  install(
      TARGETS opengeolab_pywrapper
      LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  ```
- [ ] 步骤 2：创建 `python_wrapper_module.cpp`。轻量占位实现：
  - `PYBIND11_MODULE(opengeolab_pywrapper, module)`
  - `module.def("process", ...)` — 解析 JSON，返回 `{"ok": false, "summary": "No C++ modules registered yet"}`
  - `module.def("protocol_version", ...)` — 返回 `"1.0"`
  - 使用 `nlohmann/json` 或 pybind11 自带 JSON 处理
- [ ] 步骤 3：注意 wrapper 不依赖 nlohmann_json（保持轻量），直接用字符串拼接或 pybind11::dict 返回
- [ ] 步骤 4：运行 `cmake --build build --target opengeolab_pywrapper --config RelWithDebInfo --parallel 4`
- [ ] 步骤 5：确认 `opengeolab_pywrapper.cp3XX-win_amd64.pyd` 生成在 `build/bin/`
- [ ] 步骤 6：提交：`feat(python): add lightweight pybind11 wrapper module`

**验证命令：** `cmake --build build --target opengeolab_pywrapper --config RelWithDebInfo --parallel 4`

**预期结果：** `.pyd` 文件生成。

---

## 任务 4：ProcessService — QML ↔ Python 桥接

**文件：**
- 新增：`src/app/include/opengeolab/app/process_service.hpp`
- 新增：`src/app/src/process_service.cpp`

**参考：** REF `src/app/include/opengeolab/app/process_service.hpp` 和 `src/app/src/process_service.cpp`

- [ ] 步骤 1：创建 `process_service.hpp`：
  - 类 `ProcessService : public QObject`
  - `Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)`
  - 构造函数接收 `EmbeddedPythonRuntime&`
  - `Q_INVOKABLE void submitRequest(const QString& requestJson)`
  - 信号：`responseReady(QString responseJson)`, `errorOccurred(QString errorMessage)`, `busyChanged()`
  - 私有成员：`m_runtime` 引用, `std::atomic<int> m_pendingCount{0}`
- [ ] 步骤 2：创建 `process_service.cpp`：
  - `submitRequest()` 路由逻辑：
    - 解析 JSON，提取 `module` 和 `action`
    - 如果 `module == "plugins" && action == "invoke_ui"` → 主线程同步执行
    - 否则 → `QtConcurrent::run()` + `QFutureWatcher` 异步执行
  - 同步路径：try/catch 调用 `m_runtime.process()`，根据 `ok` 字段 emit 对应信号
  - 异步路径：increment pending → run on pool → on finished: decrement, parse, emit
- [ ] 步骤 3：暂不修改 CMakeLists（任务 5 统一做）
- [ ] 步骤 4：提交：`feat(app): add ProcessService for QML-Python bridge`

---

## 任务 5：App CMake + main.cpp 集成

**文件：**
- 修改：`src/app/CMakeLists.txt`
- 修改：`src/app/src/main.cpp`

**参考：** REF `src/app/CMakeLists.txt` 和 REF `src/app/src/main.cpp`

- [ ] 步骤 1：修改 `src/app/CMakeLists.txt`：
  - 在 `qt_add_executable` 的源文件列表中添加 `src/process_service.cpp`
  - 在 `SOURCES` 中添加 `include/opengeolab/app/process_service.hpp`
  - 在 `target_link_libraries` 添加 `OpenGeoLab::Python` 和 `Qt6::Concurrent`
  - 添加 post-build 命令拷贝 `resource/python/` 到 `$<TARGET_FILE_DIR:opengeolab_app>/python`
  - 添加 post-build 命令拷贝 `${PROJECT_SOURCE_DIR}/plugins/` 到 `$<TARGET_FILE_DIR:opengeolab_app>/plugins`
- [ ] 步骤 2：修改 `main.cpp`：
  - 添加 includes：`pybind11/pybind11.h`, `process_service.hpp`, `embedded_python_runtime.hpp`
  - 在 `QApplication app(...)` 之后、`QQmlApplicationEngine` 之前：
    - 计算 `app_dir`, `runtime_dir`, `plugin_dir`
    - 构造 `EmbeddedPythonRuntime python_runtime(...)`
    - 构造 `ProcessService process_service(python_runtime)`
  - 在 engine 创建后：`engine.rootContext()->setContextProperty("processService", &process_service)`
  - 添加 `#include <QQmlContext>`
  - 在 `return app.exec()` 前：改为 `const pybind11::gil_scoped_release release; return app.exec();`
- [ ] 步骤 3：运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 4：确认 app 编译成功
- [ ] 步骤 5：提交：`feat(app): integrate Python runtime and ProcessService into main`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 4`

**预期结果：** `opengeolab_app.exe` 编译成功。

---

## 任务 6：opengeolab_runtime.py

**文件：**
- 新增：`src/app/resource/python/opengeolab_runtime.py`

**参考：** REF `src/app/resource/python/opengeolab_runtime.py`（简化版，移除 recording 功能）

- [ ] 步骤 1：创建 `opengeolab_runtime.py`，包含：
  - Windows DLL 目录注册（`os.add_dll_directory`）
  - `PROTOCOL_VERSION = "1.0"`
  - `_plugin_root()` — 从环境变量或相对路径解析插件目录
  - `_python_capabilities()` — 返回运行时信息（PySide6 可用性等）
  - `_discover_plugins()` — `pkgutil.iter_modules` 扫描，调用 `describe_plugin()`
  - `_make_response(module, action, ok, summary, result, errors)` — 统一响应构建
  - 路由分发（基于 `module` + `action`）：
    - `plugins` + `list` → `_plugins_response()`
    - `plugins` + `invoke_ui` → `_launch_plugin_ui(request)`
    - `plugins` + `execute` → `_execute_plugin(request)`
    - `system` + `capabilities` → `_capabilities_response()`
    - 其它 → `_import_backend_wrapper().process(json)`
  - `def process(request_json: str) -> str:` 入口函数
- [ ] 步骤 2：确保所有返回值是 `json.dumps()` 序列化的字符串
- [ ] 步骤 3：确保错误处理用 try/except + traceback，不静默失败
- [ ] 步骤 4：提交：`feat(python): add opengeolab_runtime.py request router`

---

## 任务 7：示例插件

**文件：**
- 新增：`plugins/hello_plugin.py`
- 新增：`plugins/demo_ui_plugin/__init__.py`

- [ ] 步骤 1：创建 `plugins/hello_plugin.py`：
  ```python
  """Hello Plugin — 纯功能插件示例。"""
  from __future__ import annotations

  def describe_plugin() -> dict:
      return {
          "name": "Hello Plugin",
          "description": "Returns a greeting message.",
          "hasUI": False,
      }

  def execute(param: dict) -> dict:
      name = param.get("name", "World")
      return {"ok": True, "message": f"Hello, {name}!"}
  ```
- [ ] 步骤 2：创建 `plugins/demo_ui_plugin/__init__.py`：
  ```python
  """Demo UI Plugin — PySide6 非模态窗口示例。"""
  from __future__ import annotations

  _active_windows: list = []

  def describe_plugin() -> dict:
      return {
          "name": "Demo UI",
          "description": "Opens a PySide6 window (non-modal).",
          "hasUI": True,
      }

  def launch_ui() -> dict:
      from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QPushButton
      from PySide6.QtCore import Qt

      window = QWidget()
      window.setWindowTitle("Demo UI Plugin")
      window.setMinimumSize(320, 200)
      window.setAttribute(Qt.WA_DeleteOnClose)
      window.destroyed.connect(lambda: _active_windows.remove(window))
      _active_windows.append(window)

      layout = QVBoxLayout(window)
      layout.addWidget(QLabel("Hello from PySide6 Plugin!"))
      btn = QPushButton("Close")
      btn.clicked.connect(window.close)
      layout.addWidget(btn)

      window.show()
      return {"ok": True, "message": "Demo UI launched."}
  ```
- [ ] 步骤 3：提交：`feat(plugins): add hello_plugin and demo_ui_plugin examples`

---

## 任务 8：插件图标

**文件：**
- 新增：`src/app/resource/icons/pluginA.svg` ~ `pluginJ.svg` (10 个)
- 修改：`src/app/CMakeLists.txt` (RESOURCES 列表)

**来源：** `C:\Users\layton\Desktop\WorkSpace\Project\ionicons-8.0.13\src\svg\`

- [ ] 步骤 1：从 ionicons 目录拷贝 10 个 outline SVG 到 `src/app/resource/icons/`：
  | 目标名 | 来源名 |
  |--------|--------|
  | pluginA.svg | extension-puzzle-outline.svg |
  | pluginB.svg | diamond-outline.svg |
  | pluginC.svg | sparkles-outline.svg |
  | pluginD.svg | rocket-outline.svg |
  | pluginE.svg | flash-outline.svg |
  | pluginF.svg | compass-outline.svg |
  | pluginG.svg | prism-outline.svg |
  | pluginH.svg | planet-outline.svg |
  | pluginI.svg | cube-outline.svg |
  | pluginJ.svg | flask-outline.svg |
- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 `qt_add_qml_module` `RESOURCES` 段追加 10 个 SVG
- [ ] 步骤 3：提交：`chore(app): add 10 generic plugin icons from ionicons`

**验证命令：** 确认 10 个 SVG 文件存在且非空。

---

## 任务 9：QML — Plugins Tab 和动态 Ribbon

**文件：**
- 修改：`src/app/resource/qml/RibbonConfig.qml`
- 新增：`src/app/resource/qml/components/PluginRibbonGroup.qml`
- 修改：`src/app/resource/qml/Main.qml`
- 修改：`src/app/resource/qml/sections/AppHeader.qml`
- 修改：`src/app/CMakeLists.txt` (QML_FILES 列表)

**注意：** 本任务不涉及 TDD，因为 QML 组件验证依赖完整运行时集成。验证方式为构建成功 + 手动启动应用确认 UI。

- [ ] 步骤 1：修改 `RibbonConfig.qml`：
  - `tabs` 数组新增 `qsTr("Plugins")` 作为第 4 项
  - `groupsModel` 新增第 4 个空数组 `[]`（Plugins Tab 的 groups 由 Main.qml 动态管理，不走静态 config）
- [ ] 步骤 2：创建 `PluginRibbonGroup.qml`（在 `components/` 下）：
  - 接收 `required property AppTheme theme`
  - 接收 `property var plugins: []` — 插件列表数组
  - 接收 `required property var actionHandler`
  - 10 个图标名常量数组：`["pluginA", "pluginB", ..., "pluginJ"]`
  - 使用 `Row` + `Repeater` 渲染，每个插件一个 `RibbonTile`
  - icon 使用 `pluginIcons[index % 10]`
  - actionKey 使用 `"plugin_" + modelData.name`（hasUI 为 true 则 `"pluginUI_" + name`）
  - 空态：显示 "No plugins" 文本
- [ ] 步骤 3：修改 `Main.qml`：
  - 新增 `property var pluginList: []` — 保存发现的插件列表
  - 新增 `property bool pluginListLoaded: false`
  - 连接 `processService.responseReady` 信号：
    - 解析 responseJson，如果 `module == "plugins" && action == "list"` → 更新 `pluginList`
  - 连接 `processService.errorOccurred` 信号：
    - 更新 `statusNote` 显示错误
  - 修改 `openActionPage(actionKey)` 函数：
    - 如果 `actionKey.startsWith("pluginUI_")` → 提取 plugin name，发送 `plugins.invoke_ui` 请求
    - 如果 `actionKey.startsWith("plugin_")` → 提取 plugin name，发送 `plugins.execute` 请求
  - 在 `Component.onCompleted` 中发送 `plugins.list` 请求以初始化
  - 在 Ribbon Tab 切换到 Plugins Tab 时也触发刷新
- [ ] 步骤 4：修改 `AppHeader.qml`：
  - 新增 `property var pluginList: []`
  - 新增 `property int pluginTabIndex: 3` — Plugins Tab 的固定索引
  - 当 `selectedTab === pluginTabIndex` 时，ribbon 内容区显示 `PluginRibbonGroup` 而非 `HeaderRibbonGroup` Repeater
  - 实现方式：在 ribbon 内容区的 `Flickable` 中使用 `Loader` 或条件渲染：
    - 非 plugin tab → 现有 `Repeater` + `HeaderRibbonGroup`
    - plugin tab → `PluginRibbonGroup`
- [ ] 步骤 5：更新 `Main.qml` 中 `AppHeader` 调用，传入 `pluginList`
- [ ] 步骤 6：在 `src/app/CMakeLists.txt` 的 `QML_FILES` 列表中添加 `resource/qml/components/PluginRibbonGroup.qml`
- [ ] 步骤 7：提交：`feat(app): add dynamic Plugins ribbon tab with plugin discovery`

---

## 任务 10：集成构建验证

**文件：** 无新增/修改

- [ ] 步骤 1：完整重新配置：`cmake -S . -B build`
- [ ] 步骤 2：完整构建：`cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 3：确认构建产物：
  - `build/bin/opengeolab_app.exe`
  - `build/bin/opengeolab_python_embed.dll`（或 `.lib`）
  - `build/bin/opengeolab_pywrapper.cp3XX-win_amd64.pyd`
  - `build/bin/python/opengeolab_runtime.py`
  - `build/bin/plugins/hello_plugin.py`
  - `build/bin/plugins/demo_ui_plugin/__init__.py`
  - `build/bin/python3.dll`
- [ ] 步骤 4：如有回归测试：`ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 5：提交所有变更（如有遗漏的小修复）

**验证命令：**
```
cmake -S . -B build
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

**预期结果：** 全部构建通过，无编译错误。运行时文件均被正确拷贝。

---

## 依赖关系

```
任务 1 (CMake 基础设施)
  ├─→ 任务 2 (python_embedded)
  │     └─→ 任务 4 (ProcessService)
  │           └─→ 任务 5 (App 集成)
  ├─→ 任务 3 (python_wrapper)
  │
  任务 6 (runtime.py) — 独立，但需任务 1 的路径知识
  任务 7 (示例插件) — 独立
  任务 8 (图标) — 独立
  任务 9 (QML) — 依赖任务 5 (processService 已注册)
  任务 10 (集成验证) — 依赖所有任务
```

**可并行：** 任务 6 + 7 + 8 可与任务 2~5 并行开发。

## 不使用 TDD 的说明

本计划以**构建验证**为主要检验手段，不引入独立单元测试，原因：

1. **EmbeddedPythonRuntime** — 测试需要完整 Python 环境 + pybind11 初始化，属于集成测试范畴
2. **ProcessService** — 依赖运行中的 Python 运行时和 Qt 事件循环
3. **QML 组件** — 需要完整 QML 引擎 + processService context
4. **opengeolab_runtime.py** — 可独立测试但不在本计划范围（后续可补）

验证方式：每个任务的构建命令 + 最终集成测试（手动启动应用）。
