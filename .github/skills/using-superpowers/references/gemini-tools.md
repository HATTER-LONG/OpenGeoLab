# 旧 Gemini 术语兼容表

> 说明：本仓库当前目标平台是 GitHub Copilot。这个文件保留原文件名，仅用于把历史文档里的 Gemini 相关术语映射到 Copilot 语境。

## 兼容映射

| 历史术语 | 在 Copilot 中应理解为 |
|----------|----------------------|
| `activate_skill` | 读取并启用对应的 `SKILL.md` |
| `write_todos` | 建立可跟踪任务清单 |
| `read_file` / `write_file` / `replace` | 使用 Copilot 的文件读取与编辑能力 |
| `run_shell_command` | 使用终端命令能力 |
| `ask_user` | 向用户提出澄清问题 |
| Gemini 中无子代理能力的说法 | 在 Copilot 中按是否支持子代理决定使用 `subagent-driven-development` 还是 `executing-plans` |

## 使用规则

如果历史文档提到了 Gemini CLI 的工具名或限制，请把它解释成 Copilot 下对应的能力和约束，而不要继续沿用 Gemini 的平台假设。

## 最底线

这个仓库不再以 Gemini CLI 为主目标平台。

如果历史文档出现 Gemini 术语，统一按 Copilot 兼容语义理解。