# OpenGeoLab Architecture

本文档描述 OpenGeoLab 的分层架构：App 层、Module 层、Action 层如何组织，Python Plugin 与 PySide6 如何接入，以及该架构面向 LLM 大模型集成时带来的天然优势。

---

## 1. 整体架构总览

```
┌────────────────────────────────────────────────────────────────────────┐
│                            User Interface                              │
│  ┌────────────────┐  ┌────────────────────┐  ┌──────────────────────┐  │
│  │    QML / Qt    │  │   PySide6 Plugin   │  │  HTTP Client / LLM  │  │
│  │   (Main.qml)  │  │  (ai_chat, etc.)   │  │  (curl / Copilot)   │  │
│  └───────┬────────┘  └─────────┬──────────┘  └──────────┬───────────┘  │
└──────────┼─────────────────────┼────────────────────────┼──────────────┘
           │ Qt Signal           │ Python call             │ HTTP POST
           ▼                     ▼                         ▼
┌────────────────────────────────────────────────────────────────────────┐
│                          App Layer (Glue)                               │
│  ┌─────────────────┐  ┌───────────────────────┐  ┌─────────────────┐  │
│  │ RequestService  │  │ EmbeddedPythonRuntime │  │   HTTP Server   │  │
│  │ (QML ↔ Python)  │  │   (pybind11 host)     │  │ (Flask Plugin)  │  │
│  └────────┬────────┘  └───────────┬───────────┘  └────────┬────────┘  │
└───────────┼───────────────────────┼────────────────────────┼───────────┘
            │                       │                        │
            ▼                       ▼                        ▼
┌────────────────────────────────────────────────────────────────────────┐
│                      Python Router (统一入口)                           │
│                   opengeolab_runtime.py::process()                      │
│                                                                        │
│   ┌────────────┐   ┌────────────┐   ┌───────────────────────────────┐  │
│   │  plugins   │   │   system   │   │   fallback → C++ wrapper     │  │
│   │   .list    │   │ .describe  │   │   opengeolab_pywrapper       │  │
│   │ .invoke_ui │   │ .describe  │   │     .process(json)           │  │
│   │ .execute   │   │  _action   │   │     .describe()              │  │
│   └────────────┘   └────────────┘   └───────────────┬───────────────┘  │
└─────────────────────────────────────────────────────┼──────────────────┘
                                                      │ pybind11
                                                      ▼
┌────────────────────────────────────────────────────────────────────────┐
│                      Command Layer (C++ Dispatch)                      │
│                                                                        │
│                        CommandDispatcher                                │
│               dispatch({"module","action","param"})                     │
│                             │                                          │
│          ┌──────────────────┼──────────────────────┐                   │
│          ▼                  ▼                      ▼                   │
│     ModuleBase         ModuleBase             ModuleBase               │
│    "geometry"           "scene"                "mesh"                  │
│          │                  │                      │                   │
│    ┌─────┴──────┐    ┌─────┴──────┐         ┌─────┴─────┐            │
│    │  IAction   │    │  IAction   │         │  IAction  │            │
│    │ create_box │    │   select   │         │ gen_mesh  │            │
│    │ import_step│    │ set_camera │         │   ...     │            │
│    │   ...      │    │ capture_.. │         └───────────┘            │
│    └────────────┘    └────────────┘                                   │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. App 层

App 层是整个系统的胶水，负责初始化、布线和生命周期管理。

### 2.1 初始化流程 (`main.cpp`)

```
main()
 ├─ Qt 环境：OpenGL 3.3 Core、Fusion 风格
 ├─ registerBuiltinModules(factory)        ← 注册 C++ 模块
 ├─ CommandDispatcher(factory)             ← 创建分发器
 ├─ findModule() × 4                      ← 预热模块缓存 (io → geometry → scene → mesh)
 ├─ initBridge()                           ← 模块间桥接 (ShapeStore → SceneGraph → MeshStore)
 ├─ EmbeddedPythonRuntime(app, runtime, plugins)
 ├─ RequestService(dispatcher, runtime)    ← 注册为 QML Singleton
 ├─ ModuleDataNotifier(dispatcher)         ← C++ dataChanged → Qt Signal
 ├─ SelectionService                       ← 选择状态暴露给 QML
 ├─ QQmlApplicationEngine → Main.qml
 ├─ GLViewport ↔ SceneGraph 绑定
 ├─ [可选] --start-http-server → 自动启动 HTTP 插件
 └─ gil_scoped_release → QApplication::exec()
