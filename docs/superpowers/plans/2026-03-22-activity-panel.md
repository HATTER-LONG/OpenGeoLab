# Activity Log & Command Panel 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 在主窗口右下角添加浮动 Activity 面板，包含 Events 日志查看 + Command Line 终端 + Progress 进度条，全部使用 QML mock 数据。

**架构：** 5 个新 QML 文件分三层组织 — ActivityOverlay（sections/容器层）→ ActivityPanel（components/壳层）→ LogEventsView + TerminalView + LogLevelChip（components/内容层）。8 个 ionicons outline SVG 提供图标。纯 QML ListModel mock，不涉及 C++。

**技术栈：** Qt 6 QML, QtQuick Layouts, ionicons 8.0, CMake, lupdate/lrelease

---

## 文件清单

### 新增文件

| 文件路径 | 职责 |
|---------|------|
| `src/app/resource/icons/pulse.svg` | Activity 按钮图标 |
| `src/app/resource/icons/list.svg` | Events 标签图标 |
| `src/app/resource/icons/terminal.svg` | Command Line 标签图标 |
| `src/app/resource/icons/closePanel.svg` | 面板关闭按钮图标 |
| `src/app/resource/icons/trash.svg` | 清除日志/终端图标 |
| `src/app/resource/icons/funnel.svg` | 级别筛选切换图标 |
| `src/app/resource/icons/chevronDown.svg` | 筛选展开/收起图标 |
| `src/app/resource/icons/play.svg` | 运行命令按钮图标 |
| `src/app/resource/qml/components/LogLevelChip.qml` | 日志级别筛选标签 |
| `src/app/resource/qml/components/TerminalView.qml` | Command Line 标签内容 |
| `src/app/resource/qml/components/LogEventsView.qml` | Events 标签内容 |
| `src/app/resource/qml/components/ActivityPanel.qml` | 主面板壳（标签切换 + mock model） |
| `src/app/resource/qml/sections/ActivityOverlay.qml` | 右下角容器 + Activity 按钮 + 进度条 |

### 修改文件

| 文件路径 | 变更 |
|---------|------|
| `src/app/CMakeLists.txt` | RESOURCES 添加 8 个 SVG，QML_FILES 添加 5 个 QML |
| `src/app/resource/qml/Main.qml` | 在内容区域添加 ActivityOverlay |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | 添加新组件的中文翻译 |

---

## 任务 1：拷贝 ionicons SVG 并注册到 CMake

**文件：**
- 新增：`src/app/resource/icons/{pulse,list,terminal,closePanel,trash,funnel,chevronDown,play}.svg`
- 修改：`src/app/CMakeLists.txt`

**参考：** 规格 §4（Icons — Ionicons Outline）

- [ ] 步骤 1：从 `D:\WorkSpace\OGLWorkSpace\ionicons-8.0.13\src\svg\` 拷贝 8 个 outline SVG 到 `src/app/resource/icons/`，按规格重命名为 camelCase

```powershell
$src = "D:\WorkSpace\OGLWorkSpace\ionicons-8.0.13\src\svg"
$dst = "D:\WorkSpace\OGLWorkSpace\OpenGeoLabBack\src\app\resource\icons"
$map = @{
    "pulse-outline.svg"        = "pulse.svg"
    "list-outline.svg"         = "list.svg"
    "terminal-outline.svg"     = "terminal.svg"
    "close-outline.svg"        = "closePanel.svg"
    "trash-outline.svg"        = "trash.svg"
    "funnel-outline.svg"       = "funnel.svg"
    "chevron-down-outline.svg" = "chevronDown.svg"
    "play-outline.svg"         = "play.svg"
}
foreach ($k in $map.Keys) { Copy-Item "$src\$k" "$dst\$($map[$k])" }
```

- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 RESOURCES 块末尾添加 8 行。当前最后一行是 `resource/icons/clear.svg)`，需先去掉右括号，追加新行后在最后一行加回右括号：

```cmake
    resource/icons/clear.svg
    resource/icons/pulse.svg
    resource/icons/list.svg
    resource/icons/terminal.svg
    resource/icons/closePanel.svg
    resource/icons/trash.svg
    resource/icons/funnel.svg
    resource/icons/chevronDown.svg
    resource/icons/play.svg)
```

- [ ] 步骤 3：验证构建配置

```powershell
cmake -S . -B build
```

预期：CMake 配置成功，无文件缺失错误。

- [ ] 步骤 4：提交

```
feat(icons): add ionicons outline SVGs for activity panel
```

---

## 任务 2：创建 LogLevelChip.qml

**文件：**
- 新增：`src/app/resource/qml/components/LogLevelChip.qml`
- 修改：`src/app/CMakeLists.txt`（QML_FILES 添加一行）

**参考：** 规格 §3.4

- [ ] 步骤 1：创建 `LogLevelChip.qml`

组件接口：
```qml
pragma ComponentBehavior: Bound
import QtQuick
import "../theme"

