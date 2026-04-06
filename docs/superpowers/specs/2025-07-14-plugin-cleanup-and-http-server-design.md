# Plugin Cleanup & HTTP Server Plugin Design

## 问题陈述

当前 OpenGeoLab 项目存在以下需要整理的问题：

1. **测试/演示插件残留**：`hello_plugin.py`、`progress_demo_plugin.py`、`demo_ui_plugin/`、`selection_demo_plugin/` 四个插件仅用于开发演示，不应出现在生产环境中。
2. **AI Tab 暴露**：Ribbon 中的 "AI" Tab（含 Suggest / Chat）属于内部功能入口，需要隐藏；`ai_chat_plugin` 仍可通过 Plugins tab 正常启动。
3. **缺少外部集成接口**：OpenGeoLab 目前只能通过内嵌 Python 运行时执行 action，没有标准化的外部访问方式。需要一个本地 HTTP 服务插件，允许外部程序通过 JSON over HTTP 调用任何 action。

## 涉及范围

### Task 1: 移除 4 个 Demo 插件

**删除文件：**
- `plugins/hello_plugin.py`
- `plugins/progress_demo_plugin.py`
- `plugins/demo_ui_plugin/`（整个目录）
- `plugins/selection_demo_plugin/`（整个目录）

**保留文件：**
- `plugins/_shared/`（未来插件可复用的主题工具）
- `plugins/ai_chat_plugin/`（生产插件）

**无需修改的代码：**
- `opengeolab_runtime.py` — 插件发现是动态的（`pkgutil.iter_modules`），删除文件即可
- `CMakeLists.txt` — `copy_directory` 整体复制 `plugins/`，无需调整
- 无测试文件引用这些 demo 插件

### Task 2: 隐藏 AI Ribbon Tab

**修改文件：**

1. **`src/app/resource/qml/RibbonConfig.qml`**
   - 从 `tabs` 数组中移除 `"AI"` 项：`["Geometry", "Mesh", "Plugins"]`（3 个 tab）
   - 从 `groupsModel` 中移除 AI tab 对应的 groups 数组（原 index 2 的 Assist 组）
   - Plugins tab 的 groupsModel 条目从 index 3 调整为 index 2

2. **`src/app/resource/qml/Main.qml`**
   - 移除 `actionKey === "aiChat"` 的特殊处理分支（lines 100-113）
   - `ai_chat_plugin` 的 `hasUI: true` 已保证它在 Plugins tab 中显示为可点击按钮，通过已有的 `pluginUI_*` 通用路径启动

**不删除的资源：**
- AI 图标资源文件保留（`aiChat`、`aiSuggest`），未来可能复用

### Task 3: 新增 http_server_plugin

#### 3.1 插件概述

`http_server_plugin` 是一个带 UI 面板的 PySide6 插件。用户在 Plugins tab 中点击启动后，打开管理面板，手动启动/停止一个本地 HTTP 服务。外部客户端可通过 `POST /api/v1/action` 发送 JSON 请求，服务端透传给 `pywrapper.process()` 并返回完整响应。

#### 3.2 目录结构

```
plugins/http_server_plugin/
├── __init__.py          # describe_plugin() + launch_ui()
├── server_core.py       # HTTPServer 线程管理、请求处理
├── server_backend.py    # QObject 桥接层（QML ↔ Python）
├── qml/
│   └── ServerWindow.qml # 管理面板 UI
```

#### 3.3 协议设计

**端点：** `POST /api/v1/action`

**请求格式（与现有 pywrapper 协议完全一致）：**
```json
{
  "module": "geometry",
  "action": "create_box",
  "param": {"width": 10, "height": 10, "depth": 10}
}
```

**响应格式（原样返回 `process()` 结果）：**
```json
{
  "protocolVersion": "1.0",
  "ok": true,
  "module": "geometry",
  "action": "create_box",
  "summary": "Box created.",
  "result": {"shapeId": 0},
  "errors": []
}
```

**错误响应（请求格式错误等）：**
- `400 Bad Request` — JSON 解析失败或缺少必需字段
- `500 Internal Server Error` — `process()` 抛出未捕获异常
- 正常业务错误（`ok: false`）仍返回 `200`，错误信息在 JSON body 中

