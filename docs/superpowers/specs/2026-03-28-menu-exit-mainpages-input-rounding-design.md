# 设计规格：Menu Exit + MainPages 路由框架 + 输入框圆角

## 问题陈述

1. MenuConfig 中 Script Recorder 功能尚未实现，该位置需替换为 Exit 退出按钮
2. 当前 Main.qml 的 `openActionPage()` 函数硬编码了所有 action 处理逻辑，缺乏参考 OGL 仓库的 Action Page 路由框架（Singleton + componentMap + floating page）
3. TerminalView 的 CommandLine 输入框底部为直角，与整体圆角风格不协调

## 任务 1：Script Recorder → Exit

### 改动文件

| 文件 | 操作 |
|------|------|
| `src/app/resource/icons/exitOutline.svg` | 新建：从 ionicons 拷贝 `exit-outline.svg` |
| `src/app/CMakeLists.txt` | 修改：RESOURCES 增加 `exitOutline.svg` |
| `src/app/resource/qml/MenuConfig.qml` | 修改：删除 Script Recorder section，Workspace section 末尾增加 Exit action |
| `src/app/resource/qml/Main.qml` | 修改：openActionPage 增加 `"exit"` → `Qt.quit()` |

### MenuConfig 改动

```qml
// 删除整个 Script Recorder section (第 22-32 行)
// Workspace section actions 末尾增加：
{ "key": "exit", "title": qsTr("Exit"), "icon": "exitOutline",
  "accent": "accentD", "alphaScale": "muted", "hoverAccent": "accentD" }
```

### Main.qml openActionPage 增加

```javascript
if (actionKey === "exit") {
    Qt.quit();
    return;
}
```

---

## 任务 2：MainPages 路由框架

### 架构概览

```
Main.qml
  ├─ AppHeader (ribbonGroups, actionHandler)
  │    └─ onActionTriggered → root.openActionPage(key)
  │         └─ 先查 MainPages.componentMap，有页面则走浮动路由
  │            否则走现有直提交逻辑
  │
  ├─ functionPagesContainer (Item, z:100, 覆盖 viewport 区域)
  │    └─ FunctionPageBase 实例在此容器内创建和定位
  │
  └─ MainPages (Singleton)
       ├─ theme: AppTheme       — 由 Main.qml 注入
       ├─ mainWindow: Window    — 由 Main.qml 注入
       ├─ pagesContainer: Item  — 由 Main.qml 注入
       ├─ componentMap: {}      — actionId → { path, floating }
       ├─ pageCache: {}         — 懒加载实例缓存
       ├─ currentOpenPage: ""   — 当前打开的浮动页面 ID
       ├─ getPage(actionId)     — 获取或创建页面
       ├─ handleAction(actionId, payload) — 路由入口
       └─ closeAll()            — 关闭所有浮动页面
```

### 新建文件

#### 2.1 `src/app/resource/qml/MainPages.qml`

pragma Singleton QtObject，核心职责：

- `property AppTheme theme` — Main.qml 在 `Component.onCompleted` 注入
- `property var mainWindow` — Main.qml 注入，用于 sidebar 宽度检测
- `property var pagesContainer` — functionPagesContainer Item 引用
- `readonly property var componentMap: ({})` — 暂时为空，后续任务逐步添加
- `property var pageCache: ({})` — 缓存已创建的页面实例
- `property string currentOpenPage: ""` — 当前浮动页面 ID

方法：
- `getPage(actionId)` — 从 componentMap 查路径，Qt.createComponent 懒加载，parent 为 pagesContainer
- `handleAction(actionId, payload)` — 关闭当前浮动页面 → 创建/获取目标页面 → open(payload)
- `hasPage(actionId)` — 检查 componentMap 是否包含该 action（供 Main.qml 判断路由）
- `closeAll()` — 关闭所有缓存页面

#### 2.2 `src/app/resource/qml/components/FunctionPageBase.qml`

参考 OGL 的 FunctionPageBase，适配 OpenGeoLabNew 的 theme 系统：

**属性：**
- `property string pageTitle` — 标题
- `property string pageIcon` — AppIcon 的 iconKind
- `property string actionId` — 唯一标识符
- `property bool pageVisible: false` — 可见性
- `property int maxContentHeight: 420` — 内容区最大高度
- `default property alias content: contentColumn.data` — 子类内容插槽

**方法：**
- `open(payload)` — 定位到 sidebar 右侧，设置 pageVisible=true
- `close()` — pageVisible=false，清理 MainPages.currentOpenPage
- `parsePayload(payload)` — 子类可覆盖
- `getParameters()` — 子类覆盖，返回参数 JSON
- `execute()` — 调用 `RequestService.submitAsync(JSON.stringify(getParameters()))`

**UI 结构：**
```
Item (root, visible: pageVisible, z: 1000)
  └─ Rectangle (shadow, -2 margin)
  └─ Rectangle (panel, radius: theme.radiusMedium)
       └─ Column
            ├─ Rectangle (titleBar, draggable)
            │    ├─ AppIcon (pageIcon)
            │    ├─ Text (pageTitle)
            │    └─ close button (✕)
            ├─ Rectangle (separator)
            ├─ ScrollView (contentArea)
            │    └─ Column (contentColumn, default alias)
            ├─ Rectangle (separator)
            └─ Item (buttonRow)
                 ├─ Execute button (accentA)
                 └─ Cancel button (surfaceMuted)
```

**Theme 访问：** 通过 `MainPages.theme` 获取 AppTheme 引用（避免在动态创建时传递 required property）。

**Sidebar 避让与定位逻辑：**

