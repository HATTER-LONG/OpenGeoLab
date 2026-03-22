# QML Action & Style Refactoring — Implementation Plan

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 统一 Action 配置、精简 signal 链、收敛样式到 AppTheme 颜色集，外观像素级不变。

**架构：** 自底向上改造——先扩展 AppTheme 颜色集，再创建 MenuConfig 数据源，然后改造叶组件（ActionButton/RibbonTile），最后改造容器组件（HeaderRibbonGroup → HeaderMenuPanel → AppHeader → Main）。每一步构建验证，最终全量回归。

**技术栈：** Qt 6 QML, CMake, Ninja

**规格文档：** `docs/superpowers/specs/2026-03-22-qml-action-refactor-design.md`

**TDD 说明：** 项目无 QML 测试基础设施（CTest 无已注册测试），无法 TDD。每个任务使用构建验证 + 手动验证替代。

---

## 文件地图

| 文件 | 动作 | 职责 |
|------|------|------|
| `src/app/resource/qml/theme/AppTheme.qml` | 修改 | 新增 ribbonTile/panel 颜色集 + actionButtonColors/accentHoverBorder 函数 |
| `src/app/resource/qml/MenuConfig.qml` | 新建 | 菜单面板 action 数据源（sections/actions/accent/alphaScale/hoverAccent） |
| `src/app/resource/qml/components/ActionButton.qml` | 修改 | 删除 6 个颜色属性 + hover overlay，改用 colorSet + hoverBorderOverride + actionHandler |
| `src/app/resource/qml/components/RibbonTile.qml` | 修改 | 删除 signal clicked，改用 actionHandler + theme.ribbonTile 颜色集 |
| `src/app/resource/qml/sections/HeaderRibbonGroup.qml` | 修改 | 删除 signal triggerAction，改用 required property var actionHandler |
| `src/app/resource/qml/sections/HeaderMenuPanel.qml` | 重写 | 数据驱动 Repeater 从 MenuConfig，删除 signal triggerAction/requestThemeToggle |
| `src/app/resource/qml/sections/AppHeader.qml` | 修改 | 删除 signal triggerAction/requestThemeToggle，新增 required property var actionHandler |
| `src/app/resource/qml/Main.qml` | 修改 | 传入 openActionPage 函数引用，移入 TranslationManager.switchLanguage() |
| `src/app/CMakeLists.txt` | 修改 | QML_FILES 添加 MenuConfig.qml |

**稳定边界（不得触碰）：** SidebarPanel, ViewportPanel, ActivityOverlay, ActivityPanel, LogEventsView, TerminalView, StatChip, LogLevelChip, AppIcon, RibbonConfig

---

### 任务 1：扩展 AppTheme 颜色集

**文件：**
- 修改：`src/app/resource/qml/theme/AppTheme.qml`

- [ ] 步骤 1：在 `accentByName()` 函数后添加 `ribbonTile` QtObject 颜色集，包含 5 个属性：normal, hovered, pressed, borderNormal, iconBg。值从规格 §4.2 精确复制。
- [ ] 步骤 2：添加 `panel` QtObject 颜色集，包含 10 个属性：normal, border, tabBarBorder, tabActiveBg, tabActiveBorder, tabHovered, menuBg, menuBorder, menuRecorderBg, separator。
- [ ] 步骤 3：添加 `actionButtonColors(accentName, alphaScale)` 函数，返回 `{ normal, pressed }`。muted 分支使用 0.18/0.1 和 0.28/0.16，normal 分支使用 0.2/0.11 和 0.3/0.18。
- [ ] 步骤 4：添加 `accentHoverBorder(accentName)` 函数，返回 `tint(accent, darkMode ? 0.58 : 0.34)`。
- [ ] 步骤 5：构建验证 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 6：提交 `refactor(theme): add pre-computed color sets to AppTheme`

**参考：** 规格 §4.2，当前 RibbonTile.qml 行 20-22（tint 值来源），HeaderMenuPanel.qml 行 78-80/100-101/112-113（alpha 差异来源）

---

