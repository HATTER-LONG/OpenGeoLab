# Geometry Creation Pages 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 为 Box/Cylinder/Sphere/Torus 四种几何体创建浮动参数页面，替代当前 Main.qml 中的硬编码默认参数直接提交。

**架构：** 3 个共享输入组件（DimensionInput、ParamField、CoordinateField）+ 4 个页面（CreateBoxPage、CreateCylinderPage、CreateSpherePage、CreateTorusPage），全部扩展 FunctionPageBase。页面通过 MainPages.componentMap 注册路由，替换 Main.qml 中的 geometryActions 硬编码块。

**技术栈：** Qt 6.9 QML、MainPages 路由框架、FunctionPageBase 基类、RequestService JSON 协议

**规格文档：** `docs/superpowers/specs/2026-03-28-geometry-pages-design.md`

---

## 文件总览

**新增文件（7个）：**
- `src/app/resource/qml/components/DimensionInput.qml` — 带颜色徽标的数值输入
- `src/app/resource/qml/components/ParamField.qml` — 带标签的文本输入
- `src/app/resource/qml/components/CoordinateField.qml` — XYZ 三维坐标输入组合
- `src/app/resource/qml/components/pages/CreateBoxPage.qml` — Box 参数页面
- `src/app/resource/qml/components/pages/CreateCylinderPage.qml` — Cylinder 参数页面
- `src/app/resource/qml/components/pages/CreateSpherePage.qml` — Sphere 参数页面
- `src/app/resource/qml/components/pages/CreateTorusPage.qml` — Torus 参数页面

**修改文件（4个）：**
- `src/app/resource/qml/MainPages.qml` — 填充 componentMap（4条路由）
- `src/app/resource/qml/Main.qml` — 删除 geometryActions 块（约第 131–150 行）
- `src/app/CMakeLists.txt` — QML_FILES 新增 7 个文件
- `src/app/resource/translations/opengeolab_zh_CN.ts` — 新增翻译条目

---

### 任务 1：DimensionInput 共享组件

**文件：**
- 新增：`src/app/resource/qml/components/DimensionInput.qml`

**参考文件：**
- OGL 原版：`C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OGL\resources\qml\util\DimensionInput.qml`（88 行）
- 当前主题系统：`MainPages.theme` 而非全局 `Theme`

**接口定义：**
```qml
// Properties
property string label: ""          // 徽标字母 ("W", "H", "R" 等)
property real value: 0.0            // 绑定数值
property int decimals: 3            // 小数位数
property real minValue: 0.001       // DoubleValidator 下限（CoordinateField 传 -1e9）
property color accentColor: MainPages.theme.accentA  // 徽标色调
property string tooltipText: ""     // 悬停提示

// Signal
signal valueEdited(real newVal)
```

**内部结构：**
- Rectangle 容器（`MainPages.theme.surface` 背景，`MainPages.theme.radiusSmall` 圆角）
- RowLayout：16×16 彩色徽标 Rectangle + TextField
- DoubleValidator：`bottom: root.minValue`，`decimals` 属性
- editingFinished 处理：`Math.max(root.minValue, parseFloat(text) || 0)` → emit `valueEdited`
- 聚焦时 border 变 2px 并使用 `accentColor`，非聚焦时 1px `MainPages.theme.borderSubtle`
- 徽标带 ToolTip（500ms 延迟，仅当 `tooltipText` 非空时显示）

**步骤：**
- [ ] 创建 `DimensionInput.qml`，实现上述接口和内部结构
- [ ] 确保所有用户可见字符串使用 `qsTr()`
- [ ] 确保使用 `MainPages.theme` 访问主题

**验证：** 在任务 4 中通过 CreateBoxPage 集成验证

---

### 任务 2：ParamField 共享组件

**文件：**
- 新增：`src/app/resource/qml/components/ParamField.qml`

**参考文件：**
- OGL 原版：`C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OGL\resources\qml\Pages\ParamField.qml`（133 行）

**接口定义：**
```qml
// Properties
property string label: ""           // 上方标签文本
property string placeholder: ""     // 占位提示文本
property string value: ""           // 当前文本

// Signal
signal valueEdited(string newValue)
```

**内部结构：**
- Column（4px spacing）
  - Text 标签（`MainPages.theme.textSecondary`，`font.pixelSize: 12`），仅在 `label.length > 0` 时可见
  - Rectangle 输入容器（`MainPages.theme.surface` 背景，`MainPages.theme.radiusSmall` 圆角）
    - TextField（`MainPages.theme.textPrimary`，placeholder `MainPages.theme.textSecondary`）