```javascript
// 常量
property int sidebarGap: 12
property int sidebarWidth: 280  // OpenGeoLabNew sidebar 固定宽度

// open() 初始定位
function open(payload) {
    x = sidebarWidth + sidebarGap;  // 280 + 12 = 292
    y = 12;
    pageVisible = true;
    forceActiveFocus();
}

// 拖拽 clamp 范围
// minX = sidebarWidth + sidebarGap  (不覆盖 sidebar)
// maxX = pagesContainer.width - root.width  (不超出右边界)
// minY = 12  (顶部留白)
// maxY = pagesContainer.height - root.height  (不超出底边界)
function _clampX(value) {
    return Math.max(sidebarWidth + sidebarGap,
                    Math.min(root.parent.width - root.width, value));
}
function _clampY(value) {
    return Math.max(12, Math.min(root.parent.height - root.height, value));
}
```

**拖拽：** MouseArea 在 titleBar 上，onPositionChanged 更新 x/y，使用 _clampX/_clampY 约束范围。

**Esc 关闭：** `Keys.onEscapePressed` 调用 close()。

### 修改文件

#### 2.3 `src/app/resource/qml/qmldir`

```
singleton MainPages 1.0 MainPages.qml
```

#### 2.4 `src/app/resource/qml/Main.qml`

1. **添加 functionPagesContainer：**

在 Main.qml 第 206 行的 `Item { Layout.fillWidth/Height }` 内部，与 RowLayout、ActivityOverlay 同级：

```
Item (line 206, content area)
  ├─ MouseArea (menu backdrop, z:10)
  ├─ RowLayout (sidebar + viewport)
  ├─ ActivityOverlay (right-bottom)
  └─ Item#functionPagesContainer (NEW, z:100, anchors.fill: parent)
```

```qml
Item {
    id: functionPagesContainer
    anchors.fill: parent
    z: 100
    visible: MainPages.currentOpenPage.length > 0
}
```

2. **Component.onCompleted 注入：**
```javascript
Component.onCompleted: {
    MainPages.mainWindow = root;
    MainPages.theme = appTheme;
    MainPages.pagesContainer = functionPagesContainer;
    // 现有 plugin list 请求保持不变
    RequestService.submitAsync(...);
}
```

3. **openActionPage 路由优先级：**
```javascript
function openActionPage(actionKey) {
    // 1. 内置处理 (exit, toggleTheme, switchLanguage)
    if (actionKey === "exit") { Qt.quit(); return; }
    if (actionKey === "toggleTheme") { ... return; }
    if (actionKey === "switchLanguage") { ... return; }

    // 2. Plugin 处理 (保持不变)
    if (actionKey.startsWith("pluginUI_")) { ... return; }
    if (actionKey.startsWith("plugin_")) { ... return; }

    // 3. MainPages 路由 — 有对应页面则走浮动面板
    if (MainPages.hasPage(actionKey)) {
        MainPages.handleAction(actionKey);
        return;
    }

    // 4. Fallback: 现有直提交 (geometry 等)
    const geometryActions = { ... };
    if (actionKey in geometryActions) { ... return; }

    // 5. 未实现
    root.statusNote = qsTr("Action: %1 (not yet implemented)").arg(actionKey);
    root.menuOpen = false;
}
```

4. **darkMode 变化同步到 MainPages.theme：** 无需额外处理，appTheme 是同一个对象引用，darkMode 切换时 AppTheme 内部属性自动更新。

#### 2.5 `src/app/CMakeLists.txt`

QML_FILES 为显式列表（非 glob 扫描），必须添加以下两行：

| 添加路径 | 说明 |
|----------|------|
| `resource/qml/MainPages.qml` | Singleton 路由管理器 |
| `resource/qml/components/FunctionPageBase.qml` | 浮动页面基类 |

#### 2.6 `pragma ComponentBehavior` 决策

- `MainPages.qml`：**不加** `pragma ComponentBehavior: Bound`。它是 `pragma Singleton` 的 QtObject，不涉及 delegate 绑定。
- `FunctionPageBase.qml`：**不加** `pragma ComponentBehavior: Bound`。它通过 `Qt.createComponent()` 动态创建，Bound 会限制上下文访问（如访问 `MainPages.theme`），导致运行时错误。

---

## 任务 3：CommandLine 输入框底部圆角

### 改动文件

| 文件 | 操作 |
|------|------|
| `src/app/resource/qml/components/TerminalView.qml` | 修改：inputPanel Rectangle 增加底部圆角 |

### 具体改动

```qml
Rectangle {
    id: inputPanel

    Layout.fillWidth: true
    Layout.preferredHeight: editorContainer.implicitHeight + (root.theme.gapTight * 2)
    color: root.theme.surfaceMuted
    // 底部圆角匹配外层 TerminalView 的 radius
    bottomLeftRadius: root.theme.radiusMedium
    bottomRightRadius: root.theme.radiusMedium
}
```

使用 Qt 6.7+ 的 per-corner radius API（项目已确认 Qt 6.9）。圆角大小使用 `radiusMedium` 与外层 TerminalView 的 `radius: root.theme.radiusMedium` 保持一致。

---

## 验证计划

1. **构建验证：** `cmake --build build --config RelWithDebInfo --parallel 4`
2. **测试验证：** `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
3. **运行时验证：**
   - Menu 面板：Script Recorder 消失，Exit 按钮可见且点击退出
   - MainPages 框架加载无错误，`MainPages.hasPage` 函数可调用
   - 现有 geometry action 仍走直提交路径，功能不变
   - TerminalView 输入框底部圆角可见
4. **clang-format / clang-tidy：** 无 C++ 改动，不需要