```

### 2.2 关键组件

| 组件 | 职责 | 线程模型 |
|------|------|----------|
| **RequestService** | QML ↔ Python 桥接 | `submitAsync()` worker 线程；`executeOnMainThread()` 主线程 |
| **ModuleDataNotifier** | C++ `dataChanged` 信号 → Qt 信号 | 订阅 C++ 信号，转发到 Qt 主线程 |
| **GLViewport** | OpenGL 渲染视口 | 在 Qt 渲染线程绘制 |
| **SelectionService** | 选择状态暴露给 QML | 主线程 |
| **EmbeddedPythonRuntime** | 嵌入 Python 解释器 | 内部管理 GIL |

---

## 3. Module 层

Module 是业务逻辑的组织单位。每个 Module 拥有独立的数据存储和一组 Action。

### 3.1 ModuleBase 接口

```cpp
class ModuleBase {
public:
    // 自描述 — 枚举所有注册的 Action 及其参数 schema
    virtual json describe() const;

    // 分发 — 根据 request["action"] 查找并执行对应 IAction
    virtual json process(const json& request, const ProgressCallback& progress) const;

    // 数据变更信号 — Action 修改数据后发射
    Signal<ModuleDataEvent> dataChanged;

protected:
    // 注册一个 Action 到全局工厂，key = "moduleName.actionName"
    template <class ActionT, class... Args>
    void registerAction(Args&&... args);
};
```

### 3.2 模块注册

```cpp
// module_registry.cpp — 启动时一次性注册
void registerBuiltinModules(PluginComponentFactory& factory) {
    factory.bindSingleton<ModuleBase, IOModule>("io", ref(factory));
    factory.bindSingleton<ModuleBase, GeometryModule>("geometry", ref(factory));
    factory.bindSingleton<ModuleBase, SceneModule>("scene", ref(factory));
    factory.bindSingleton<ModuleBase, MeshModule>("mesh", ref(factory));
}
```

### 3.3 已注册模块

```
Module "io"         ── IOModule
  └─ read_brep

Module "geometry"   ── GeometryModule ── owns ShapeStore
  ├─ create_box         创建长方体
  ├─ create_sphere      创建球体
  ├─ create_cylinder    创建圆柱
  ├─ create_torus       创建圆环
  ├─ import_brep        导入 BRep 文件
  ├─ import_step        导入 STEP 文件
  ├─ list_shapes        列出所有形状
  ├─ list_sub_shapes    列出子拓扑（面、边、顶点）
  ├─ query_shape        查询形状属性/包围盒
  ├─ delete_shape       删除形状
  └─ tessellate         细分可视化

Module "scene"      ── SceneModule ── owns SceneGraph
  ├─ new_model          创建场景根
  ├─ list_nodes         枚举场景树
  ├─ select / deselect / clear_selection    选择管理
  ├─ query_selection    查询选中实体
  ├─ set_visibility / set_display_mode      显示控制
  ├─ set_camera / set_view_preset           相机控制
  ├─ fit_to_scene       适配视图
  ├─ capture_viewport   截图（文件 / base64）
  ├─ set_pick_mode / pick_area              交互拾取
  ├─ add_label / remove_label / ...         标注管理
  └─ set_hover / set_auto_label             UI 反馈

Module "mesh"       ── MeshModule ── owns MeshStore
  ├─ generate_mesh      生成有限元网格
  ├─ query_mesh_info    查询网格信息
  └─ clear_mesh         清除网格
```

### 3.4 模块依赖与桥接

```
   IOModule (leaf)
       │
       ▼
GeometryModule ── ShapeStore
       │                 │
       │        initBridge(shapeStore)
       ▼                 ▼
  SceneModule ── SceneGraph ── SelectionState, LabelManager
       │                 │
       │    initBridge(sceneGraph, shapeStore)
       ▼                 ▼
  MeshModule ── MeshStore
```

- Bridge 模式让上层模块在 Shape 增删时自动同步场景节点和网格数据。
- `dataChanged` 信号链式传播：`Action 修改数据 → Module.dataChanged → CommandDispatcher → ModuleDataNotifier → Qt Signal → GLViewport.update()`。

---

## 4. Action 层

Action 是最小的可执行单元。每个 Action 做一件事，自包含完整的参数 schema。

### 4.1 IAction 接口

```cpp
class IAction {
public:
    // 自描述：返回 name, description, params, returns 四段 JSON
    virtual json describe() const = 0;