**辅助端点：**
- `GET /api/v1/health` — 健康检查，返回 `{"status": "running", "version": "1.0"}`

**Content-Type：** 请求和响应均为 `application/json; charset=utf-8`

#### 3.4 服务器架构

```
┌─────────────────────────────────────────────┐
│              http_server_plugin              │
│                                              │
│  ┌──────────────┐    ┌───────────────────┐  │
│  │ ServerWindow  │    │  ServerBackend    │  │
│  │   (QML UI)    │◄──►│  (QObject)        │  │
│  │               │    │                   │  │
│  │ • Start/Stop  │    │ • host/port props │  │
│  │ • Request log │    │ • running state   │  │
│  │ • Status bar  │    │ • request log     │  │
│  └──────────────┘    │ • start()/stop()  │  │
│                       └───────┬───────────┘  │
│                               │              │
│                       ┌───────▼───────────┐  │
│                       │   ServerCore      │  │
│                       │  (daemon thread)  │  │
│                       │                   │  │
│                       │ • HTTPServer      │  │
│                       │ • RequestHandler  │  │
│                       │ → process() call  │  │
│                       └───────────────────┘  │
└─────────────────────────────────────────────┘
```

#### 3.5 组件职责

**`server_core.py` — HTTP 服务器线程**

- 基于 `http.server.HTTPServer` + `BaseHTTPRequestHandler`
- 运行在独立守护线程（`threading.Thread(daemon=True)`）
- `RequestHandler.do_POST`:
  1. 读取请求 body，解析 JSON
  2. 调用 `pywrapper.process(json_str)` 透传执行
  3. 将 `process()` 返回的 JSON 字符串作为 HTTP 响应
- `RequestHandler.do_GET`:
  - `/api/v1/health` → 健康检查
  - 其他路径 → `404`
- 提供 `start(host, port)` / `stop()` / `is_running()` 接口
- 每次请求产生一条日志记录（时间、方法、路径、状态码、耗时）

**`server_backend.py` — QML 桥接层**

QObject 子类，暴露以下接口给 QML：

| 类型 | 名称 | 说明 |
|------|------|------|
| `Q_PROPERTY(str)` | `host` | 监听地址，默认 `"127.0.0.1"` |
| `Q_PROPERTY(int)` | `port` | 监听端口，默认 `8080` |
| `Q_PROPERTY(bool)` | `running` | 服务是否正在运行 |
| `Q_PROPERTY(QVariant)` | `requestLog` | 请求日志列表（最近 100 条） |
| `Q_INVOKABLE` | `start()` | 启动服务器 |
| `Q_INVOKABLE` | `stop()` | 停止服务器 |
| `Signal` | `requestReceived(dict)` | 新请求到达时发射 |
| `Signal` | `errorOccurred(str)` | 启动失败等错误 |

请求日志条目结构：
```json
{
  "time": "14:32:05",
  "method": "POST",
  "path": "/api/v1/action",
  "module": "geometry",
  "action": "create_box",
  "status": 200,
  "duration_ms": 42,
  "ok": true
}
```

**`ServerWindow.qml` — 管理面板**

布局：
```
┌─────────────────────────────────────────┐
│  HTTP Server                        [×] │
├─────────────────────────────────────────┤
│  Host: [127.0.0.1]  Port: [8080]       │
│  [▶ Start Server]  Status: ● Stopped   │
├─────────────────────────────────────────┤
│  Request Log                            │
│  ┌─────────────────────────────────────┐│
│  │ 14:32:05 POST geometry.create_box   ││
│  │          → 200 OK (42ms)            ││
│  │ 14:32:08 POST scene.select          ││
│  │          → 200 OK (3ms)             ││
│  │ 14:32:10 GET  /api/v1/health        ││
│  │          → 200 OK (1ms)             ││
│  └─────────────────────────────────────┘│
│                                         │
│  Selected Request/Response              │
│  ┌─────────────────────────────────────┐│
│  │ Request:                            ││
│  │ {"module":"geometry","action":...}  ││
│  │                                     ││
│  │ Response:                           ││
│  │ {"ok":true,"result":{"shapeId":0}} ││
│  └─────────────────────────────────────┘│
└─────────────────────────────────────────┘
```

