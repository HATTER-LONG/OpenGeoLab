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
- **禁止自行执行 `git commit`** — 所有 commit 操作必须先通过 `ask_user` 询问用户确认后才能执行

## `ask_user` 调用要求

- 你必须要给我一项用户输入的选择项
- 问题必须与当前任务上下文直接相关
- 问题必须具体、可操作，不要问泛泛的"还需要什么帮助"
- 可以提供选项供用户选择，降低用户输入成本

## Git Commit Message 规范

- 提交信息优先使用 `type(scope): summary` 结构，例如 `feat(scene): add placeholder scene service pipeline`。
- `type` 建议使用：`feat`、`fix`、`refactor`、`build`、`test`、`docs`、`chore`。
- `scope` 优先使用模块名或目录名，例如 `core`、`geometry`、`scene`、`render`、`selection`、`command`、`app`、`python`、`cmake`、`docs`。
- `summary` 使用英文祈使句，聚焦单一结果，不要堆叠多个无关动作。
- 当改动跨越多个模块但服务同一个目的时，允许使用更高层 scope，例如 `architecture`、`workspace`。
- 非平凡提交应补充正文，至少说明为什么改，以及关键约束或兼容性影响。
- 避免无信息量提交信息，例如 `update files`、`fix bug`、`wip`、`temp`。
- 示例：`refactor(architecture): split app and libs into modular subprojects`
- 示例：`feat(selection): add pick and box-select action components`

## 构建配置

- 优先使用 Ninja 作为 CMake 生成器：`cmake -S . -B build -G Ninja`
- 默认构建类型为 RelWithDebInfo
- 构建命令：`cmake --build build --parallel $(nproc)`
- 测试命令：`ctest --test-dir build --output-on-failure`