    // 执行：接收 param JSON，返回结果 JSON
    virtual json execute(const json& param, const ProgressCallback& progress) = 0;

    // 每个 Action 必须声明静态名称
    static constexpr string_view ACTION_NAME = "create_box";
};
```

### 4.2 describe() 示例

以 `CreateBoxAction` 为例：

```json
{
  "name": "create_box",
  "description": "Create a box primitive and register it in ShapeStore.",
  "params": {
    "width":  {"type": "number",  "required": false, "description": "Box width (default 1.0)"},
    "height": {"type": "number",  "required": false, "description": "Box height (default 1.0)"},
    "depth":  {"type": "number",  "required": false, "description": "Box depth (default 1.0)"},
    "origin": {"type": "array",   "required": false, "description": "[x,y,z] (default [0,0,0])"},
    "name":   {"type": "string",  "required": false, "description": "Shape name (default Box)"}
  },
  "returns": {
    "ok":       {"type": "boolean", "description": "true when success"},
    "action":   {"type": "string",  "description": "Echo of the action name"},
    "shapeId":  {"type": "integer", "description": "Allocated shape identifier"},
    "name":     {"type": "string",  "description": "Resolved shape name"},
    "topology": {"type": "object",  "description": "Topology counts"}
  }
}
```

### 4.3 统一请求 / 响应协议

**请求信封：**
```json
{
  "module": "geometry",
  "action": "create_box",
  "param": { "width": 3, "height": 2, "depth": 1 }
}
```

**成功响应：**
```json
{
  "ok": true,
  "module": "geometry",
  "action": "create_box",
  "summary": "Box created.",
  "result": { "shapeId": 0, "name": "Box", "topology": {...} },
  "errors": []
}
```

**失败响应：**
```json
{
  "ok": false,
  "module": "geometry",
  "action": "unknown_action",
  "summary": "Action 'unknown_action' not found in module 'geometry'.",
  "errors": ["..."]
}
```

---

## 5. Plugin 与 PySide6 接入

### 5.1 Plugin 发现机制

```
plugins/                          ← OPENGEOLAB_PLUGIN_ROOT
├── _shared/                      ← 共享资源（主题、图标）
├── http_server_plugin/           ← HTTP 服务插件
│   ├── __init__.py               ← describe_plugin() + launch_ui()
│   ├── server_backend.py         ← QObject 后端
│   ├── server_core.py            ← HTTP 服务器实现
│   └── qml/ServerWindow.qml      ← PySide6 QML 界面
├── ai_chat_plugin/               ← AI 对话插件
│   ├── __init__.py               ← describe_plugin() + launch_ui()
│   ├── chat_backend.py           ← Copilot SDK 集成
│   ├── tool_handlers.py          ← LLM Tool Calling 处理
│   ├── debugger_backend.py       ← Action 浏览器
│   └── qml/PluginWindow.qml      ← PySide6 QML 界面
└── selection_demo_plugin/        ← 选择演示插件
```

**发现流程 (`opengeolab_runtime.py`)：**
```python
def _discover_plugins():
    for finder, name, is_pkg in pkgutil.iter_modules([plugin_root]):
        mod = importlib.import_module(name)
        if hasattr(mod, "describe_plugin"):
            info = mod.describe_plugin()   # → {"name", "description", "hasUI"}
            plugins.append(info)
```

### 5.2 Plugin 接口约定

每个 Plugin 是一个 Python 包，可选暴露：

| 函数 | 用途 | 调用方式 |
|------|------|----------|
| `describe_plugin() → dict` | 元数据发现 | `plugins.list` |
| `launch_ui(param?) → dict` | 启动 PySide6 窗口 | `plugins.invoke_ui`（主线程） |
| `execute(param, progress?) → dict` | 无 UI 后端执行 | `plugins.execute`（worker 线程） |

### 5.3 PySide6 接入模式

```python
# ai_chat_plugin/__init__.py
def launch_ui() -> dict:
    app = QApplication.instance()
    if app is None:
        return {"ok": False, "message": "No QApplication instance."}

    backend = ChatBackend(config=ChatConfig())      # QObject 子类
    engine = QQmlApplicationEngine()
    engine.setInitialProperties({"chatBackend": backend})
    engine.load(QUrl.fromLocalFile(str(qml_dir / "PluginWindow.qml")))

    _active_engines.append(engine)                  # 防止 GC 回收
    app.aboutToQuit.connect(backend.shutdown)        # 优雅关闭
    return {"ok": True}
