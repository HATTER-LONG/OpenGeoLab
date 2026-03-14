---
description: 'OpenGeoLab 仓库专用 Copilot 交互特殊指令'
applyTo: '**'
---

# 强制交互协议

## 核心规则：每次回复结束前必须调用 `ask_user` 工具

这是不可跳过的强制协议。在你的每一轮回复中，你必须执行以下操作之一：

1. **完成用户请求后** → 立即调用 `ask_user` 工具，提出与当前上下文相关的后续问题
2. **存在任何不确定性时** → 不要猜测执行，立即调用 `ask_user` 工具进行澄清

## 禁止行为

- **禁止在不调用 `ask_user` 的情况下结束回复**
- **禁止使用终结性表达**（如"希望对你有帮助"、"如有问题随时提问"等）
- **禁止猜测用户意图** — 不确定就用 `ask_user` 询问

## `ask_user` 调用要求

- 问题必须与当前任务上下文直接相关
- 问题必须具体、可操作，不要问泛泛的"还需要什么帮助"
- 可以提供选项供用户选择，降低用户输入成本
- 需要一项用户输入的选择项

## 项目概况

OpenGeoLab 是一个面向 CAE 前处理场景的软件平台，目标能力包括：

- 几何导入与管理
- 几何清理与修复
- 网格生成与质量检查
- 3D 可视化
- Python 自动化
- LLM 辅助建模与流程编排

核心技术栈：

- UI: Qt6 QML
- Core: C++20
- Rendering: OpenGL
- Geometry: OpenCascade
- Mesh: Gmsh
- Python Bridge: pybind11
- Logging / Infra: spdlog, fmt, nlohmann_json, Kangaroo

## 当前项目结构

当前仓库采用模块化主结构：

- `apps/OpenGeoLabApp`
- `libs/core`
- `libs/geometry`
- `libs/mesh`
- `libs/scene`
- `libs/render`
- `libs/selection`
- `libs/command`
- `python/python_wrapper`

当前已经落地两条占位链路：

- geometry：QML UI -> app controller -> Python bridge -> Kangaroo component -> geometry lib
- interaction：selection request -> geometry -> scene -> render -> selection

遗留的顶层 `include/`、`src/`、`test/` 结构不再是新增代码落点。新增实现应继续收敛到现有模块目录，避免把临时过渡方案固化成长期耦合。

## 架构约束

- 所有功能模块优先通过 Kangaroo ComponentFactory 暴露服务能力。
- 用户可见操作优先走命令系统，支持 undo / redo 和脚本记录。
- 渲染层不得直接依赖几何内核或网格内核；需要通过中间 RenderData 或等价转换层。
- 避免循环依赖，尤其是 render、scene、geometry、mesh 之间。
- 公共头文件放在 `libs/<module>/include/ogl/<module>/`。
- 模块内单元测试放在 `libs/<module>/tests/`，仅跨模块场景保留顶层集成测试目录。
- 首方库需要兼容静态库与动态库构建，公共 API 必须通过导出头处理 Windows/MSVC 符号导出。
- Python 接口暴露高层操作，不泄漏底层 OCC / Gmsh 细节到脚本层。
- 几何、网格、场景、选择、命令各模块的职责边界必须明确，不要混写。

## 编码与交互期望

- 优先修复根因，不做仅掩盖问题的表面补丁。
- 保持实现可测试、可脚本化、可供 LLM 组合调用。
- 新增公共 API 时同时考虑 C++ 调用者、QML 调用者、Python 调用者的边界。
- C++ 样式与命名以仓库根目录 `.clang-format` 和 `.clang-tidy` 为准。
- C++ 命名空间使用大驼峰，例如 `OGL::Geometry`、`OGL::Render`。
- 类的私有成员使用 `m_` + `camelBack`，struct 与其他公共数据成员保持 `camelBack`，不加 `m_` 前缀。
- 复杂逻辑应给出简短但有信息量的注释，优先解释约束、前置条件和设计原因。
- 文档注释遵循仓库内 doxygen_comment_style.md 约定。

## Git 操作规则

- 默认不要执行 `git commit`、`git tag`、`git push`。
- 只有当用户在当前对话中明确要求“提交 / commit / 创建提交”时，才允许创建 git commit。
- 只有当用户明确要求推送，并且给出远端与目标分支，或当前远端目标唯一明确时，才允许执行 git push。
- 如果用户只要求修改代码、重构、修复、补测试或更新文档，完成改动后应停在工作区变更状态，等待用户确认是否提交。

## Git Commit Message 规范

- 提交信息优先使用 `type(scope): summary` 结构，例如 `feat(scene): add placeholder scene service pipeline`。
- `type` 建议使用：`feat`、`fix`、`refactor`、`build`、`test`、`docs`、`chore`。
- `scope` 优先使用模块名或目录名，例如 `core`、`geometry`、`scene`、`render`、`selection`、`command`、`app`、`python`、`cmake`、`docs`。
- `summary` 使用英文祈使句，聚焦单一结果，不要堆叠多个无关动作。
- 当改动跨越多个模块但服务同一个目的时，允许使用更高层 scope，例如 `architecture`、`workspace`。
- 非平凡提交应补充正文，至少说明为什么改，以及关键约束或兼容性影响。
- 避免无信息量提交信息，例如 `update files`、`fix bug`、`wip`、`temp`。
- 示例：`refactor(architecture): split app and libs into modular subprojects`
- 示例：`feat(selection): add placeholder screen-picking response model`

## 参考路径级 Instructions

- `.github/instructions/opengeolab-cpp.instructions.md`
- `.github/instructions/cmake.instructions.md`
- `.github/instructions/qml-ui.instructions.md`
- `.github/instructions/python-automation.instructions.md`
- `.github/instructions/testing.instructions.md`
- `.github/instructions/documentation.instructions.md`