### 任务 2：创建 MenuConfig.qml

**文件：**
- 新建：`src/app/resource/qml/MenuConfig.qml`
- 修改：`src/app/CMakeLists.txt`

- [ ] 步骤 1：创建 `MenuConfig.qml`，使用 `pragma ComponentBehavior: Bound` + `import QtQml`，定义 `readonly property var sections` 数组。
  - Workspace section：accent "accentA"，4 个 actions（importModel, exportModel, toggleTheme with dynamic+muted+hoverAccent:accentA, switchLanguage with dynamic+accent:accentE+muted+hoverAccent:accentE）
  - Script Recorder section：accent "accentB"，4 个 actions（recordSelection, replayCommands, exportScript, clearRecordedCommands with hoverAccent:accentD）
- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 QML_FILES 列表中添加 `resource/qml/MenuConfig.qml`
- [ ] 步骤 3：构建验证
- [ ] 步骤 4：提交 `feat(app): add MenuConfig for data-driven menu panel`

**参考：** 规格 §4.1，RibbonConfig.qml 结构对照

---

### 任务 3：简化 ActionButton.qml

**文件：**
- 修改：`src/app/resource/qml/components/ActionButton.qml`

- [ ] 步骤 1：删除属性 `buttonColor`, `pressedColor`, `iconSecondaryColor`, `hoverBorderColor`。删除 `signal clicked`。
- [ ] 步骤 2：新增 `property var colorSet: ({})`, `required property var actionHandler`, `property string actionKey: ""`, `property color hoverBorderOverride: "transparent"`。
- [ ] 步骤 3：删除 hover overlay Rectangle（当前行 31-36，使用 iconSecondaryColor 的那个）。
- [ ] 步骤 4：更新 `color:` 绑定为 `mouseArea.pressed ? colorSet.pressed : (mouseArea.containsMouse ? theme.tint(colorSet.normal, quiet ? 0.92 : 1.0) : colorSet.normal)`
- [ ] 步骤 5：更新 `border.color:` 绑定为 `mouseArea.containsMouse ? (hoverBorderOverride.a > 0 ? hoverBorderOverride : theme.tint(theme.textPrimary, theme.darkMode ? 0.52 : 0.3)) : (quiet ? theme.tint(theme.borderSubtle, 0.45) : theme.borderSubtle)`
- [ ] 步骤 6：更新 MouseArea `onClicked` 为 `control.actionHandler(control.actionKey)`
- [ ] 步骤 7：构建验证（此时 HeaderMenuPanel 会报错——预期行为，因为调用方还没改）
- [ ] 步骤 8：**不单独提交**——与任务 6 一起提交（HeaderMenuPanel 依赖此改动）

**参考：** 规格 §4.4，当前 ActionButton.qml 完整代码

---

### 任务 4：重构 RibbonTile.qml

**文件：**
- 修改：`src/app/resource/qml/components/RibbonTile.qml`

- [ ] 步骤 1：删除 `signal clicked`，新增 `required property var actionHandler`, `property string actionKey: ""`。
- [ ] 步骤 2：将 accent-independent 颜色替换为 theme.ribbonTile 引用：
  - `color:` → `mouseArea.pressed ? theme.ribbonTile.pressed : (mouseArea.containsMouse ? theme.ribbonTile.hovered : theme.ribbonTile.normal)`
  - `border.color` (非 hover 分支) → `theme.ribbonTile.borderNormal`
  - icon 背景 Rectangle `color:` → `theme.ribbonTile.iconBg`
- [ ] 步骤 3：保留 accent-dependent 的 tint() 调用不变（gradient stops, icon border `tint(accentOne, ...)`, border hover `tint(accentOne, ...)`）
- [ ] 步骤 4：更新 MouseArea `onClicked` 为 `tile.actionHandler(tile.actionKey)`
- [ ] 步骤 5：构建验证（此时 HeaderRibbonGroup 会报错——预期行为）
- [ ] 步骤 6：**不单独提交**——与任务 5 一起提交

**参考：** 规格 §4.3，当前 RibbonTile.qml 完整代码

