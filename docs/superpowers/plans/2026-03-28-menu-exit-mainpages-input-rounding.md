# Menu Exit + MainPages 路由框架 + 输入框圆角 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 将 Script Recorder 替换为 Exit、搭建 MainPages 浮动页面路由框架、CommandLine 输入框底部圆角。

**架构：** 三个独立改动共享一次提交。任务 1 改菜单数据和 action 处理；任务 2 新增 MainPages singleton + FunctionPageBase 组件并注册到 CMake 和 qmldir；任务 3 修改 TerminalView 一个属性。所有改动都是 QML + CMake，无 C++ 变更。

**技术栈：** Qt 6.9 QML, CMake, ionicons SVG

**规格文档：** `docs/superpowers/specs/2026-03-28-menu-exit-mainpages-input-rounding-design.md`

---

## 文件清单

| 操作 | 文件路径 | 职责 |
|------|----------|------|
| 新增 | `src/app/resource/icons/exitOutline.svg` | Exit 按钮图标 |
| 新增 | `src/app/resource/qml/MainPages.qml` | Singleton 路由管理器 |
| 新增 | `src/app/resource/qml/components/FunctionPageBase.qml` | 浮动页面基类 |
| 修改 | `src/app/CMakeLists.txt` | 注册新 QML 文件和 icon |
| 修改 | `src/app/resource/qml/qmldir` | 注册 MainPages singleton |
| 修改 | `src/app/resource/qml/MenuConfig.qml` | 删除 Script Recorder，增加 Exit |
| 修改 | `src/app/resource/qml/Main.qml` | exit 处理 + MainPages 注入 + functionPagesContainer |
| 修改 | `src/app/resource/qml/components/TerminalView.qml` | 输入框底部圆角 |

---

### 任务 1：Exit 图标 + MenuConfig + action 处理

**文件：**
- 新增：`src/app/resource/icons/exitOutline.svg`
- 修改：`src/app/CMakeLists.txt`
- 修改：`src/app/resource/qml/MenuConfig.qml`
- 修改：`src/app/resource/qml/Main.qml`

- [ ] 步骤 1：拷贝 `C:\Users\layton\Desktop\WorkSpace\Project\ionicons-8.0.13\src\svg\exit-outline.svg` 到 `src/app/resource/icons/exitOutline.svg`

- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 RESOURCES 列表末尾（`resource/icons/copyOutline.svg` 之后）添加：
  ```
  resource/icons/exitOutline.svg
  ```

- [ ] 步骤 3：修改 `src/app/resource/qml/MenuConfig.qml`
  - 删除第 22-32 行整个 Script Recorder section（第二个 object）
  - 在 Workspace section 的 actions 数组末尾（switchLanguage 之后）添加：
  ```qml
  { "key": "exit", "title": qsTr("Exit"), "icon": "exitOutline",
    "accent": "accentD", "alphaScale": "muted", "hoverAccent": "accentD" }
  ```

- [ ] 步骤 4：修改 `src/app/resource/qml/Main.qml` 的 `openActionPage` 函数
  - 在函数开头（toggleTheme 判断之前）添加：
  ```javascript
  if (actionKey === "exit") {
      Qt.quit();
      return;
  }
  ```

- [ ] 步骤 5：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 8
  ```

**预期结果：** 构建成功。Menu 面板只剩 Workspace section，包含 Import、Export、Theme、Language、Exit 五个按钮。

---

### 任务 2：MainPages singleton

**文件：**
- 新增：`src/app/resource/qml/MainPages.qml`
- 修改：`src/app/resource/qml/qmldir`
- 修改：`src/app/CMakeLists.txt`

- [ ] 步骤 1：创建 `src/app/resource/qml/MainPages.qml`
  ```qml
  pragma Singleton
  import QtQuick

  QtObject {
      id: mainPages

      property var theme: null
      property var mainWindow: null
      property var pagesContainer: null
      property string currentOpenPage: ""
      property var pageCache: ({})

      readonly property var componentMap: ({})

      function hasPage(actionId) {
          return actionId in componentMap;
      }

      function getPage(actionId) {
          if (!pageCache[actionId]) {
              const config = componentMap[actionId];
              if (!config) {
                  console.warn("[MainPages] Unknown action:", actionId);
                  return undefined;
              }
              const component = Qt.createComponent(config.path);
              if (component.status === Component.Ready) {
                  const parent = pagesContainer ? pagesContainer : mainPages;
                  pageCache[actionId] = component.createObject(parent);
                  if (pageCache[actionId])
                      console.log("[MainPages] Created page for:", actionId);
              } else if (component.status === Component.Error) {
                  console.error("[MainPages] Failed:", config.path, component.errorString());
              }
          }
          return pageCache[actionId];
      }

      function handleAction(actionId, payload) {
          const config = componentMap[actionId];
          if (config && currentOpenPage && currentOpenPage !== actionId) {
              const currentPage = pageCache[currentOpenPage];
              if (currentPage && typeof currentPage.close === "function")
                  currentPage.close();
          }
          const page = getPage(actionId);
          if (page && typeof page.open === "function") {
              page.open(payload);
              currentOpenPage = actionId;
          } else if (!page) {
              console.warn("[MainPages] No page handler for:", actionId);
          }
      }

      function closeAll() {
          for (const actionId in pageCache) {
              const page = pageCache[actionId];
              if (page && typeof page.close === "function")
                  page.close();
          }
          currentOpenPage = "";
      }
  }
  ```

- [ ] 步骤 2：在 `src/app/resource/qml/qmldir` 末尾添加：
  ```
  singleton MainPages 1.0 MainPages.qml
  ```

- [ ] 步骤 3：在 `src/app/CMakeLists.txt` 的 QML_FILES 列表中，`resource/qml/MenuConfig.qml` 之后添加：
  ```
  resource/qml/MainPages.qml
  ```

- [ ] 步骤 4：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 8
  ```

