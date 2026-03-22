---
name: using-git-worktrees
description: 用于开始需要隔离工作区的功能开发，或在执行实现计划前先创建独立 git worktree，并完成目录选择与安全校验
---

# 使用 Git Worktree

## 概述

Git worktree 可以在同一个仓库上同时维护多个独立工作目录，避免频繁切分支污染当前环境。

**核心原则：** 目录选择要稳定，安全校验不能跳过。

## 模型选择

本技能用于开发准备阶段，统一使用 **GPT-5.4**。

## 目录选择顺序

按以下优先级处理：

### 1. 先看仓库里是否已有约定目录

优先检查：

- .worktrees/
- worktrees/

如果两个都存在，优先使用 .worktrees/。

### 2. 再看仓库指令文件

检查 copilot-instructions.md、AGENTS.md 或其他同类约束中是否有 worktree 目录偏好。

如果已经明确指定，就直接遵循，不要再问用户。

### 3. 最后再问用户

如果前两步都没有结果，再询问用户使用哪种目录：

1. .worktrees/，项目内隐藏目录
2. 全局目录，例如用户级 worktree 存放位置

## 安全校验

### 项目内目录

如果使用 .worktrees/ 或 worktrees/，创建前必须确认该目录被 git 忽略。

如果没有被忽略：

1. 先补 .gitignore
2. 提交这项修复
3. 再继续创建 worktree

原因：否则 worktree 内容可能污染仓库状态。

### 项目外全局目录

如果 worktree 位于仓库外，一般不需要 .gitignore 校验。

## 创建步骤

1. 识别项目名
2. 计算目标路径
3. 创建新分支并创建 worktree
4. 进入新目录
5. 自动检测并运行项目初始化命令
6. 执行基线验证，确认新 worktree 处于可工作的干净状态

## 项目初始化

根据仓库文件自动判断应运行哪些命令，例如：

- package.json -> 安装 Node.js 依赖
- Cargo.toml -> 构建或下载 Rust 依赖
- requirements.txt 或 pyproject.toml -> 安装 Python 依赖
- go.mod -> 下载 Go 依赖

不要把初始化命令写死成单一技术栈。

## 基线验证

创建并初始化后，运行项目适配的测试或验证命令，确认当前分支起点是干净的。

如果测试失败：

- 明确汇报失败情况
- 询问用户是继续带病开发，还是先排查基线问题

如果测试通过：

- 报告 worktree 路径
- 报告基线验证通过
- 再开始实现

## 常见错误

- 没确认目录已被忽略就创建项目内 worktree
- 目录约定不清时擅自决定路径
- 基线测试失败还继续推进
- 初始化命令写死，导致跨项目失效

## 集成关系

通常在以下技能前使用：

- superpowers:subagent-driven-development
- superpowers:executing-plans
- 任何需要隔离工作区的实现流程

完成开发后通常配合：

- superpowers:finishing-a-development-branch