```

**关键约束：**
- `launch_ui()` 必须通过 `RequestService.executeOnMainThread()` 调用 — Qt 控件创建要求主线程亲和性。
- `main.cpp` 在进入事件循环前释放 GIL (`gil_scoped_release`)，确保 PySide6 信号处理不死锁。
- PySide6 插件创建独立的 `QQmlApplicationEngine`，与主 QML 引擎隔离。

### 5.4 Plugin 与 C++ Action 系统的交互

```
PySide6 Plugin
    │
    │  import opengeolab_pywrapper
    │  result = pywrapper.process('{"module":"geometry","action":"create_box",...}')
    │
    ▼
C++ CommandDispatcher → GeometryModule → CreateBoxAction
    │
    │  返回 JSON
    ▼
PySide6 Plugin 收到结果 → 更新 QML UI
```

Plugin 通过 `opengeolab_pywrapper` pybind11 模块直接调用 C++ Action 系统，无需经过 HTTP。

---

## 6. 渐进式发现（Progressive Discovery）

OpenGeoLab 内置了完整的运行时自省能力，任何客户端（HTTP、LLM、CLI）都可以从零开始发现所有可用操作。

### 6.1 发现层级

```
Level 0: 健康检查
  GET /api/v1/health → {"status": "ok"}

Level 1: 能力查询
  system.capabilities → Python 版本、PySide6 可用性、协议版本

Level 2: 模块枚举
  system.list_modules → [{"name": "geometry", "description": "..."}, ...]

Level 3: Action 枚举
  system.describe_module → {"actions": [{"name": "create_box", "description": "..."}, ...]}

Level 4: 参数 Schema
  system.describe_action → 完整的 params / returns JSON schema

Level 5: 全量 Schema
  system.describe → 一次性获取所有模块的所有 Action 及参数
```

### 6.2 发现流程图

```
客户端启动
    │
    ▼
system.list_modules ─────► 获得模块列表
    │                       ["geometry", "scene", "mesh", "io"]
    ▼
system.describe_module ──► 获得某模块的 action 列表
    │  param: {"module": "geometry"}
    │                       ["create_box", "import_step", ...]
    ▼
system.describe_action ──► 获得精确参数 schema
    │  param: {"module": "geometry",
    │          "action": "create_box"}
    ▼
geometry.create_box ─────► 构造请求，执行操作
    param: {"width": 3, "height": 2, "depth": 1}
```

### 6.3 Schema 自描述链

每个 `IAction::describe()` 返回的 JSON 包含四个稳定字段：

```
name ──────── Action 名称（与 request["action"] 匹配）
description ─ 人类可读 / LLM 可读的功能描述
params ────── 参数 schema（type, required, description）
returns ───── 返回值 schema（type, description）
```

`ModuleBase::describe()` 自动聚合所有注册 Action 的 `describe()` 输出。
`CommandDispatcher::describe()` 进一步聚合所有 Module，加上 `request_schema` 信封格式。

最终形成一棵完整的自描述树：

```json
{
  "request_schema": {
    "module":  {"type": "string", "required": true},
    "action":  {"type": "string", "required": true},
    "param":   {"type": "object", "required": false}
  },
  "modules": [
    {
      "name": "geometry",
      "description": "Geometry creation and manipulation",
      "actions": [
        {
          "name": "create_box",
          "description": "Create a box primitive...",
          "params": { ... },
          "returns": { ... }
        }
      ]
    }
  ]
}
```

---

## 7. LLM 大模型集成优势

### 7.1 天然适配 Tool Calling

现代 LLM（GPT、Claude、Copilot）的 Tool Calling / Function Calling 机制需要：

1. **函数列表** — `system.list_modules` + `system.describe_module` 提供
2. **参数 Schema** — `IAction::describe()` 的 `params` 直接映射为 Tool 的 `parameters`
3. **调用接口** — 统一的 `{"module", "action", "param"}` JSON 信封
4. **结果解析** — 统一的 `{"ok", "result", "errors"}` 响应

**对比传统 CAD 软件的 LLM 集成：**

| 维度 | 传统方式 | OpenGeoLab |
|------|----------|------------|
| API 发现 | 查阅文档 / 硬编码 | 运行时自省 `system.describe` |
| 参数格式 | 多种协议混杂 | 统一 JSON 信封 |
| 错误处理 | 异常 / 错误码 / 静默失败 | 结构化 `{"ok": false, "errors": [...]}` |
| 能力扩展 | 修改硬编码工具定义 | 新增 Action 后自动可发现 |
| 上下文大小 | 整个 API 文档灌入 Prompt | 按需查询，逐步深入 |

### 7.2 渐进式发现降低 Token 消耗

LLM 不需要一次性加载所有 API 文档。通过分层发现：

```
第 1 轮: "用户想建模" → system.list_modules → 发现 geometry 模块     (~200 tokens)
第 2 轮: 选定 geometry → system.describe_module → 发现 create_box    (~500 tokens)
第 3 轮: 选定 create_box → system.describe_action → 获取参数 schema  (~300 tokens)
第 4 轮: 构造请求 → geometry.create_box → 获取结果                   (~200 tokens)
```

vs. 一次性灌入全量文档 → ~5000+ tokens

### 7.3 实际集成：AI Chat Plugin

`ai_chat_plugin` 已实现了完整的 LLM ↔ Action 闭环：

```
用户对话 → Copilot SDK → Tool Calling
                            │
                ┌───────────┼────────────────┐
                ▼           ▼                ▼
         list_modules  describe_action  execute_action
                │           │                │
                └───────────┼────────────────┘
                            ▼
               opengeolab_pywrapper.process()
                            │
                            ▼
                    C++ CommandDispatcher
