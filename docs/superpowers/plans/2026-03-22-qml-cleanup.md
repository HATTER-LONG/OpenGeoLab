# QML 代码清理 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 消除 QML 组件中的硬编码映射链，统一图标命名约定，去除 Qt5Compat 过渡依赖，提升代码可扩展性。

**架构：** 图标文件重命名为 `{iconKind}.svg` 约定，消除 AppIcon 中的 if/else 映射；accent 颜色查找下沉到 AppTheme 的 `accentByName()` 方法；ColorOverlay 替换为 Qt 6 原生 MultiEffect。

**技术栈：** Qt 6.9 QML, CMake, QtQuick.Effects (MultiEffect)

**规格文档：** `docs/superpowers/specs/2026-03-22-qml-cleanup-design.md`

**TDD 说明：** 本次改动全部是 QML 界面层重构，无 C++ 测试基础设施可覆盖。验证方式为构建通过 + 手动目视检查。

---

## 文件结构

### 会修改的文件

| 文件 | 职责 |
|------|------|
| `src/app/resource/icons/` | 6 个 SVG 文件重命名 |
| `src/app/CMakeLists.txt` | 更新 RESOURCES 列表中的 6 个文件名 |
| `src/app/resource/qml/components/AppIcon.qml` | 简化 iconFileName、theme required、MultiEffect 替换 |
| `src/app/resource/qml/theme/AppTheme.qml` | 添加 `accentByName()` 方法 |
| `src/app/resource/qml/sections/HeaderRibbonGroup.qml` | 删除 `accentColor()`，改用 `theme.accentByName()` |

### 不会修改的文件

所有其他 QML 文件、C++ 源码、其他 CMakeLists.txt 均不触达。

### 需要保持稳定的边界

- `AppIcon` 的公共接口（`iconKind`, `theme`, `primaryColor`, `useThemeContrast`）保持不变
- `AppTheme` 的现有属性和 `tint()` 方法保持不变
- `HeaderRibbonGroup` 的信号和属性接口保持不变
- RibbonConfig 的数据结构（`"accentOne": "accentA"` 字符串键）保持不变

---

## 任务 1：重命名图标文件 + 更新 CMakeLists.txt

**文件：**
- 修改：`src/app/resource/icons/` 中 6 个 SVG 文件（git mv 重命名）
- 修改：`src/app/CMakeLists.txt`

- [ ] 步骤 1：使用 `git mv` 重命名 6 个图标文件

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLabBack
git mv src/app/resource/icons/dark.svg src/app/resource/icons/darkTheme.svg
git mv src/app/resource/icons/light.svg src/app/resource/icons/lightTheme.svg
git mv src/app/resource/icons/smooth_mesh.svg src/app/resource/icons/smoothMesh.svg
git mv src/app/resource/icons/ai_suggestion.svg src/app/resource/icons/aiSuggest.svg
git mv src/app/resource/icons/ai_chat.svg src/app/resource/icons/aiChat.svg
git mv src/app/resource/icons/export_record.svg src/app/resource/icons/exportRecord.svg
```

- [ ] 步骤 2：更新 `src/app/CMakeLists.txt` RESOURCES 列表中对应的 6 个文件名

替换表：

| 原值 | 新值 |
|------|------|
| `resource/icons/dark.svg` | `resource/icons/darkTheme.svg` |
| `resource/icons/light.svg` | `resource/icons/lightTheme.svg` |
| `resource/icons/smooth_mesh.svg` | `resource/icons/smoothMesh.svg` |
| `resource/icons/ai_suggestion.svg` | `resource/icons/aiSuggest.svg` |
| `resource/icons/ai_chat.svg` | `resource/icons/aiChat.svg` |
| `resource/icons/export_record.svg` | `resource/icons/exportRecord.svg` |

- [ ] 步骤 3：验证构建通过

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 编译成功。此时图标在运行时仍可加载（AppIcon.qml 的 if/else 映射尚存，指向旧文件名会 404，但不影响编译）。此为预期中间态，运行时图标暂时不可见。

---

## 任务 2：重构 AppIcon.qml

**文件：**
- 修改：`src/app/resource/qml/components/AppIcon.qml`

**依赖：** 任务 1 完成（图标文件已重命名）

- [ ] 步骤 1：将 `property AppTheme theme` 改为 `required property AppTheme theme`

将第 11 行：
```qml
property AppTheme theme
```
改为：
```qml
required property AppTheme theme
```

- [ ] 步骤 2：将 `primaryColor` 默认值从硬编码改为 theme 推导

将第 13 行：
```qml
property color primaryColor: "#1473e6"
```
改为：
```qml
property color primaryColor: theme.accentA
```

- [ ] 步骤 3：简化 `iconFileName` — 删除整个 if/else 块，替换为一行

将第 23-46 行（整个 `readonly property string iconFileName: { ... }` 块）替换为：
```qml
readonly property string iconFileName: iconKind.length > 0 ? iconKind + ".svg" : ""
```

- [ ] 步骤 4：替换 ColorOverlay 为 MultiEffect

4a. 将 import 行从：
```qml
import Qt5Compat.GraphicalEffects
```
改为：
```qml
import QtQuick.Effects
```

4b. 将 Image + ColorOverlay + fallback Rectangle 替换为：
```qml
Image {
    id: iconImage

    anchors.fill: parent
    source: iconRoot.iconUrl
    fillMode: Image.PreserveAspectFit
    smooth: true
    antialiasing: true
    cache: true
    sourceSize.width: iconRoot.pixelWidth
    sourceSize.height: iconRoot.pixelHeight
    visible: iconImage.status === Image.Ready && iconRoot.iconUrl.toString().length > 0
    layer.enabled: true
    layer.effect: MultiEffect {
        colorization: 1.0
        colorizationColor: iconRoot.overlayColor
    }
}

