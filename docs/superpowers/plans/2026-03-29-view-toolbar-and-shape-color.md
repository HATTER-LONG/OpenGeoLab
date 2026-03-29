# View Toolbar 美化 & Shape 颜色一致性 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 美化 view toolbar（图标重绘、按钮增大、自定义 tooltip）并修复 shape 颜色在侧边栏与 3D 渲染之间不一致的问题。

**架构：** 将 15 色调色盘提取到 Core 共享头文件，使 C++ 渲染端和 list_shapes 响应使用同一数据源；QML 侧边栏通过后端返回的 `color` 字段显示颜色。View toolbar 通过重写 SVG 图标、调整 QML 尺寸和新建自定义 tooltip 组件来美化。

**技术栈：** C++20 / CMake / Qt 6 QML / SVG / OpenGL (shaders 不变)

---

## 任务 0：基线构建验证

**文件：** 无修改

- [ ] 步骤 1：执行 `cmake --build build --config RelWithDebInfo --parallel 4`，确认当前代码构建通过
- [ ] 步骤 2：执行 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`，确认所有测试通过

**验证命令：** 上述两条命令
**预期结果：** 构建 0 error，测试全部 pass

---

## 任务 1：Core 共享调色盘

**文件：**
- 新增：`src/libs/core/include/opengeolab/core/shape_color_palette.hpp`
- 新增：`src/libs/core/src/shape_color_palette.cpp`
- 修改：`src/libs/core/CMakeLists.txt`

### 步骤

- [ ] 步骤 1：新建 `shape_color_palette.hpp`

```cpp
/// @file shape_color_palette.hpp
/// @brief Shared 15-colour palette for shape face colouring.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace OpenGeoLab::Core {

/// 15-colour face palette — index by `shape_id % kShapeColorPaletteSize`.
constexpr std::array<std::array<float, 4>, 15> kShapeColorPalette = {{
    {0.671f, 0.929f, 0.847f, 1.f}, // #ABEDD8 Mint
    {0.275f, 0.804f, 0.812f, 1.f}, // #46CDCF Cyan
    {0.722f, 0.537f, 0.325f, 1.f}, // #B88953 Brown
    {0.698f, 0.875f, 0.541f, 1.f}, // #B2DF8A Light Green
    {0.200f, 0.627f, 0.173f, 1.f}, // #33A02C Dark Green
    {0.122f, 0.471f, 0.706f, 1.f}, // #1F78B4 Blue
    {0.651f, 0.808f, 0.890f, 1.f}, // #A6CEE3 Light Blue
    {0.984f, 0.604f, 0.600f, 1.f}, // #FB9A99 Pink
    {0.800f, 0.659f, 0.914f, 1.f}, // #CCA8E9 Lavender
    {0.596f, 0.306f, 0.639f, 1.f}, // #984EA3 Purple
    {1.000f, 1.000f, 0.200f, 1.f}, // #FFFF33 Yellow
    {0.216f, 0.494f, 0.722f, 1.f}, // #377EB8 Steel Blue
    {0.302f, 0.686f, 0.290f, 1.f}, // #4DAF4A Forest Green
    {0.969f, 0.506f, 0.749f, 1.f}, // #F781BF Hot Pink
    {0.455f, 0.263f, 0.263f, 1.f}, // #744343 Dark Brown
}};

/// Total number of palette entries.
constexpr std::size_t kShapeColorPaletteSize = kShapeColorPalette.size();

/// Return hex colour string for a shape ID (e.g. "#ABEDD8").
/// Wraps `kShapeColorPalette[shape_id % kShapeColorPaletteSize]`.
std::string shapeColorHex(uint32_t shape_id);

} // namespace OpenGeoLab::Core
```

- [ ] 步骤 2：新建 `shape_color_palette.cpp`

```cpp
/// @file shape_color_palette.cpp
/// @brief shapeColorHex implementation.
#include <opengeolab/core/shape_color_palette.hpp>

#include <cmath>
#include <cstdio>

