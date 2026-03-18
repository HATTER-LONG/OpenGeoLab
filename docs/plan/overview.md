# 项目概况

OpenGeoLab 是一个面向 CAE 前处理场景的软件平台，目标能力包括：

- 几何导入与管理
- 几何清理与修复
- 网格生成与质量检查
- 3D 可视化
- Python 自动化
- LLM 辅助建模与流程编排

核心技术栈：

- UI: PySide6 QML Ribbon 风格
- Core: C++20
- Rendering: OpenGL
- Geometry: OpenCascade
- Mesh: Gmsh
- Python Bridge: pybind11
- Logging / Infra: spdlog, fmt, nlohmann_json, Kangaroo

## 项目结构

采用模块化主结构：

- `apps/OpenGeoLabApp`：PySide6 + QML 实现界面相关；优先记录可回放的语义化 Python 命令。Phase 1 的右侧视口需要真实显示 Box，并支持 rotate / zoom / pan，但该类高频相机交互先作为本地 viewport 行为，不进入 replay 命令流。
- `libs/python_wrapper`：唯一的 Python ↔ C++ pybind11 边界。P稳定暴露 `execute_command(json)`、`get_scene_snapshot()`、`poll_events()`、`poll_logs()`，并通过独立 viewport bridge 暴露视口交互与只读 render 查询能力。
- `libs/command`：统一入口、命令路由、请求/响应契约、request-scoped event batch，以及显式的 scene → render 同步顺序。
- `libs/core`：公共定义、核心方法、配置信息、基础设施类型。
- `libs/geometry`：几何。
- `libs/mesh`：网格。
- `libs/render`：渲染相关；持有 render cache、viewport 交互逻辑，以及只读 query / debug state，不直接成为业务状态源。
- `libs/selection`：拾取器相关。
- `etc`：按设计可以自己重定义 libs。

┌────────────────────────────┐
│        UI Layer            │
│  QML / PySide6 / Toolbar   │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Interaction Layer      │
│  - Command Recorder        │
│  - Input Mapper (UI → Cmd) │
│  - Viewport Host           │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Python Adapter         │  ← pybind11
│  - execute_command(json)   │
│  - get_scene_snapshot()    │
│  - poll_events()           │
│  - poll_logs()             │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│       Command Bus          │ ⭐核心
│  - validate                │
│  - dispatch                │
│  - middleware              │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│       Orchestrator         │
│  - workflow seam           │
│  - undo/redo hook          │
│  - render sync ordering    │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│      Service Router        │
│  - module lookup           │
│  - action routing          │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Module Services        │
│ geometry / mesh /          │
│ selection / render         │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Scene / Data Model     │ ⭐唯一状态源
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│       RenderData           │
│  read-only query/debug     │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│         OpenGL             │
└────────────────────────────┘

## 架构约束
- 所有用户操作必须转化为 Command，所有模块间通信必须通过 Service Router，所有状态必须存储在 Scene Model，渲染层只依赖 RenderData，不依赖 Geometry/Mesh，LLM 只能调用 Command，不允许直接调用底层模块。
- 所有功能模块优先通过 Kangaroo ComponentFactory 暴露 service 与 action factory；也就是不同 module 通过 module name 对外提供统一组件接口，内部通过 action name 调用对应功能。
- service 请求统一使用 `{ module, action, param }` JSON 信封，`param` 必须是 object。
- 用户可见操作优先走命令系统；但 Phase 1 的 viewport rotate / zoom / pan 属于本地相机交互，不作为 replay 命令记录。
- bootstrap 层统一拥有共享 `EventQueue` 与 `LogQueue` 生命周期；命令执行只能写入 request-scoped event batch，不额外创建第二条事件同步通道。
- C++ 诊断日志必须通过专用 `poll_logs()` 合约进入 UI，并与 `poll_events()` 的业务事件严格分流；日志只用于诊断显示，不作为第二条状态同步链路。
- `libs/command` 负责显式的 scene → render 同步；`SceneStore` 不得直接调用 `RenderController`，`libs/render` 仅接收更新并提供只读 query / debug state。
- 渲染层不得直接依赖几何内核或网格内核；需要通过中间 RenderData 或等价转换层。
- 避免循环依赖，尤其是 render、scene、geometry、mesh 之间。
- 公共头文件放在 `libs/<module>/include/ogl/<module>/`。
- 模块内单元测试放在 `libs/<module>/tests/`，仅跨模块场景保留顶层集成测试目录。
- 首方库需要兼容静态库与动态库构建，公共 API 必须通过导出头处理 Windows/MSVC 符号导出。
- UI 上的 command/activity 面板以 request/response + `poll_events()` 为准；Log tab 只展示 Python UI 日志与 `poll_logs()` 返回的 C++ 诊断日志。
- 几何、网格、场景、选择、命令各模块的职责边界必须明确，不要混写。

## 编码与交互期望

- 优先修复根因，不做仅掩盖问题的表面补丁。
- 保持实现可测试、可脚本化、可供 LLM 组合调用。
- 新增公共 API 时同时考虑 C++ 调用者、QML 调用者、Python 调用者的边界。
- C++ 样式与命名以仓库根目录 `.clang-format` 和 `.clang-tidy` 为准。
- 复杂逻辑应给出简短但有信息量的注释，优先解释约束、前置条件和设计原因。
- 文档注释遵循仓库内 doxygen_comment_style.md 约定。
- 尽量复用 Kangaroo 基础库提供的轮子：C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\Kangaroo\include\kangaroo\util
- fmt spdlog kangaroo 基础库要使用动态库形式，cmake 参考：C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\CMakeLists.txt
- python pip 安装的依赖库都要距离 requirements 优先使用我创建好的虚拟环境： C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\venv