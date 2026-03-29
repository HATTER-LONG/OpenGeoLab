# View Toolbar 美化 & Shape 颜色一致性设计

> **日期**: 2026-03-29  
> **状态**: Draft  

---

## 1. 问题陈述

当前存在两个独立但关联的 UI/UX 问题：

### 1.1 View Toolbar 视觉粗糙

- 按钮偏小（32×32px），图标简朴（22×22px 简单线条）
- Hover tooltip 使用原生 Qt `ToolTip`，无自定义样式，视觉不精致
- 整体工具栏与应用精致暗色主题的风格不协调

### 1.2 Shape 颜色不一致

- C++ 渲染侧（`scene_bridge.cpp`）使用 **15 色调色盘**：`shape_id % 15`
- QML 侧边栏（`ShapeListItem.qml`）使用 **8 色调色盘**：`shape_id % 8`
- 两套调色盘**颜色完全不同**，导致侧边栏颜色指示与 3D 视口中模型实际面颜色不匹配
- `list_shapes` 后端响应不返回颜色信息，前端独立维护了一套错误的调色盘

---

## 2. 设计方案

### 2.1 View Toolbar 美化

#### 2.1.1 按钮尺寸调整

| 属性 | 当前 | 目标 |
|------|------|------|
| 按钮尺寸 | 32×32px | **36×36px** |
| 图标尺寸 | 22×22px | **24×24px** |
| 工具栏高度 | 40px | **44px** |
| 按钮间距 | 3px | **4px** |

**涉及文件：**
- `src/app/resource/qml/components/ViewToolButton.qml` — 尺寸属性
- `src/app/resource/qml/sections/ViewportPanel.qml` — 容器高度、间距

#### 2.1.2 SVG 图标重新设计

**风格定义：** 精致线条风格，保持立方体方向指示概念。

设计规范：
- ViewBox: 24×24（与现有一致）
- 线条粗细: `stroke-width="1.8"`（参考值，实现时可微调以获得最佳视觉效果）
- 线条端点: `stroke-linecap="round"` + `stroke-linejoin="round"`
- 高亮面: `fill="currentColor" fill-opacity="0.15"`（统一柔和填充）
- 方向箭头: 更精致的箭头造型，线条粗细一致
- 整体风格更圆润、精致，减少生硬的直角感

**7 个图标清单：**

| 图标 | 文件 | 设计要点 |
|------|------|---------|
| Fit All | `view_fit.svg` | 圆角支架 + 居中立方体，支架线条更圆润 |
| Front | `view_front.svg` | 立方体前面高亮填充 |
| Back | `view_back.svg` | 立方体背面高亮填充，虚线暗示前面 |
| Top | `view_top.svg` | 立方体顶面高亮 + 向下箭头 |
| Bottom | `view_bottom.svg` | 立方体底面高亮 + 向上箭头 |
| Left | `view_left.svg` | 立方体左面高亮 + 向右箭头 |
| Right | `view_right.svg` | 立方体右面高亮 + 向左箭头 |

**涉及文件：** `src/app/resource/icons/view_*.svg`（7 个文件全部重写）

#### 2.1.3 自定义 Tooltip 浮层

替换原生 Qt `ToolTip`，使用自定义 QML 组件 `ViewToolTip.qml`。

**设计规范：**
- 圆角矩形背景（radius: 8px）
- 背景色: `theme.surfaceMuted`（暗模式半透明）
- 微妙 `DropShadow`（blur: 12, spread: 0, offset.y: 4, opacity: 0.3）
- 底部小三角箭头指向按钮（6×6px 旋转 45° 的正方形）
- 文字: 13px，`theme.textPrimary`
- 出现动画: 150ms opacity 淡入 + y 轴微移
- 延迟: 400ms hover 后显示
- 定位: 按钮下方居中

**涉及文件：**
- `src/app/resource/qml/components/ViewToolTip.qml`（**新建**）
- `src/app/resource/qml/components/ViewToolButton.qml`（替换原生 ToolTip 为自定义组件）

### 2.2 Shape 颜色一致性

#### 2.2.1 共享调色盘（Core 层）

在 Core 库新增共享调色盘头文件，作为 **唯一真实来源**。

```cpp
// src/libs/core/include/opengeolab/core/shape_color_palette.hpp
namespace OpenGeoLab::Core {

/// 15-colour palette for shape face colouring.
/// Index a shape's colour as: kShapeColorPalette[shape_id % kShapeColorPaletteSize].
constexpr float kShapeColorPalette[][4] = {
    {0.671f, 0.929f, 0.847f, 1.f}, // #ABEDD8 Mint
    {0.275f, 0.804f, 0.812f, 1.f}, // #46CDCF Cyan
    // ... (完整 15 色搬迁自 scene_bridge.cpp)
};
constexpr std::size_t kShapeColorPaletteSize = std::size(kShapeColorPalette);

/// Return hex colour string for a shape ID (e.g. "#ABEDD8").
std::string shapeColorHex(uint32_t shape_id);

} // namespace OpenGeoLab::Core
```