Rectangle {
    anchors.fill: parent
    radius: width / 4
    color: Qt.rgba(iconRoot.overlayColor.r, iconRoot.overlayColor.g, iconRoot.overlayColor.b, 0.12)
    border.width: 1
    border.color: Qt.rgba(iconRoot.overlayColor.r, iconRoot.overlayColor.g, iconRoot.overlayColor.b, 0.32)
    visible: iconImage.status !== Image.Ready && iconRoot.iconKind.length > 0
}
```

- [ ] 步骤 5：验证构建通过

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

如果构建失败或运行时 `import QtQuick.Effects` 报错，在 `src/app/CMakeLists.txt` 中添加 `Qt6::QuickEffects` 链接：

```cmake
# 在 find_package 中追加 QuickEffects 组件（如果顶层 find_package 未包含）
# 在 target_link_libraries 中追加：
Qt6::QuickEffects
```

然后重新构建验证。

**预期结果：** 编译成功。

---

## 任务 3：AppTheme 添加 `accentByName()` + 更新 HeaderRibbonGroup

**文件：**
- 修改：`src/app/resource/qml/theme/AppTheme.qml`
- 修改：`src/app/resource/qml/sections/HeaderRibbonGroup.qml`

- [ ] 步骤 1：在 `AppTheme.qml` 末尾（`tint()` 函数之后、闭合 `}` 之前）添加 `accentByName()` 方法

```qml
/// Return the accent color property identified by @p name, defaulting to accentA.
function accentByName(name: string): color {
    const map = { "accentA": accentA, "accentB": accentB, "accentC": accentC,
                  "accentD": accentD, "accentE": accentE };
    return map[name] ?? accentA;
}
```

- [ ] 步骤 2：在 `HeaderRibbonGroup.qml` 中删除 `accentColor()` 函数（第 20-34 行）

- [ ] 步骤 3：更新 `HeaderRibbonGroup.qml` 中的调用点

将第 67-68 行：
```qml
accentOne: groupRoot.accentColor(modelData.accentOne)
accentTwo: groupRoot.accentColor(modelData.accentTwo)
```
改为：
```qml
accentOne: groupRoot.theme.accentByName(modelData.accentOne)
accentTwo: groupRoot.theme.accentByName(modelData.accentTwo)
```

- [ ] 步骤 4：验证构建通过

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 编译成功。

---

## 任务 4：全量验证 + 提交

- [ ] 步骤 1：全量构建

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

- [ ] 步骤 2：全量回归测试

```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

- [ ] 步骤 3：检查所有改动文件

```powershell
git --no-pager status
git --no-pager diff --stat
```

确认改动范围仅限于：
- 6 个 SVG 文件重命名
- `AppIcon.qml`
- `AppTheme.qml`
- `HeaderRibbonGroup.qml`
- `CMakeLists.txt`

- [ ] 步骤 4：提交

```powershell
git add -A
git commit -m "refactor(qml): eliminate hard-coded icon and accent mappings

- Rename 6 SVG icons to match camelCase iconKind convention
- Simplify AppIcon.iconFileName from if/else chain to one-liner
- Add AppTheme.accentByName() and remove HeaderRibbonGroup.accentColor()
- Make AppIcon.theme required for consistency
- Replace Qt5Compat.ColorOverlay with QtQuick.Effects.MultiEffect

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

- [ ] 步骤 5（手动）：启动应用，目视验证

检查项：
- 所有 Ribbon 图标正常显示（无缺失/空白）
- 图标着色与改动前一致（MultiEffect 无明显色偏）
- 主题切换后图标颜色正确跟随
- Ribbon 组按钮的 accent 渐变颜色正确

**MultiEffect 回退标准：** 如果图标着色与原 ColorOverlay 有明显视觉差异，需将 `import QtQuick.Effects` 改回 `import Qt5Compat.GraphicalEffects`，并恢复 ColorOverlay 写法。
