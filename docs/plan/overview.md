
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

- `apps/OpenGeoLabApp`： pyside6 + qml 实现界面相关，自动 record 界面操作的所有 py 命令，支持 replay 回放操作
- `python/python_wrapper`: 使用 pybind11 封装 libs 模块提供的 c++ 接口
- `libs/command` ： 统一入口 路由任务
- `libs/core` : 公共定义，核心方法，配置信息等等
- `libs/geometry`：几何
- `libs/mesh`：网格
- `libs/render`：渲染相关
- `libs/selection`：拾取器相关
- `etc` ： 按照设计可以自己重定义 libs

┌────────────────────────────┐
│        UI Layer            │
│  QML / PySide6 / Toolbar  │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Interaction Layer      │
│  - Command Recorder        │
│  - Input Mapper (UI → Cmd) │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Python Adapter         │  ← pybind11
│  - execute_command(json)   │
│  - subscribe_event()       │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│       Command Bus          │ ⭐核心
│  - validate               │
│  - dispatch               │
│  - middleware             │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│       Orchestrator         │
│  - workflow                │
│  - undo/redo               │
│  - macro / batch           │
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
│ geometry / mesh / render   │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│     Scene / Data Model     │ ⭐唯一状态源
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│       RenderData           │
└────────────┬───────────────┘
             │
┌────────────▼───────────────┐
│         OpenGL             │
└────────────────────────────┘

## 架构约束
- 所有用户操作必须转化为 Command，所有模块间通信必须通过 Service Router，所有状态必须存储在 Scene Model， 渲染层只依赖 RenderData，不依赖 Geometry/Mesh，LLM 只能调用 Command，不允许直接调用底层模块
- 所有功能模块优先通过 Kangaroo ComponentFactory 暴露 service 与 action factory；也就是不同 module 通过 module name 对外提供统一组件接口，内部通过 action name 调用对应功能。
- service 请求统一使用 `{ module, action, param }` JSON 信封，`param` 必须是 object。
- 用户可见操作优先走命令系统，例如旋转模型，鼠标拾取等等，但是要避免重复记录，只记录必要的。
- 渲染层不得直接依赖几何内核或网格内核；需要通过中间 RenderData 或等价转换层。
- 避免循环依赖，尤其是 render、scene、geometry、mesh 之间。
- 公共头文件放在 `libs/<module>/include/ogl/<module>/`。
- 模块内单元测试放在 `libs/<module>/tests/`，仅跨模块场景保留顶层集成测试目录。
- 首方库需要兼容静态库与动态库构建，公共 API 必须通过导出头处理 Windows/MSVC 符号导出。
- UI 上要支持 log 显示，通过 spdlog， Activity / operation log 统一走 module logger + QML sink 管线，不要绕开 `ModuleLogger` / `AppLogger` 自建散乱日志出口。
- 几何、网格、场景、选择、命令各模块的职责边界必须明确，不要混写。

## 编码与交互期望

- 优先修复根因，不做仅掩盖问题的表面补丁。
- 保持实现可测试、可脚本化、可供 LLM 组合调用。
- 新增公共 API 时同时考虑 C++ 调用者、QML 调用者、Python 调用者的边界。
- C++ 样式与命名以仓库根目录 `.clang-format` 和 `.clang-tidy` 为准。
- C++ 命名空间使用大驼峰。
- 类的私有成员使用 `m_` + `camelBack`，struct 与其他公共数据成员保持 `camelBack`，不加 `m_` 前缀。
- 复杂逻辑应给出简短但有信息量的注释，优先解释约束、前置条件和设计原因。
- 文档注释遵循仓库内 doxygen_comment_style.md 约定。

## 界面设计

- 主界面分为三大块：
    - Header Ribbon 风格，ribbon tab 最左侧增加一个面包菜单 icon 支持弹出一个 popmenu
        - popmenu 包括：
            - 工程操作：new model、import model、export model
            - App 设置：switch light/dark、switch language、setting
            - setting 可以弹出一个模态的设置页面，当前空实现占位即可
        - Ribbon 分为三个 tab：
            - geometry：
                - create 组：支持创建 box、圆柱、圆环、球等基础模型
                - 修改组：支持 split、offset 等
                - 信息组：支持 query，通过鼠标拾取不同数据可以显示几何信息
            - mesh：
                - 剖分：surface mesh、3D mesh
                - 优化：网格自动优化，网格质量评估
                - 信息组：支持query
            - AI：
                - LLM 支持：操作建议，Chat 等等
    - 左侧 side bar，用于显示当前模型的信息，包括有多少个 part 组成，每个part 包含的元素，控制每个 part 的几何 mesh 显示隐藏
    - 右侧主窗口，显示 opengl 渲染显示信息
- 其他细节：
    - 在主窗口，渲染显示信息右上角有个toolbar 支持fit view 、切换各个视角、几何显示切换按钮（X Ray(透视模式，可以穿透拾取)、线框模式不显示三角面片只显示 点、线基础几何，离散信息模式显示三角面片的同时高亮显示出三角面片组成的边及点），网格显示切换按钮（默认渲染网格element面片、网格element edge、网格 element node，切换显示 element edge + node 不显示面片，x ray 模式）
    - 主窗口右下角增加一个 button，可以打开一个窗口非模态窗口，有两个 tab，一个显示 操作日志，可以过滤显示日志等级以及设计 spdlog 的日志等级，以及 python UI 自身的日志信息 另一个 tab 时 command line 可以显示每次操作记录的request 与回复，并且支持手动输入命令进行操作