功能：
- Host / Port 输入框（服务停止时可编辑，运行时只读）
- Start / Stop 按钮（根据运行状态切换文本和颜色）
- 状态指示器（绿色 Running / 红色 Stopped）
- 请求日志列表（ListView，最近 100 条，新请求自动滚动到底部）
- 点击日志条目显示完整的请求和响应 JSON
- 所有用户可见文本使用 `qsTr()` 包裹

#### 3.6 线程模型与 GIL 注意事项

```
Main Thread (Qt)          HTTP Thread (daemon)
     │                         │
     │                         ├─ HTTPServer.serve_forever()
     │                         │    │
     │                         │    ├─ do_POST() received
     │                         │    │    │
     │                         │    │    ├─ gil_scoped_release NOT needed
     │                         │    │    │  (process() is pure Python,
     │                         │    │    │   no C++ dispatch blocking)
     │                         │    │    │
     │                         │    │    ├─ pywrapper.process(json)
     │  ◄── QueuedConnection ──│    │    │   (C++ actions may post
     │      if action needs    │    │    │    back to main thread)
     │      main thread        │    │    │
     │                         │    │    ├─ response JSON
     │                         │    │    └─ emit requestReceived()
     │  ◄── QueuedConnection ──│    │       (updates QML log)
     │                         │    │
```

关键约束：
- `pywrapper.process()` 内部调用 C++ `dispatch()`，该函数已有 `gil_scoped_release`（见 `python_wrapper_module.cpp:64-74`），不会造成 GIL 死锁
- `ServerBackend` 的信号使用 `Qt.QueuedConnection` 确保 QML 更新在主线程
- HTTP 服务器线程是守护线程，应用退出时自动停止
- `plugins.invoke_ui` 类型的 action 需要主线程执行；HTTP 线程透传时 `opengeolab_runtime.py` 已有 `executeOnMainThread` 的 QueuedConnection 机制处理此情况

#### 3.7 配置

- 默认 host: `127.0.0.1`，默认 port: `8080`
- 通过 UI 面板可修改（在服务停止状态下）
- 端口冲突时返回明确错误（`errorOccurred` 信号 → UI 显示）
- 不持久化配置（每次启动恢复默认值，保持简单）

#### 3.8 安全考虑

- 默认仅绑定 `127.0.0.1`，外部网络不可访问
- 无认证机制（本地开发工具，信任本机）
- UI 面板中 host 可切换为 `0.0.0.0`（局域网访问），用户自行承担风险
- CORS headers：`Access-Control-Allow-Origin: *`（方便浏览器端调用）

#### 3.9 错误处理

| 场景 | 处理方式 |
|------|---------|
| 端口被占用 | `errorOccurred("Port 8080 is already in use.")` → UI 红色提示 |
| JSON 解析失败 | HTTP 400 + `{"ok": false, "error": "Invalid JSON"}` |
| `process()` 异常 | HTTP 500 + `{"ok": false, "error": "Internal server error: ..."}` |
| action 业务错误 | HTTP 200 + 正常 `process()` 响应（`ok: false` 在 body 中） |
| 服务未启动时请求 | 不会发生（HTTP 端口未监听，连接直接被拒绝） |

#### 3.10 测试策略

- **`server_core.py` 单元测试**：启动服务器，发送 HTTP 请求，验证 JSON 透传和响应格式
- **`server_backend.py` 单元测试**：mock `ServerCore`，验证 start/stop 状态管理和信号发射
- 测试使用 `unittest.mock.patch` mock `pywrapper.process()`
- 测试使用动态端口（port=0）避免端口冲突
- 测试文件：`plugins/http_server_plugin/tests/`

## 实施顺序

1. **Task 1** — 删除 4 个 demo 插件
2. **Task 2** — 隐藏 AI ribbon tab
3. **Task 3** — 实现 http_server_plugin（server_core → server_backend → QML UI → tests）

Task 1 和 Task 2 无依赖关系，可并行。Task 3 独立于前两者。