Rectangle {
    id: chip
    required property AppTheme theme
    property string text: ""
    property color accentColor: theme.accentA
    property bool selected: false
    signal clicked
    // ...
}
```

实现要点：
- `implicitWidth: label.implicitWidth + 22`，`implicitHeight: 28`，`radius: 9`
- 选中态：`theme.tint(accentColor, darkMode ? 0.24 : 0.14)` 背景 + accent 边框 + bold 文字
- 未选中态：`theme.tint(theme.surface, darkMode ? 0.48 : 0.94)` 背景 + subtle 边框 + secondary 文字
- MouseArea with `cursorShape: Qt.PointingHandCursor`

- [ ] 步骤 2：在 CMakeLists.txt QML_FILES 中添加 `resource/qml/components/LogLevelChip.qml`

- [ ] 步骤 3：验证编译

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 4：提交

```
feat(qml): add LogLevelChip component for log level filtering
```

---

## 任务 3：创建 TerminalView.qml

**文件：**
- 新增：`src/app/resource/qml/components/TerminalView.qml`
- 修改：`src/app/CMakeLists.txt`（QML_FILES 添加一行）

**参考：** 规格 §3.5

- [ ] 步骤 1：创建 `TerminalView.qml`

组件接口：
```qml
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    required property AppTheme theme
    required property var model   // ListModel { role: type, text }
    signal commandSubmitted(string text)
    // ...
}
```

实现要点：
- **输出区域：** ListView/Flickable 显示终端条目，每条用 Text 渲染
  - `"command"` 条目前缀 `>>>` 颜色 `theme.success`
  - `"response"` 条目前缀 `<<<` 颜色 `theme.warning`
  - `"error"` 条目前缀 `!!!` 颜色 `theme.danger`
  - 字体：`theme.monoFontFamily`
- **滚动条：** 6px 宽，surfaceStrong thumb
- **输入区域：** TextEdit + Run 按钮（play 图标）
  - WrapAnywhere，最小 58px 高，最大 156px
  - 空态 placeholder: `qsTr("Type a command...")`
  - Ctrl+Enter 或点击 Run → emit `commandSubmitted(text)` + 清空输入
- 背景：`theme.tint(theme.surface, darkMode ? 0.5 : 1.0)`

- [ ] 步骤 2：在 CMakeLists.txt QML_FILES 中添加 `resource/qml/components/TerminalView.qml`

- [ ] 步骤 3：验证编译

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 4：提交

```
feat(qml): add TerminalView component for command line tab
```

---

## 任务 4：创建 LogEventsView.qml

**文件：**
- 新增：`src/app/resource/qml/components/LogEventsView.qml`
- 修改：`src/app/CMakeLists.txt`（QML_FILES 添加一行）

**参考：** 规格 §3.3

- [ ] 步骤 1：创建 `LogEventsView.qml`

组件接口：
```qml
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    required property AppTheme theme
    required property var model   // ListModel with log entry roles
    property int enabledLevelMask: 0x3F  // all 6 levels
    // ...
}
```

实现要点：

**筛选栏（顶部 Row）：**
- funnel 图标按钮 → 切换 `filterExpanded` 布尔值
- chevronDown 图标（`rotation: filterExpanded ? 180 : 0` + Behavior on rotation）
- trash 图标按钮 → `model.clear()`
- 展开时显示 6 × LogLevelChip（Flow 布局），参数：

| chip index | text         | accentColor     |
|------------|-------------|-----------------|
| 0          | "TRACE"     | theme.accentA   |
| 1          | "DEBUG"     | theme.accentA   |
| 2          | "INFO"      | theme.accentB   |
| 3          | "WARN"      | theme.accentC   |
| 4          | "ERROR"     | theme.accentD   |
| 5          | "CRITICAL"  | theme.accentD   |

- 点击 chip → toggle bit in `enabledLevelMask`

**日志列表（ListView）：**
- `model`: 父传入的 ListModel
- `delegate`: 内联 Component，显示：
  - 左侧 4px 色条（accent 颜色由 level 决定）
  - 头行：[LEVEL badge] [source ellided] [time mono right]
  - 正文：message（Text.Wrap）
  - 底行：`qsTr("tid %1").arg(threadId) + " · " + file + ":" + line`（仅当 file 非空）
- 筛选逻辑：`visible: (root.enabledLevelMask & (1 << model.level)) !== 0`
  - 使用 delegate `visible` + `height: visible ? implicitHeight : 0` 方式实现（比 DelegateModel 简单，mock 阶段足够）

**自动滚动：**
- `onCountChanged: if (atYEnd) positionViewAtEnd()`

- [ ] 步骤 2：在 CMakeLists.txt QML_FILES 中添加 `resource/qml/components/LogEventsView.qml`

- [ ] 步骤 3：验证编译

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 4：提交

```
feat(qml): add LogEventsView component for events tab
```

---

## 任务 5：创建 ActivityPanel.qml

**文件：**
- 新增：`src/app/resource/qml/components/ActivityPanel.qml`
- 修改：`src/app/CMakeLists.txt`（QML_FILES 添加一行）

**参考：** 规格 §3.2, §5

- [ ] 步骤 1：创建 `ActivityPanel.qml`

组件接口：
```qml
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    required property AppTheme theme
    property int currentTab: 0
    signal closeRequested
    // ...
}
```

实现要点：

**布局（ColumnLayout）：**
1. **标题行：** Text "Activity Center" + close 按钮（closePanel 图标）
2. **标签栏（Row）：**
   - Events 按钮（list 图标 + 文字），选中态 accent tint 背景
   - Command Line 按钮（terminal 图标 + 文字）
   - 点击切换 `currentTab`
3. **内容区域（StackLayout 或 visible 切换）：**
   - `currentTab === 0` → LogEventsView { theme; model: mockLogModel }
   - `currentTab === 1` → TerminalView { theme; model: mockTerminalModel; onCommandSubmitted: root.runCommand(text) }

**Mock models:**
```qml
ListModel { id: mockLogModel }
ListModel { id: mockTerminalModel }
```

**Mock 函数：**
- `Component.onCompleted` → 填充 8 条规格 §5 中的样本日志到 `mockLogModel`
- `runCommand(text)`:
  1. `appendTerminalEntry("command", text)`
  2. 启动 200ms Timer → `appendTerminalEntry("response", mockResponse(text))`
- `appendTerminalEntry(type, text)`: append to mockTerminalModel，超 160 条时 remove(0)
- `mockResponse(text)`: 尝试 JSON.parse → 成功返回 `{"status":"ok","echo":...}`；失败返回 error

**面板样式：**
- 背景：`theme.tint(theme.surface, darkMode ? 0.96 : 0.98)`
- 边框：`theme.borderSubtle`，1px
- 圆角：`theme.radiusLarge`（24px）
- 高度：`currentTab === 0 ? 400 : 500`
- 阴影：在主 Rectangle 下方放一个略大的半透明 Rectangle（偏移 2px，radius 相同，color: tint(black, 0.08)）模拟 drop shadow

**动画（交给 ActivityOverlay 控制，此组件本身无 show/hide 动画）**

- [ ] 步骤 2：在 CMakeLists.txt QML_FILES 中添加 `resource/qml/components/ActivityPanel.qml`

- [ ] 步骤 3：验证编译

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 4：提交

```
feat(qml): add ActivityPanel with mock log and terminal models
```

---

## 任务 6：创建 ActivityOverlay.qml

**文件：**
- 新增：`src/app/resource/qml/sections/ActivityOverlay.qml`
- 修改：`src/app/CMakeLists.txt`（QML_FILES 添加一行）

**参考：** 规格 §3.1, §7

- [ ] 步骤 1：创建 `ActivityOverlay.qml`

组件接口：
```qml
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: root
    required property AppTheme theme
    property bool activityOpen: false
    property bool hasNewErrors: false
    property bool hasNewLogs: false
    property real progress: -1
    property string progressStatus: ""
    // ...
}
```

实现要点：

**宽度：** `implicitWidth: Math.min(920, parent.width * 0.5)`

**布局（ColumnLayout, anchors.right + anchors.bottom）：**
1. **进度条（Rectangle）：**
   - 高 6px，visible 当 `progress >= 0`
   - 颜色由 progressStatus 决定（accentA / success / danger）
   - 不确定模式（progress === 0）：内部滑块 1200ms 循环动画
   - 自动隐藏：成功 3s / 失败 6s 后 visible → false
   - Mock 阶段：progress 始终 -1，进度条不可见

2. **ActivityPanel 实例：**
   - `visible: root.activityOpen`
   - 动画：Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
   - 动画：Behavior on y offset (同上)
   - `onCloseRequested: root.activityOpen = false`

3. **Activity 按钮（Rectangle）：**
   - 38px 高，auto 宽
   - pulse 图标 + "Activity" 文字
   - MouseArea → `root.activityOpen = !root.activityOpen`
   - anchors.right: parent.right

4. **通知圆点（Rectangle, 8px）：**
   - visible 当 hasNewErrors || hasNewLogs
   - 颜色：hasNewErrors ? theme.danger : theme.accentB
   - 脉冲动画：SequentialAnimation on opacity { loops: Animation.Infinite; NumberAnimation { to: 0.38; duration: 800 } NumberAnimation { to: 1.0; duration: 800 } }

- [ ] 步骤 2：在 CMakeLists.txt QML_FILES 中添加 `resource/qml/sections/ActivityOverlay.qml`

- [ ] 步骤 3：验证编译

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 4：提交

```
feat(qml): add ActivityOverlay container with button and progress bar
```

---

## 任务 7：集成到 Main.qml

**文件：**
- 修改：`src/app/resource/qml/Main.qml`

**参考：** 规格 §2（Integration Point）

- [ ] 步骤 1：在 Main.qml 的内容区域 `Item`（包含 RowLayout 的那个，当前约 line 122）内部，添加 ActivityOverlay

在 `RowLayout { ... }` 闭合括号之后、`Item` 闭合括号之前添加：

```qml
ActivityOverlay {
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: appTheme.gap
    anchors.bottomMargin: appTheme.gap
    theme: appTheme
    z: 40
}
```

注意：Main.qml 已有 `import "sections"` 和 `import "theme"`，无需新增 import。

- [ ] 步骤 2：验证完整构建

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 3：提交

```
feat(qml): integrate ActivityOverlay into main window layout
```

---

## 任务 8：更新中文翻译

**文件：**
- 修改：`src/app/resource/translations/opengeolab_zh_CN.ts`

**参考：** 规格 §8

- [ ] 步骤 1：在 .ts 文件中添加以下翻译条目

新增上下文和翻译键：

| Context | Source | Translation |
|---------|--------|-------------|
| ActivityOverlay | Activity | 活动 |
| ActivityPanel | Activity Center | 活动中心 |
| ActivityPanel | Events | 事件 |
| ActivityPanel | Command Line | 命令行 |
| TerminalView | Type a command... | 输入命令... |
| TerminalView | Run | 运行 |
| LogEventsView | Clear | 清除 |
| LogEventsView | TRACE | 跟踪 |
| LogEventsView | DEBUG | 调试 |
| LogEventsView | INFO | 信息 |
| LogEventsView | WARN | 警告 |
| LogEventsView | ERROR | 错误 |
| LogEventsView | CRITICAL | 严重 |
| LogEventsView | tid %1 | 线程 %1 |

- [ ] 步骤 2：验证构建（.ts 编译为 .qm）

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 3：提交

```
feat(i18n): add Chinese translations for activity panel components
```

---

## 任务 9：全量构建 + 视觉验证

- [ ] 步骤 1：全量编译

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

预期：零错误，零 QML 相关警告。

- [ ] 步骤 2：运行回归测试

```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