- 聚焦时 border 变 2px + `MainPages.theme.accentA`，非聚焦 1px + `MainPages.theme.borderSubtle`
- TextField.onTextChanged → emit `valueEdited(text)`

**步骤：**
- [ ] 创建 `ParamField.qml`，实现上述接口
- [ ] 确保 `qsTr()` 包裹用户可见字符串
- [ ] 确保使用 `MainPages.theme`

**验证：** 在任务 4 中通过 CreateBoxPage 集成验证

---

### 任务 3：CoordinateField 共享组件

**文件：**
- 新增：`src/app/resource/qml/components/CoordinateField.qml`

**参考文件：**
- OGL 原版：`C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OGL\resources\qml\Pages\CoordinateField.qml`（185 行）

**接口定义：**
```qml
// Properties
property string label: ""
property real coordX: 0.0
property real coordY: 0.0
property real coordZ: 0.0
property int decimals: 3

// Signal
signal coordinateChanged(real x, real y, real z)
```

**内部结构：**
- Column（4px spacing）
  - Text 标签（同 ParamField 样式），仅在 `label.length > 0` 时可见
  - RowLayout（6px spacing）包含 3 个 DimensionInput：
    - X：`label: "X"`, `accentColor: "#E53935"`, `value: coordX`
    - Y：`label: "Y"`, `accentColor: "#43A047"`, `value: coordY`
    - Z：`label: "Z"`, `accentColor: "#1E88E5"`, `value: coordZ`
  - 每个 DimensionInput 的 `onValueEdited` → 更新对应 coordX/Y/Z → emit `coordinateChanged`
- DimensionInput 的 DoubleValidator 需允许负数（不同于独立 DimensionInput 的 `bottom: 0.001`）
  → CoordinateField 中 DimensionInput 不适合直接复用 bottom: 0.001
  → **方案：** 在 DimensionInput 增加 `property real minValue: 0.001`，CoordinateField 传入 `minValue: -Infinity`
  → 或者 CoordinateField 内联 3 个输入组件而不复用 DimensionInput

**设计决策：** DimensionInput 已包含 `property real minValue: 0.001`（任务 1 完成），CoordinateField 传入 `minValue: -1e9`。

**步骤：**
- [ ] 创建 `CoordinateField.qml`，复用 3 个 DimensionInput 并传入 `minValue: -1e9`
- [ ] 确保 `qsTr()` 和 `MainPages.theme`

**验证：** 在任务 4 中通过 CreateBoxPage 集成验证

---

### 任务 4：CreateBoxPage 页面 + 注册 + 构建验证

**文件：**
- 新增：`src/app/resource/qml/components/pages/CreateBoxPage.qml`
- 修改：`src/app/resource/qml/MainPages.qml` — componentMap 增加 `"addBox"` 路由
- 修改：`src/app/CMakeLists.txt` — QML_FILES 增加 4 个文件（3 共享组件 + CreateBoxPage）

**页面结构（参考规格文档和 OGL AddBoxPage）：**
```qml
FunctionPageBase {
    pageTitle: qsTr("Create Box")
    pageIcon: "cubeOutline"
    actionId: "addBox"

    // Properties
    property string boxName: ""
    property real originX: 0.0
    property real originY: 0.0
    property real originZ: 0.0
    property real dimW: 10.0
    property real dimH: 10.0
    property real dimD: 10.0

    // Content: ParamField + CoordinateField + Dimensions section + Info
}
```

**Content 区域布局：**
1. **ParamField** — `label: qsTr("Box Name")`, `placeholder: qsTr("Auto-generated if empty")`
2. **CoordinateField** — `label: qsTr("Origin Point")`
3. **Dimensions section:**
   - Text 标签 `qsTr("Dimensions")`
   - RowLayout 包含 3 个 DimensionInput：
     - W (red `"#E53935"`) → dimW
     - H (green `"#43A047"`) → dimH
     - D (blue `"#1E88E5"`) → dimD
4. **Info section:**
   - Rectangle（`MainPages.theme.surfaceMuted`，`MainPages.theme.radiusSmall`）
   - Text: `qsTr("Volume: %1").arg((dimW * dimH * dimD).toFixed(3))`

**getParameters() 返回值：**
```javascript
function getParameters() {
    return {
        module: "geometry",
        action: "create_box",
        param: {
            name: root.boxName,
            x: root.originX, y: root.originY, z: root.originZ,
            width: root.dimW, height: root.dimH, depth: root.dimD
        },
        mute: false
    };
}
```