namespace OpenGeoLab::Core {

std::string shapeColorHex(uint32_t shape_id) {
    const auto& c = kShapeColorPalette[shape_id % kShapeColorPaletteSize];
    auto to_byte = [](float v) -> int {
        return static_cast<int>(std::lround(v * 255.f));
    };
    char buf[8]; // "#RRGGBB\0"
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                  to_byte(c[0]), to_byte(c[1]), to_byte(c[2]));
    return buf;
}

} // namespace OpenGeoLab::Core
```

- [ ] 步骤 3：修改 `src/libs/core/CMakeLists.txt`

在 `core_public_headers` 列表末尾添加：
```cmake
include/opengeolab/core/shape_color_palette.hpp
```

在 `core_sources` 列表中添加：
```cmake
src/shape_color_palette.cpp
```

- [ ] 步骤 4：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 5：测试验证 — `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

**预期结果：** 构建 0 error，测试全部 pass

---

## 任务 2：Scene Bridge 迁移到共享调色盘

**文件：**
- 修改：`src/app/src/scene_bridge.cpp`

### 步骤

- [ ] 步骤 1：在 `scene_bridge.cpp` 顶部添加 `#include <opengeolab/core/shape_color_palette.hpp>`

- [ ] 步骤 2：删除文件中的本地调色盘定义（L21-38 的 `kFaceColorMap` 和 `kFaceColorMapSize`）

- [ ] 步骤 3：修改 `applyColorMap` 函数，将 `kFaceColorMap` 替换为 `Core::kShapeColorPalette`，`kFaceColorMapSize` 替换为 `Core::kShapeColorPaletteSize`：

```cpp
void applyColorMap(Core::VisualData& visual, uint32_t shape_id) {
    const auto& c = Core::kShapeColorPalette[shape_id % Core::kShapeColorPaletteSize];
    for(auto& surf : visual.surfaces) {
        std::copy(c.begin(), c.end(), std::begin(surf.defaultColor));
    }
    for(auto& edge : visual.edges) {
        std::copy(std::begin(kEdgeColor), std::end(kEdgeColor), std::begin(edge.color));
    }
    for(auto& pts : visual.points) {
        std::copy(std::begin(kVertexColor), std::end(kVertexColor), std::begin(pts.color));
        pts.pointSize = kVertexPointSize;
    }
}
```

注意：`kEdgeColor`、`kVertexColor`、`kVertexPointSize` 保持原位不动（它们是 app 级常量，仅 `kFaceColorMap` 被替换）。

- [ ] 步骤 4：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 5：测试验证 — `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

**预期结果：** 构建 0 error，测试全部 pass，渲染颜色与之前完全一致（数据未变，只是引用源变了）

---

## 任务 3：list_shapes 响应新增 color 字段

**文件：**
- 修改：`src/libs/geometry/src/list_shapes_action.cpp`

### 步骤

- [ ] 步骤 1：在文件顶部添加 `#include <opengeolab/core/shape_color_palette.hpp>`

- [ ] 步骤 2：在 `shapes.push_back(...)` 的 JSON 对象中，紧跟 `"shapeType"` 之后添加 `"color"` 字段：

```cpp
shapes.push_back(
    {{"shapeId", id},
     {"name", entry->name},
     {"shapeType", shapeTypeToString(entry->shape.ShapeType())},
     {"color", Core::shapeColorHex(id)},  // ← 新增
     {"hasTessellation", entry->visualData != nullptr},
     // ... 其余不变
    });
```

geometry 模块已链接 `OpenGeoLab::Core`（见 `src/libs/geometry/CMakeLists.txt:40`），无需修改 CMake。