---

### 任务 5：简化 HeaderRibbonGroup.qml

**文件：**
- 修改：`src/app/resource/qml/sections/HeaderRibbonGroup.qml`

- [ ] 步骤 1：删除 `signal triggerAction(string actionKey)`，新增 `required property var actionHandler`。
- [ ] 步骤 2：更新 RibbonTile delegate：
  - 删除 `onClicked: groupRoot.triggerAction(modelData.key)`
  - 新增 `actionKey: modelData.key`
  - 新增 `actionHandler: groupRoot.actionHandler`
- [ ] 步骤 3：构建验证（此时 AppHeader 会报错——预期行为）
- [ ] 步骤 4：**不单独提交**——与任务 7 一起提交

**参考：** 规格 §4.6，当前 HeaderRibbonGroup.qml 完整代码

---

### 任务 6：重写 HeaderMenuPanel.qml

**文件：**
- 修改：`src/app/resource/qml/sections/HeaderMenuPanel.qml`

- [ ] 步骤 1：删除 `signal requestThemeToggle`, `signal triggerAction(string actionKey)`。新增 `required property var actionHandler`。
- [ ] 步骤 2：在文件顶部区域实例化 `MenuConfig { id: menuConfig }`。MenuConfig.qml 位于上级目录 `resource/qml/`，同属 `OpenGeoLab.App` QML module，类型应自动可见；若构建时类型解析失败，在文件顶部添加 `import ".."`。
- [ ] 步骤 3：删除所有硬编码 ActionButton 实例（8 个）和手动 Column/section 结构。
- [ ] 步骤 4：用双层 Repeater 替代：外层遍历 `menuConfig.sections`，内层遍历 `section.actions`。
  - 每个 section 有 Text 标题 + Rectangle 容器（bg 颜色用 `theme.panel.menuBg` 或根据 section index 用 `menuRecorderBg`）
  - sections 之间用 separator Rectangle（`theme.panel.separator`）
- [ ] 步骤 5：ActionButton delegate 设置：
  - `colorSet: theme.actionButtonColors(effectiveAccent, effectiveAlpha)`
  - `hoverBorderOverride: modelData.hoverAccent ? theme.accentHoverBorder(modelData.hoverAccent) : "transparent"`
  - `actionHandler: panel.actionHandler`
  - `actionKey: modelData.key`
  - `leftAligned: true`
  - `width: parent.width`
- [ ] 步骤 6：处理 dynamic 按钮的 `buttonText` 和 `iconKind` 绑定（toggleTheme/switchLanguage 特殊逻辑）
- [ ] 步骤 7：保留 panel 动画（Behavior on opacity/scale）和基本容器样式不变
- [ ] 步骤 8：构建验证（此时 AppHeader 会报错——预期行为，triggerAction/requestThemeToggle 还在用）
- [ ] 步骤 9：**不单独提交**——与任务 7 一起提交

**参考：** 规格 §4.5，当前 HeaderMenuPanel.qml 完整代码

---

### 任务 7：简化 AppHeader.qml

**文件：**
- 修改：`src/app/resource/qml/sections/AppHeader.qml`

- [ ] 步骤 1：删除 `signal triggerAction(string actionKey)`, `signal requestThemeToggle`。新增 `required property var actionHandler`。
- [ ] 步骤 2：更新 tab bar Rectangle border.color 为 `theme.panel.tabBarBorder`。
- [ ] 步骤 3：更新 tab bar Rectangle color 为 `theme.panel.normal`。
- [ ] 步骤 4：更新 tab 按钮颜色：active bg → `theme.panel.tabActiveBg`, active border → `theme.panel.tabActiveBorder`, hovered → `theme.panel.tabHovered`。
- [ ] 步骤 5：更新 ribbon panel Rectangle color/border 为 `theme.panel.normal`/`theme.panel.border`。
- [ ] 步骤 6：更新 HeaderRibbonGroup 实例化：
  - 删除 `onTriggerAction: function(actionKey) { header.triggerAction(actionKey) }`
  - 新增 `actionHandler: header.actionHandler`