**MainPages.qml componentMap 修改：**
```javascript
readonly property var componentMap: ({
    "addBox": { path: "components/pages/CreateBoxPage.qml" }
})
```

**CMakeLists.txt 修改（在 FunctionPageBase.qml 行之后追加）：**
```cmake
resource/qml/components/DimensionInput.qml
resource/qml/components/ParamField.qml
resource/qml/components/CoordinateField.qml
resource/qml/components/pages/CreateBoxPage.qml
```

**步骤：**
- [ ] 创建 `pages/` 子目录
- [ ] 创建 `CreateBoxPage.qml`
- [ ] 修改 `MainPages.qml` componentMap 添加 addBox 路由
- [ ] 修改 `CMakeLists.txt` QML_FILES 添加 4 个文件
- [ ] 运行构建：`cmake --build build --config RelWithDebInfo --parallel 8`
- [ ] 运行测试：`ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 运行应用验证 Box 页面可正常打开、参数调整、执行

**验证命令：**
```bash
cmake --build build --config RelWithDebInfo --parallel 8
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

**预期结果：** 构建成功，5/5 测试通过，点击 Ribbon "Box" 打开浮动页面，参数可编辑，Execute 提交 JSON。

---

### 任务 5：CreateCylinderPage 页面

**文件：**
- 新增：`src/app/resource/qml/components/pages/CreateCylinderPage.qml`

**页面属性：**
- `cylinderName: ""`, `centerX/Y/Z: 0.0`, `radius: 5.0`, `cylHeight: 10.0`
- `pageTitle: qsTr("Create Cylinder")`, `pageIcon: "cylinder"`, `actionId: "addCylinder"`

**Dimensions section：**
- R (red `"#E53935"`) → radius
- H (blue `"#1E88E5"`) → cylHeight（蓝色，因为和 R 同行时 H 用蓝色）

**Info section：**
- Volume: `(Math.PI * radius * radius * cylHeight).toFixed(3)`
- Surface Area: `(2 * Math.PI * radius * (radius + cylHeight)).toFixed(3)`

**getParameters():**
```javascript
{ module: "geometry", action: "create_cylinder",
  param: { name, x, y, z, radius, height }, mute: false }
```

**步骤：**
- [ ] 创建 `CreateCylinderPage.qml`
- [ ] 修改 `CMakeLists.txt` QML_FILES 增加该文件
- [ ] 修改 `MainPages.qml` componentMap 增加 `"addCylinder"` 路由

---

### 任务 6：CreateSpherePage 页面

**文件：**
- 新增：`src/app/resource/qml/components/pages/CreateSpherePage.qml`

**页面属性：**
- `sphereName: ""`, `centerX/Y/Z: 0.0`, `radius: 5.0`
- `pageTitle: qsTr("Create Sphere")`, `pageIcon: "sphere"`, `actionId: "addSphere"`

**Dimensions section：**
- R (red `"#E53935"`) → radius（仅 1 个 DimensionInput，不需要 RowLayout）

**Info section：**
- Volume: `(4/3 * Math.PI * radius * radius * radius).toFixed(3)`
- Surface Area: `(4 * Math.PI * radius * radius).toFixed(3)`
- Diameter: `(2 * radius).toFixed(3)`

**getParameters():**
```javascript
{ module: "geometry", action: "create_sphere",
  param: { name, x, y, z, radius }, mute: false }
```

**步骤：**
- [ ] 创建 `CreateSpherePage.qml`
- [ ] 修改 `CMakeLists.txt` QML_FILES 增加该文件
- [ ] 修改 `MainPages.qml` componentMap 增加 `"addSphere"` 路由

---

### 任务 7：CreateTorusPage 页面

**文件：**
- 新增：`src/app/resource/qml/components/pages/CreateTorusPage.qml`

**页面属性：**
- `torusName: ""`, `centerX/Y/Z: 0.0`, `majorRadius: 10.0`, `minorRadius: 3.0`
- `pageTitle: qsTr("Create Torus")`, `pageIcon: "torus"`, `actionId: "addTorus"`

**Dimensions section：**
- R1 (red `"#E53935"`) → majorRadius
- R2 (green `"#43A047"`) → minorRadius

**Validation warning：**
- 当 `minorRadius >= majorRadius` 时，显示橙色警告框
- 使用 `MainPages.theme.accentC` 作为警告色调
- 文本：`qsTr("Minor radius should be less than major radius")`
- `visible: root.minorRadius >= root.majorRadius`