- [ ] 步骤 3：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 4：测试验证 — `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

**预期结果：** 构建 0 error，`list_shapes` 响应中每个 shape 对象包含 `"color": "#XXXXXX"` 字段

---

## 任务 4：侧边栏使用后端颜色

**文件：**
- 修改：`src/app/resource/qml/components/ShapeListItem.qml`
- 修改：`src/app/resource/qml/sections/SidebarPanel.qml`

### 步骤

- [ ] 步骤 1：修改 `ShapeListItem.qml`

  1. 新增 required property：
     ```qml
     required property string shapeColor
     ```
  2. 删除本地 `palette` 属性（L26-29 的 `readonly property var palette: [...]`）
  3. 修改 Color block（约 L76）：
     ```qml
     // 旧：color: root.palette[root.shapeId % root.palette.length]
     // 新：
     color: root.shapeColor
     ```

- [ ] 步骤 2：修改 `SidebarPanel.qml`

  在 `ShapeListItem` delegate（约 L138-155）中添加 `shapeColor` 绑定，带 fallback：
  ```qml
  shapeColor: modelData.color ?? "#808080"
  ```

- [ ] 步骤 3：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`

**预期结果：** 构建 0 error，侧边栏颜色条与 3D 视口中对应 shape 面颜色完全一致

---

## 任务 5：重绘 7 个 View 图标 SVG

**文件：**
- 修改：`src/app/resource/icons/view_fit.svg`
- 修改：`src/app/resource/icons/view_front.svg`
- 修改：`src/app/resource/icons/view_back.svg`
- 修改：`src/app/resource/icons/view_top.svg`
- 修改：`src/app/resource/icons/view_bottom.svg`
- 修改：`src/app/resource/icons/view_left.svg`
- 修改：`src/app/resource/icons/view_right.svg`

### 设计规范

- viewBox: `0 0 24 24`
- 主线条: `stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" fill="none"`
- 高亮面: `fill="currentColor" fill-opacity="0.15"`
- 方向箭头: 同样使用 `stroke-width="1.8"`，圆角端点
- 保持所有 SVG 使用 `currentColor`，以支持 Qt icon.color 动态着色

### 步骤

- [ ] 步骤 1：重写 `view_fit.svg` — 圆角支架 + 居中立方体，支架线条使用 round linecap
- [ ] 步骤 2：重写 `view_front.svg` — 立方体前面高亮（fill-opacity="0.15"）
- [ ] 步骤 3：重写 `view_back.svg` — 立方体背面高亮（虚线轮廓 + fill-opacity="0.15"）
- [ ] 步骤 4：重写 `view_top.svg` — 立方体顶面路径高亮 + 向下箭头
- [ ] 步骤 5：重写 `view_bottom.svg` — 立方体底面高亮 + 向上箭头
- [ ] 步骤 6：重写 `view_left.svg` — 立方体左面高亮 + 从左侧指向立方体的箭头
- [ ] 步骤 7：重写 `view_right.svg` — 立方体右面高亮 + 从右侧指向立方体的箭头
- [ ] 步骤 8：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`

**预期结果：** 构建通过，7 个图标风格统一、精致

**注意：** SVG 为纯视觉资源，不适合 TDD。通过构建验证 + 视觉检查确认。

---

## 任务 6：ViewToolButton 尺寸调整

**文件：**
- 修改：`src/app/resource/qml/components/ViewToolButton.qml`
- 修改：`src/app/resource/qml/sections/ViewportPanel.qml`

### 步骤

- [ ] 步骤 1：修改 `ViewToolButton.qml`

  ```qml
  // 旧
  width: 32
  height: 32
  icon.width: 22
  icon.height: 22

  // 新
  width: 36
  height: 36
  icon.width: 24
  icon.height: 24
  ```

- [ ] 步骤 2：修改 `ViewportPanel.qml`

  ```qml
  // 旧
  implicitHeight: 40
  // ...
  spacing: 3

  // 新
  implicitHeight: 44
  // ...
  spacing: 4
  ```

  同时更新分割线高度（如果硬编码）：
  ```qml
  // 旧：Rectangle { width: 1; height: 22; ... }
  // 新：Rectangle { width: 1; height: 24; ... }
  ```

- [ ] 步骤 3：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`

**预期结果：** 构建通过，按钮视觉更大更舒适

---

## 任务 7：自定义 ViewToolTip 组件