- [ ] 步骤 7：更新 HeaderMenuPanel 实例化：
  - 删除 `onRequestThemeToggle: header.requestThemeToggle()`
  - 删除 `onTriggerAction: function(actionKey) { header.triggerAction(actionKey) }`
  - 新增 `actionHandler: header.actionHandler`
- [ ] 步骤 8：构建验证（此时 Main.qml 会报错——预期行为）
- [ ] 步骤 9：**不单独提交**——与任务 8 一起提交

**参考：** 规格 §4.7，当前 AppHeader.qml 完整代码

---

### 任务 8：更新 Main.qml + 全量提交

**文件：**
- 修改：`src/app/resource/qml/Main.qml`

- [ ] 步骤 1：更新 AppHeader 实例化：
  - 删除 `onRequestThemeToggle: root.toggleTheme()`
  - 删除 `onTriggerAction: function(actionKey) { root.openActionPage(actionKey) }`
  - 新增 `actionHandler: root.openActionPage`
- [ ] 步骤 2：在 `openActionPage` 函数的 switchLanguage 分支中添加 `TranslationManager.switchLanguage()` 调用（从 HeaderMenuPanel 移入）：
  ```qml
  if (actionKey === "switchLanguage") {
      TranslationManager.switchLanguage(
          TranslationManager.currentLanguage === "zh_CN" ? "en_US" : "zh_CN");
      root.statusNote = TranslationManager.currentLanguage === "zh_CN"
          ? qsTr("Switched to Chinese.") : qsTr("Switched to English.");
      root.menuOpen = false;
      return;
  }
  ```
- [ ] 步骤 3：~~在 Main.qml 中实例化 MenuConfig~~——**跳过**，MenuConfig 已在任务 6 由 HeaderMenuPanel 内部实例化。
- [ ] 步骤 4：构建验证 `cmake --build build --config RelWithDebInfo --parallel 4` — **必须通过**
- [ ] 步骤 5：全量回归 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 6：提交全部改动（任务 3-8）`refactor(qml): unify action system with actionHandler callbacks and theme color sets`

**参考：** 规格 §4.8，当前 Main.qml 完整代码

---

### 任务 9：手动视觉验证清单

此任务不修改代码，仅做验证。

- [ ] 步骤 1：启动程序，确认主窗口正常显示
- [ ] 步骤 2：点击 Geometry/Mesh/AI tab 切换，确认 ribbon 区域正确刷新
- [ ] 步骤 3：点击每个 ribbon 按钮，确认 console 输出 `[Main] openActionPage: <key>`
- [ ] 步骤 4：打开菜单面板，确认 8 个按钮全部显示，样式与改造前一致
- [ ] 步骤 5：确认 Import/Export/Record/Replay/ExportRecord 的 hover 边框是中性灰
- [ ] 步骤 6：确认 toggleTheme/switchLanguage 的 hover 边框是彩色（accentA/accentE）
- [ ] 步骤 7：确认 clearRecordedCommands 的 hover 边框是橙红色（accentD）
- [ ] 步骤 8：点击 toggleTheme → 确认深浅模式切换正常
- [ ] 步骤 9：点击 switchLanguage → 确认中英文切换正常，所有标签更新
- [ ] 步骤 10：确认 Activity Panel 无回归
- [ ] 步骤 11：如有视觉差异，记录并修复后补提交

---

## 执行注意事项

1. **任务 1-2 可独立提交**，因为新增颜色集和 MenuConfig 不会影响现有代码。
2. **任务 3-8 必须作为一个原子提交**，因为组件接口变化是联动的（ActionButton 删除 clicked signal 后 HeaderMenuPanel 必须同步更新）。
3. **构建验证**：任务 3-7 的中间状态可能编译失败（预期行为）。任务 8 完成后全量构建必须通过。
4. **section 背景色区分**：Workspace section 用 `theme.panel.menuBg`，Script Recorder 用 `theme.panel.menuRecorderBg`。可通过 section index 判断（index === 0 → menuBg，其他 → menuRecorderBg）。