**Info section：**
- Volume: `(2 * Math.PI * Math.PI * majorRadius * minorRadius * minorRadius).toFixed(3)`
- Surface Area: `(4 * Math.PI * Math.PI * majorRadius * minorRadius).toFixed(3)`
- Outer Diameter: `(2 * (majorRadius + minorRadius)).toFixed(3)`

**getParameters():**
```javascript
{ module: "geometry", action: "create_torus",
  param: { name, x, y, z, majorRadius, minorRadius }, mute: false }
```

**步骤：**
- [ ] 创建 `CreateTorusPage.qml`（含验证警告）
- [ ] 修改 `CMakeLists.txt` QML_FILES 增加该文件
- [ ] 修改 `MainPages.qml` componentMap 增加 `"addTorus"` 路由

---

### 任务 8：Main.qml 清理 + 翻译 + 最终验证

**文件：**
- 修改：`src/app/resource/qml/Main.qml` — 删除 geometryActions 块
- 修改：`src/app/resource/translations/opengeolab_zh_CN.ts` — 新增翻译

**Main.qml 修改（删除约第 131–150 行）：**
删除以下代码块：
```javascript
// Other actions — not yet implemented.
// Geometry creation actions
const geometryActions = { ... };

if (actionKey in geometryActions) { ... }
```
保留最后的 fallback：
```javascript
root.statusNote = qsTr("Action: %1 (not yet implemented)").arg(actionKey);
```

**翻译新增条目（opengeolab_zh_CN.ts）：**
需要为以下 QML context 增加翻译：
- `CreateBoxPage`: "Create Box", "Box Name", "Auto-generated if empty", "Origin Point", "Dimensions", "Volume: %1"
- `CreateCylinderPage`: "Create Cylinder", "Cylinder Name", "Center Point", "Volume: %1", "Surface Area: %1"
- `CreateSpherePage`: "Create Sphere", "Sphere Name", "Volume: %1", "Surface Area: %1", "Diameter: %1"
- `CreateTorusPage`: "Create Torus", "Torus Name", "Volume: %1", "Surface Area: %1", "Outer Diameter: %1", "Minor radius should be less than major radius"
- `DimensionInput`: (无用户可见字符串，tooltip 由调用方设置)
- `ParamField`: (无用户可见字符串，label/placeholder 由调用方设置)
- `CoordinateField`: (无用户可见字符串)

**步骤：**
- [ ] 删除 `Main.qml` 中 geometryActions 相关代码块（约第 130–150 行）
- [ ] 更新 `opengeolab_zh_CN.ts` 翻译文件
- [ ] 运行完整构建：`cmake --build build --config RelWithDebInfo --parallel 8`
- [ ] 运行完整测试：`ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 手动验证：4 个几何体菜单项均打开浮动页面，参数可编辑，Execute 提交正确 JSON

**验证命令：**
```bash
cmake --build build --config RelWithDebInfo --parallel 8
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

**预期结果：** 构建成功，5/5 测试通过，4 个几何体页面功能正常，geometryActions 已移除。

---

### 任务 9：Git 提交

**前置条件：** 任务 8 构建和测试全部通过。

**注意：** 必须先询问用户确认后才能执行 `git commit`。

**步骤：**
- [ ] `git add` 所有新增和修改文件
- [ ] 询问用户确认提交
- [ ] 提交信息：`feat(scene): add geometry creation parameter pages`
  - 正文说明 4 个页面 + 3 个共享组件 + MainPages 路由 + geometryActions 移除

---

## 注意事项

1. **不需要 TDD：** 这些都是纯 QML UI 组件，项目中没有 QML 测试框架。验证通过构建 + 手动测试完成。
2. **Theme 访问：** 所有新组件使用 `MainPages.theme` 而非全局 Theme。不添加 `pragma ComponentBehavior: Bound`（动态创建的组件不兼容）。
3. **Icon：** 4 个图标（cubeOutline.svg, cylinder.svg, sphere.svg, torus.svg）已存在于 `resource/icons/`，无需新增。
4. **qmldir 不需要手动更新：** `components/qmldir` 列出可直接导入的组件，但 pages/ 子目录下的页面通过 `Qt.createComponent()` 动态创建，不需要 qmldir 注册。共享组件（DimensionInput、ParamField、CoordinateField）如果仅被 pages/ 使用，也不需要 qmldir 注册——但如果其他组件将来可能复用，建议注册。决策：暂不注册 qmldir，保持一致（FunctionPageBase 也不在 qmldir 中）。
5. **当前暂存改动未提交：** 当前工作区还有之前的 review fixes + MainPages 框架改动未提交。建议先提交那些，再开始本计划。或者将所有改动合并为更大的提交。提交策略在任务 9 与用户确认。