**涉及文件：**
- `src/libs/core/include/opengeolab/core/shape_color_palette.hpp`（**新建**）
- `src/libs/core/src/shape_color_palette.cpp`（**新建** — `shapeColorHex` 实现）
- `src/libs/core/CMakeLists.txt`（新增源文件）

#### 2.2.2 Scene Bridge 迁移

将 `scene_bridge.cpp` 中的本地调色盘替换为 Core 共享版本。

```cpp
// scene_bridge.cpp — 修改后
#include <opengeolab/core/shape_color_palette.hpp>

void applyColorMap(Core::VisualData& visual, uint32_t shape_id) {
    const auto& c = Core::kShapeColorPalette[shape_id % Core::kShapeColorPaletteSize];
    // ... 其余逻辑不变
}
```

**涉及文件：** `src/app/src/scene_bridge.cpp`

#### 2.2.3 list_shapes 响应新增颜色

在 `list_shapes_action.cpp` 的 JSON 响应中为每个 shape 添加 `color` 字段。

**响应格式变更（新增 `color` 字段）：**

```json
{
    "ok": true,
    "action": "list_shapes",
    "shapes": [
        {
            "shapeId": 1,
            "name": "My Box",
            "shapeType": "Solid",
            "color": "#ABEDD8",
            "hasTessellation": true,
            "topology": { ... },
            "boundingBox": { ... }
        }
    ]
}
```

**涉及文件：**
- `src/libs/geometry/src/list_shapes_action.cpp`（新增 color 字段，geometry 已链接 Core，无需改 CMakeLists.txt）

#### 2.2.4 侧边栏使用后端颜色

QML `ShapeListItem.qml` 移除本地 8 色调色盘，改用后端返回的 `color` 字段。

```qml
// ShapeListItem.qml — 修改后
required property string shapeColor  // 从后端 color 字段获取

// Color block
Rectangle {
    width: 4; height: 16; radius: 2
    color: root.shapeColor  // 直接使用后端颜色
}
```

**降级策略：** `SidebarPanel.qml` 传递时添加 fallback：`shapeColor: modelData.color ?? "#808080"`，确保旧缓存数据或后端未返回 color 时显示中性灰色。

**涉及文件：**
- `src/app/resource/qml/components/ShapeListItem.qml`（移除 palette，新增 shapeColor 属性）
- `src/app/resource/qml/sections/SidebarPanel.qml`（传递 color 字段到 ShapeListItem）

---

## 3. 关键约束

1. **不改变渲染管线** — 颜色通过 uniform 传递给 shader 的机制不变
2. **不改变调色盘内容** — 15 色内容保持不变，只是迁移位置
3. **主题一致性** — tooltip 和按钮样式使用 AppTheme 中定义的颜色
4. **向后兼容** — `list_shapes` 新增字段不会破坏已有消费者（仅 additive change）
5. **i18n** — tooltip 文本已使用 `qsTr()` 包裹，不需额外处理

---

## 4. 涉及文件清单

### 新建文件
| 文件 | 职责 |
|------|------|
| `src/libs/core/include/opengeolab/core/shape_color_palette.hpp` | 共享调色盘常量 + hex 转换声明 |
| `src/libs/core/src/shape_color_palette.cpp` | hex 转换实现 |
| `src/app/resource/qml/components/ViewToolTip.qml` | 自定义浮层 tooltip 组件 |

### 修改文件
| 文件 | 变更 |
|------|------|
| `src/app/resource/icons/view_fit.svg` | 图标重绘 |
| `src/app/resource/icons/view_front.svg` | 图标重绘 |
| `src/app/resource/icons/view_back.svg` | 图标重绘 |
| `src/app/resource/icons/view_top.svg` | 图标重绘 |
| `src/app/resource/icons/view_bottom.svg` | 图标重绘 |
| `src/app/resource/icons/view_left.svg` | 图标重绘 |
| `src/app/resource/icons/view_right.svg` | 图标重绘 |
| `src/app/resource/qml/components/ViewToolButton.qml` | 尺寸 + tooltip 替换 |
| `src/app/resource/qml/sections/ViewportPanel.qml` | 容器尺寸调整 |
| `src/app/resource/qml/components/ShapeListItem.qml` | 移除本地调色盘，使用后端颜色 |
| `src/app/resource/qml/sections/SidebarPanel.qml` | 传递 color 到 ShapeListItem |
| `src/app/src/scene_bridge.cpp` | 使用 Core 共享调色盘 |
| `src/libs/geometry/src/list_shapes_action.cpp` | 响应新增 color 字段（geometry 已链接 Core，无需改 CMakeLists.txt） |
| `src/libs/core/CMakeLists.txt` | 新增源文件 |

---

## 5. 测试策略

- **构建验证**: `cmake --build build --config RelWithDebInfo --parallel 4`
- **单元测试**: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- **视觉验证**: 创建 Box + Sphere，确认：
  - 3D 视口中两个模型颜色不同，且各自所有面颜色一致
  - 侧边栏颜色指示条与 3D 视口中对应模型颜色完全匹配
  - View toolbar 按钮 hover 时显示精致浮层 tooltip
  - 图标清晰、风格统一