预期：所有已有测试通过（当前无测试也应返回成功）。

- [ ] 步骤 3：启动应用进行视觉验证

```powershell
.\build\src\app\RelWithDebInfo\opengeolab_app.exe
```

验证清单：
1. 右下角可见 "Activity" 按钮
2. 点击按钮 → 面板弹出（有动画）
3. Events 标签 → 可见 8 条 mock 日志（不同级别颜色）
4. 筛选标签 → 可点击切换，日志条目按级别过滤
5. Command Line 标签 → 可输入命令，Ctrl+Enter 提交
6. 终端显示 `>>>` 命令 + `<<<` 响应
7. 切换暗色主题 → 所有面板颜色正确适配
8. 切换中文 → 面板文字变为中文

- [ ] 步骤 4：如有问题，修复后重新验证
- [ ] 步骤 5：提交最终修复（如有）

---

## 依赖关系

```
任务 1 (icons)
    ↓
任务 2 (LogLevelChip) ──┐
任务 3 (TerminalView) ──┤ 可并行
                         ↓
任务 4 (LogEventsView) ← 依赖 LogLevelChip
    ↓
任务 5 (ActivityPanel) ← 依赖 LogEventsView + TerminalView
    ↓
任务 6 (ActivityOverlay) ← 依赖 ActivityPanel
    ↓
任务 7 (Main.qml) ← 依赖 ActivityOverlay
    ↓
任务 8 (translations) ← 依赖所有 QML 文件
    ↓
任务 9 (验证) ← 依赖一切
```

## 关于 TDD

QML 组件为纯视觉组件，当前仓库无 QML 测试框架（qmltest 未配置）。因此本计划不使用 TDD 流程，改为每个任务完成后执行 `cmake --build` 编译验证，最终任务 9 做全量视觉验证。理由：配置 qmltest 超出本功能范围，且 mock 数据的视觉正确性需要人眼确认。
