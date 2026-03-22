# QML 代码清理 — 消除硬编码映射，提升可扩展性

## 问题陈述

当前 QML 代码中存在多处硬编码 if/else 映射链，每新增一个图标或 accent 颜色都需要修改组件源码，违反开闭原则，阻碍后续扩展。

核心问题：

1. `AppIcon.qml` 的 `iconFileName` — 6 条 if/else 映射 iconKind → 文件名
2. `HeaderRibbonGroup.qml` 的 `accentColor()` — 5 条 if/else 映射字符串 → 主题颜色
3. `AppIcon.qml` 的 `theme` 属性未标记 `required`，与其他组件不一致
4. `AppIcon.qml` 依赖 `Qt5Compat.GraphicalEffects`（过渡模块）

## 设计方案

### 改动 1：重命名图标文件，统一 `{iconKind}.svg` 约定

**原因：** 图标文件名混用 `snake_case` 和截断名，与 QML 中的 `camelCase` iconKind 不一致。

重命名清单：

| 原文件名 | 新文件名 |
|----------|----------|
| `dark.svg` | `darkTheme.svg` |
| `light.svg` | `lightTheme.svg` |
| `smooth_mesh.svg` | `smoothMesh.svg` |
| `ai_suggestion.svg` | `aiSuggest.svg` |
| `ai_chat.svg` | `aiChat.svg` |
| `export_record.svg` | `exportRecord.svg` |

**结果：** `iconFileName` 简化为：

```qml
readonly property string iconFileName: iconKind.length > 0 ? iconKind + ".svg" : ""
```

新增图标的流程变为：
1. 添加 `{iconKind}.svg` 文件
2. 在 CMakeLists.txt RESOURCES 注册
3. 在 QML 中使用 `iconKind: "newIcon"`
4. **无需修改 AppIcon.qml**

**需同步更新：** `src/app/CMakeLists.txt` 中 RESOURCES 列表的 6 个文件名。

### 改动 2：AppTheme 添加 `accentByName()` 方法

**原因：** `HeaderRibbonGroup.qml` 的 `accentColor()` 是字符串到颜色的手动映射，应下沉到 theme 层。

在 `AppTheme.qml` 中添加：

```qml
function accentByName(name: string): color {
    const map = { "accentA": accentA, "accentB": accentB, "accentC": accentC,
                  "accentD": accentD, "accentE": accentE };
    return map[name] ?? accentA;
}
```

`HeaderRibbonGroup.qml` 中 `accentColor()` 函数删除，调用点改为 `theme.accentByName()`。

### 改动 3：AppIcon.qml `theme` 改为 `required`

```qml
// before
property AppTheme theme
// after
required property AppTheme theme
```

同时删除 `primaryColor` 的硬编码默认值 `"#1473e6"`，改为从 theme 推导：

```qml
property color primaryColor: theme.accentA
```

### 改动 4：用 MultiEffect 替代 Qt5Compat.ColorOverlay

**原因：** `Qt5Compat.GraphicalEffects` 是过渡模块，`MultiEffect` 是 Qt 6.5+ 原生替代。

替换方式：

```qml
// before
import Qt5Compat.GraphicalEffects

Image { id: iconImage; visible: false; ... }
ColorOverlay { source: iconImage; color: overlayColor; ... }

// after
import QtQuick.Effects

Image {
    id: iconImage
    visible: iconImage.status === Image.Ready && iconRoot.iconUrl.toString().length > 0
    layer.enabled: true
    layer.effect: MultiEffect {
        colorization: 1.0
        colorizationColor: iconRoot.overlayColor
    }
    ...
}
```

**注意：** MultiEffect 使用 HSL 着色，对黑色源 SVG 可能偏暗。实现阶段需实测，如不满意可调整 `brightness` 属性补偿，或回退到 Qt5Compat。

**CMake 影响：** 移除 QML import 后无需额外 CMake 清理（`Core5Compat` 从未显式链接，`Qt5Compat.GraphicalEffects` 完全通过 QML 插件机制运行时加载）。需确认替换为 `QtQuick.Effects` 后是否需要新增 `Qt6::QuickEffects` 链接。

## 受影响文件

| 文件 | 改动类型 |
|------|---------|
| `src/app/resource/icons/dark.svg` | 重命名 → `darkTheme.svg` |
| `src/app/resource/icons/light.svg` | 重命名 → `lightTheme.svg` |
| `src/app/resource/icons/smooth_mesh.svg` | 重命名 → `smoothMesh.svg` |
| `src/app/resource/icons/ai_suggestion.svg` | 重命名 → `aiSuggest.svg` |
| `src/app/resource/icons/ai_chat.svg` | 重命名 → `aiChat.svg` |
| `src/app/resource/icons/export_record.svg` | 重命名 → `exportRecord.svg` |
| `src/app/resource/qml/components/AppIcon.qml` | 简化 iconFileName、theme required、MultiEffect 替换 |
| `src/app/resource/qml/theme/AppTheme.qml` | 添加 `accentByName()` |
| `src/app/resource/qml/sections/HeaderRibbonGroup.qml` | 删除 `accentColor()`，改用 `theme.accentByName()` |
| `src/app/CMakeLists.txt` | 更新 RESOURCES 文件名、可能调整 Qt 模块链接 |

## 不在范围内

- `Main.qml` 的 `openActionPage()` 分发逻辑 — 当前只有 1 个 special case，暂不重构
- `RibbonConfig.qml` 的 `var` 类型 — 暂不影响功能
- 窗口黑色闪烁问题 — 独立子系统
- 中文切换支持 — 独立子系统

## 验证策略

1. `cmake --build build --config RelWithDebInfo --parallel 4` — 全量构建通过
2. `ctest --test-dir build -C RelWithDebInfo --output-on-failure` — 回归测试通过
3. 确认所有重命名后的图标文件名在 `qt_add_qml_module` RESOURCES 中正确注册，QML 运行时可解析
4. 手动验证：启动应用，检查所有图标正常显示、着色正确、Ribbon 组件 accent 颜色正确
5. MultiEffect 着色接受标准：与原 ColorOverlay 目视无差异即可；若有明显色偏则调整 `brightness` 或回退至 Qt5Compat