**文件：**
- 新增：`src/app/resource/qml/components/ViewToolTip.qml`
- 修改：`src/app/resource/qml/components/ViewToolButton.qml`

### 步骤

- [ ] 步骤 1：新建 `ViewToolTip.qml`

```qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import "../theme"

/// @brief Custom styled tooltip card with arrow, shadow, and fade-in animation.
Item {
    id: tip

    required property AppTheme theme
    property alias text: label.text
    property bool shown: false

    visible: opacity > 0
    opacity: 0
    width: label.implicitWidth + 20
    height: label.implicitHeight + 14

    // 定位：由使用方在按钮下方居中放置

    Behavior on opacity {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }
    Behavior on y {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    states: State {
        name: "visible"
        when: tip.shown
        PropertyChanges { target: tip; opacity: 1.0 }
    }

    // 背景卡片
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.topMargin: 6  // 为箭头留空间
        radius: 8
        color: tip.theme.surfaceMuted
        border.width: 1
        border.color: tip.theme.borderSubtle

        // 顶部小三角箭头
        Rectangle {
            width: 10; height: 10
            rotation: 45
            color: card.color
            border.width: 1
            border.color: card.border.color
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: -5
        }

        // 箭头覆盖条（隐藏箭头底部边框）
        Rectangle {
            width: 16; height: 6
            color: card.color
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
        }

        Text {
            id: label
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 0
            font.pixelSize: 13
            font.weight: Font.Medium
            color: tip.theme.textPrimary
        }
    }

    // 阴影
    layer.enabled: tip.visible
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowBlur: 0.4
        shadowVerticalOffset: 4
        shadowColor: Qt.rgba(0, 0, 0, 0.25)
    }
}
```

- [ ] 步骤 2：修改 `ViewToolButton.qml` — 移除原生 ToolTip，集成自定义 tooltip

  1. 删除 L55-57 的三行原生 ToolTip 代码：
     ```qml
     // 删除：
     // ToolTip.visible: hovered && button.toolTipText !== ""
     // ToolTip.text: button.toolTipText
     // ToolTip.delay: 500
     ```

  2. 在 ToolButton 内部添加 hover 延迟 Timer + 自定义 tooltip：
     ```qml
     // 400ms hover 延迟
     Timer {
         id: tipDelay
         interval: 400
         onTriggered: tipItem.shown = true
     }

     onHoveredChanged: {
         if (hovered && toolTipText !== "") {
             tipDelay.restart()
         } else {
             tipDelay.stop()
             tipItem.shown = false
         }
     }

     ViewToolTip {
         id: tipItem
         theme: button.theme
         text: button.toolTipText
         anchors.horizontalCenter: button.horizontalCenter
         anchors.top: button.bottom
         anchors.topMargin: 4
     }
     ```

  3. 确保 ToolButton 有 `clip: false` 以允许 tooltip 超出按钮边界显示。
  4. 确保 `ViewportPanel.qml` 中的 `toolbarBg` Rectangle 也设置 `clip: false`（默认值即为 false，但需确认无显式覆盖）。

- [ ] 步骤 3：构建验证 — `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 4：测试验证 — `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

**预期结果：** 构建通过，hover 按钮时出现精致浮层卡片 tooltip，带箭头和阴影

---

## 任务 8：最终回归验证与提交

**文件：** 无新增修改

- [ ] 步骤 1：完整构建 — `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 2：完整测试 — `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 3：`git add -A && git diff --cached --stat` — 检查变更范围
- [ ] 步骤 4：请求用户确认后提交，建议提交信息：

```
feat(app): beautify view toolbar and unify shape color palette

- Redesign 7 view toolbar SVG icons with refined line style
- Increase toolbar button size to 36×36px with 24×24px icons
- Replace native Qt ToolTip with custom floating card tooltip
- Extract 15-color shape palette to Core shared header
- Add color field to list_shapes backend response
- Sidebar now displays the exact same color as 3D rendered faces
```

**预期结果：** 全绿构建，全通过测试，一次干净提交