**预期结果：** 构建成功，MainPages singleton 被编译进 QML 模块。

---

### 任务 3：FunctionPageBase 浮动页面基类

**文件：**
- 新增：`src/app/resource/qml/components/FunctionPageBase.qml`
- 修改：`src/app/CMakeLists.txt`

**参考文件：** `C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OGL\resources\qml\Pages\FunctionPageBase.qml`

- [ ] 步骤 1：创建 `src/app/resource/qml/components/FunctionPageBase.qml`

  核心结构（适配 AppTheme）：
  - **不加** `pragma ComponentBehavior: Bound`（动态创建需要自由上下文访问）
  - Theme 通过 `MainPages.theme` 获取
  - 属性：pageTitle, pageIcon, actionId, pageVisible, maxContentHeight, content (default alias)
  - 方法：open(payload), close(), parsePayload(payload), getParameters(), execute()
  - execute() 调用 `RequestService.submitAsync(JSON.stringify(getParameters()))`
  - UI 结构：shadow rect + panel rect (radiusMedium) + Column { titleBar (draggable) + separator + ScrollView (contentColumn) + separator + button row (Execute accentA / Cancel) }
  - 定位：open() 设 x=292 (280+12), y=12; 拖拽 clamp minX=292, maxX=parent.width-width, minY=12, maxY=parent.height-height
  - Esc 关闭
  - close() 清理 MainPages.currentOpenPage

- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 QML_FILES 列表中，`resource/qml/components/BoxListItem.qml` 之后添加：
  ```
  resource/qml/components/FunctionPageBase.qml
  ```

- [ ] 步骤 3：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 8
  ```

**预期结果：** 构建成功，FunctionPageBase 被编译进 QML 模块。

---

### 任务 4：Main.qml 集成 MainPages + functionPagesContainer

**文件：**
- 修改：`src/app/resource/qml/Main.qml`

- [ ] 步骤 1：在 `Component.onCompleted` 块（第 58-65 行）开头添加 MainPages 注入：
  ```javascript
  MainPages.mainWindow = root;
  MainPages.theme = appTheme;
  MainPages.pagesContainer = functionPagesContainer;
  ```

- [ ] 步骤 2：在第 206 行 `Item { Layout.fillWidth/Height }` 内部，ActivityOverlay 之后添加：
  ```qml
  Item {
      id: functionPagesContainer
      anchors.fill: parent
      z: 100
      visible: MainPages.currentOpenPage.length > 0
  }
  ```

- [ ] 步骤 3：修改 `openActionPage` 函数，在 plugin 处理之后、geometry 直提交之前，插入 MainPages 路由检查：
  ```javascript
  // MainPages 路由 — 有对应页面则走浮动面板
  if (MainPages.hasPage(actionKey)) {
      MainPages.handleAction(actionKey);
      root.menuOpen = false;
      return;
  }
  ```

- [ ] 步骤 4：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 8
  ```

**预期结果：** 构建成功。现有 geometry action 仍走直提交（componentMap 为空）。

---

### 任务 5：CommandLine 输入框底部圆角

**文件：**
- 修改：`src/app/resource/qml/components/TerminalView.qml`

- [ ] 步骤 1：找到 `inputPanel` Rectangle（约第 243 行），在 `color: root.theme.surfaceMuted` 之后添加：
  ```qml
  bottomLeftRadius: root.theme.radiusMedium
  bottomRightRadius: root.theme.radiusMedium
  ```
  使用 `radiusMedium` 与外层 TerminalView 的 `radius: root.theme.radiusMedium` 一致。

- [ ] 步骤 2：构建验证
  ```
  cmake --build build --config RelWithDebInfo --parallel 8
  ```

**预期结果：** 构建成功。输入框底部左右两角圆角化。

---

### 任务 6：全量测试 + 提交

- [ ] 步骤 1：运行全量测试
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```

- [ ] 步骤 2：暂存所有改动
  ```
  git add -A
  ```

- [ ] 步骤 3：**询问用户确认后**提交（遵循仓库规则：提交前必须询问）
  ```
  git commit -m "feat(app): replace Script Recorder with Exit, add MainPages routing framework, round input corners

  - Replace Script Recorder menu section with Exit button (Qt.quit)
  - Add exitOutline.svg icon from ionicons
  - Create MainPages.qml singleton (componentMap + pageCache + handleAction)
  - Create FunctionPageBase.qml (draggable floating page with Execute/Cancel)
  - Add functionPagesContainer in Main.qml with MainPages injection
  - Add MainPages routing check in openActionPage before geometry fallback
  - Add bottomLeftRadius/bottomRightRadius to TerminalView inputPanel

  Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
  ```

**预期结果：** 5/5 测试通过，提交成功。
