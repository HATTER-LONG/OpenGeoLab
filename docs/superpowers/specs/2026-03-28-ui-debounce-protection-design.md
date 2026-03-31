# UI Debounce Protection Design

## 问题陈述

当用户在 FunctionPageBase 中点击 Execute 按钮后，`RequestService.submitAsync()` 立即返回，页面立即关闭。在请求完成前，用户可以重新打开页面并再次点击 Execute，导致重复请求并发执行。

当前 `RequestService` 已有 `Q_PROPERTY(bool busy)` 和 `busyChanged()` 信号，但 QML 层完全没有使用。

## 方案选择

**选定：方案 A — 纯 QML 绑定**

利用已有的 `RequestService.busy` 属性，仅在 QML 侧添加绑定和视觉反馈，不修改 C++ 层。

理由：
- 改动范围小（仅 FunctionPageBase.qml）
- 不涉及 C++ 变更，风险低
- 全局 busy 粒度已满足防误操作需求
- 后续做后端串行化时可升级为细粒度方案

## 行为规格

### 1. Execute 按钮保护

**触发条件**：`RequestService.busy === true`

**行为**：
- `execute()` 函数开头加 `if (RequestService.busy) return;` guard
- Execute 按钮 MouseArea 设置 `enabled: !RequestService.busy`
- 按钮视觉：opacity 降至 0.5，颜色变为 `surfaceMuted`（灰态）
- 光标变为默认箭头（非 PointingHandCursor）

**约束**：
- Cancel 按钮不受影响，始终可用
- 页面可正常关闭（close() 不受 busy 限制）

### 2. Busy Banner 提示

**触发条件**：页面打开时 `RequestService.busy === true`

**位置**：panelColumn 内，titleBar 分隔线下方，contentArea 上方

**行为**：
- 高度 28px，背景色 `theme.tint(theme.warning, darkMode ? 0.18 : 0.10)`
- 居中文字 `qsTr("A task is running…")`，字色 `theme.warning`，字号 11px
- 带高度动画（150ms）平滑展开/收起
- `RequestService.busy` 变为 false 时自动消失

**约束**：
- 不阻塞页面内容浏览和参数编辑
- 不影响 contentArea 的 Flickable 滚动

### 3. 工具栏按钮

**行为**：不做任何变化。用户可自由打开页面浏览参数。

**理由**：
- 方案 A 无 actionId 粒度，无法标记具体哪个按钮对应的请求正在执行
- 页面内 Execute 按钮 + Banner 已提供足够保护
- 避免工具栏大面积置灰影响操作流畅感

### 4. 页面打开行为

**行为**：允许打开任何页面，不受 busy 限制。

- 如果 busy，页面打开后自动显示 Busy Banner
- 用户可编辑参数，但无法点击 Execute
- 任务完成后 Banner 消失，Execute 按钮恢复可用

## 涉及文件

| 文件 | 变更类型 |
|------|---------|
| `src/app/resource/qml/components/FunctionPageBase.qml` | 修改 execute() + executeButton + 新增 busyBanner |

仅修改 1 个文件。

## 不在本次范围

- C++ RequestService 层的变更
- 工具栏按钮视觉状态
- 细粒度 actionId 追踪
- 后端请求串行化（后续独立任务）
- 多请求并发队列管理

## 测试策略

- 手动验证：连续快速双击 Execute 按钮，确认只发送一次请求
- 手动验证：请求执行中打开其他页面，确认 Banner 显示且 Execute 禁用
- 手动验证：请求完成后，Execute 按钮恢复可用，Banner 消失
- 构建验证：`cmake --build build --config RelWithDebInfo --parallel 4`
- 测试验证：`ctest --test-dir build -C RelWithDebInfo --output-on-failure`