```

Tool 定义直接从 `system.describe` 生成，Action 新增后 LLM 自动获得新能力，**无需修改 Prompt 或工具定义代码**。

### 7.4 闭环验证能力

LLM 可以通过 `capture_viewport` 获取视口截图（base64），实现**视觉反馈闭环**：

```
LLM: create_box(width=3)        → 创建几何体
LLM: fit_to_scene()             → 调整视图
LLM: capture_viewport()         → 获取截图
LLM: [分析截图] "几何体看起来正确" → 确认或修正
```

这使得 LLM 不仅能"盲操作"，还能"看到"操作结果并自主纠错。

### 7.5 安全边界

- Action 层是原子操作 — LLM 无法执行任意代码，只能调用预定义的 Action。
- 参数验证在 Action 内部完成 — 无效参数返回结构化错误，不会导致崩溃。
- `opengeolab_pywrapper` 是唯一的 C++ 入口 — Python 端无法直接操作内存或绑定。

---

## 8. 数据流全景

以 "LLM 通过 HTTP 创建一个长方体" 为例，完整数据流：

```
1. HTTP Client
   POST /api/v1/action
   {"module":"geometry","action":"create_box","param":{"width":3}}
        │
2. http_server_plugin (Flask)
   ServerCore.handle_action()
        │  import opengeolab_pywrapper
        │  pywrapper.process(json)
        │
3. opengeolab_pywrapper (pybind11 C++ module)
   gil_scoped_release → CommandDispatcher::dispatch()
        │
4. CommandDispatcher
   getModule("geometry") → GeometryModule::process()
        │
5. GeometryModule::process()
   factory key = "geometry.create_box"
   getAction(key) → CreateBoxAction::execute()
        │
6. CreateBoxAction::execute()
   BRepPrimAPI_MakeBox(width, height, depth)
   ShapeStore::addShape(shape)
   return {"ok": true, "shapeId": 0, ...}
        │
7. GeometryModule → dataChanged signal
        │
8. CommandDispatcher → ModuleDataNotifier
        │
9. ModuleDataNotifier → Qt Signal → GLViewport::update()
        │
10. GLViewport 重新渲染 → 用户看到长方体
        │
11. HTTP Response → {"ok": true, "shapeId": 0, ...}
```

---

## 9. 扩展指南

### 新增一个 C++ Action

1. 创建 `FooAction` 类，继承 `IAction`，实现 `describe()` 和 `execute()`
2. 声明 `static constexpr string_view ACTION_NAME = "foo"`
3. 在对应 Module 构造函数中调用 `registerAction<FooAction>(...)`
4. 重新编译 — Action 自动可通过 `system.describe` 发现

### 新增一个 Python Plugin

1. 在 `plugins/` 下创建包目录
2. 实现 `describe_plugin()` 返回元数据
3. 可选实现 `launch_ui()` （PySide6 UI）或 `execute()` （无 UI 逻辑）
4. 重启应用 — Plugin 自动通过 `plugins.list` 发现

### 新增一个 C++ Module

1. 创建 `FooModule`，继承 `ModuleBase`
2. 在 `module_registry.cpp` 的 `registerBuiltinModules()` 中注册
3. 在 `main.cpp` 中添加预热和桥接逻辑（如需要）
4. 重新编译 — Module 自动可通过 `system.list_modules` 发现
