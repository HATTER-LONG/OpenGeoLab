# Sidebar Shape Explorer 设计规格（v2）

> **v2 更新说明**：基于已完成的 Command Direct Dispatch 重构（`62c989c`），重新设计通知机制。
> App 层不再直接依赖 Geometry 类型，改用 Command 层 EventBus 抽象实现干净分层。

## 目标

在左侧 SidebarPanel 中展示当前 ShapeStore 中所有顶层 shape 的实时列表，支持折叠/展开详情、几何与网格显隐控制，并通过 push 通知机制确保 UI、Python 脚本或任意来源的 shape 变更都能反映到 sidebar。

## 依赖层级约束

```
QML  →  App  →  Command  →  Core
                                ↑
                           Geometry（仅 Command 层可见）
```

**App 层和 QML 层不 include 任何 Geometry 头文件。** 所有跨层通信通过：
- JSON（QML ↔ RequestService ↔ CommandDispatcher）
- Command 层事件订阅 API（App 订阅模块事件）
- Qt 信号（App → QML）

## 架构概览

```
┌────────────────────────────────────────────────────────────────┐
│  Core 层                                                       │
│                                                                │
│  ModuleDataEvent (enum)     ModuleBase                         │
│  ┌──────────────────┐       ┌──────────────────────────────┐  │
│  │ ItemAdded         │       │ Signal<ModuleDataEvent>      │  │
│  │ ItemRemoved       │       │   dataChanged                │  │
│  │ ItemRenamed       │       └──────────────────────────────┘  │
│  │ ItemModified      │                    ↑ emit               │
│  │ DerivedDataUpdated│       ┌─────────────┴──────────────┐   │
│  │ BulkChanged       │       │  Geometry 层               │   │
│  │ Reset             │       │  ShapeStore signals ──────► │   │
│  └──────────────────┘       │  GeometryModule.dataChanged │   │
│                              └────────────────────────────┘   │
├────────────────────────────────────────────────────────────────┤
│  Command 层                                                    │
│                                                                │
│  CommandDispatcher                                             │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ onModuleDataChanged(name, callback) → ConnectionHandle   │ │
│  │ findModule(name) → shared_ptr<ModuleBase>                │ │
│  └──────────────────────────────────────────────────────────┘ │
│                         ↑ subscribe                            │
├─────────────────────────┼──────────────────────────────────────┤
│  App 层                 │                                      │
│                         │                                      │
│  ModuleDataNotifier (QObject)                                  │
│  ┌──────────────────────┴───────────────────────────────────┐ │
│  │ subscribe via CommandDispatcher::onModuleDataChanged()    │ │
│  │ callback → QMetaObject::invokeMethod(QueuedConnection)   │ │
│  │ emit geometryDataChanged()  [Qt signal, main thread]     │ │
│  └──────────────────────────────────────────────────────────┘ │
│                         │                                      │
├─────────────────────────┼──────────────────────────────────────┤
│  QML 层                 ▼                                      │
│                                                                │
│  SidebarPanel                                                  │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ Connections { target: ModuleDataNotifier }                │ │
│  │ Timer { interval: 100ms } → fetchShapeList()             │ │
│  │ RequestService.submitAsync("list_shapes") → shapeList    │ │
│  └──────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
```

**五层职责分离：**

| 层级 | 组件 | 职责 |
|------|------|------|
| Core | ModuleBase + ModuleDataEvent | 定义通用模块数据变更事件 |
| Geometry | GeometryModule | 连接 ShapeStore 信号 → dataChanged 事件 |
| Command | CommandDispatcher | 提供事件订阅 API，隔离模块实现细节 |
| App | ModuleDataNotifier | 桥接 Command 事件 → Qt 信号 |
| QML | SidebarPanel + ShapeListItem | 防抖、数据获取、UI 渲染 |

## 1. Core 层：ModuleDataEvent 枚举

### 1.1 文件位置

`src/libs/core/include/opengeolab/core/module_data_event.hpp`（新建）

### 1.2 定义

```cpp
#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

/// @brief 模块数据变更事件类型
///
/// 用于 ModuleBase::dataChanged 信号。各模块根据自身语义选择合适的事件类型：
/// - 几何模块：ShapeStore::shapeAdded → ItemAdded，tessellate → DerivedDataUpdated
/// - 网格模块：mesh 生成完成 → DerivedDataUpdated
/// - 分析模块：结果更新 → ItemModified
enum class ModuleDataEvent : uint8_t {
    ItemAdded,           ///< 新增数据项（创建、导入）
    ItemRemoved,         ///< 移除数据项
    ItemRenamed,         ///< 名称变更
    ItemModified,        ///< 数据/属性变更（几何操作、布尔运算、healing 等）
    DerivedDataUpdated,  ///< 派生数据更新（曲面化、网格剖分、分析结果等）
    BulkChanged,         ///< 批量变更（批导入、脚本批操作等）
    Reset                ///< 全量清空或替换
};

} // namespace OpenGeoLab::Core
```

## 2. Core 层：ModuleBase 新增 dataChanged 信号

### 2.1 修改文件

`src/libs/core/include/opengeolab/core/module.hpp`

### 2.2 新增成员

```cpp
#include <opengeolab/core/module_data_event.hpp>
#include <kangaroo/util/signal.hpp>

class ModuleBase {
    // ... existing interface ...

public:
    /// @brief 模块数据变更信号
    ///
    /// 模块在内部数据发生变化时发射此信号。
    /// 监听方通过 CommandDispatcher::onModuleDataChanged() 订阅，
    /// 不需要知道模块的具体类型。
    ///
    /// @note 信号可能在任意线程发射（如 worker thread），
    ///       监听方需自行处理线程安全（如 QueuedConnection）。
    Kangaroo::Util::Signal<ModuleDataEvent> dataChanged;
};
```

### 2.3 约束

- `dataChanged` 是公共成员，允许模块子类在任意时机 emit
- 信号参数为值语义，无生命周期问题
- 不强制要求所有模块使用此信号；未使用时不影响功能

## 3. Geometry 层：连接 ShapeStore → dataChanged

### 3.1 修改文件

`src/libs/geometry/src/geometry_module.cpp`（GeometryModule 构造函数）

### 3.2 实现

在 GeometryModule 构造函数中，注册 action 之后，连接 ShapeStore 信号：

```cpp
GeometryModule::GeometryModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(factory)
{
    // ... registerAction<...>(...) 调用 ...

    // 连接 ShapeStore 信号 → ModuleBase::dataChanged
    m_shapeStore.shapeAdded.connect(
        [this](uint32_t, const ShapeEntry&) {
            dataChanged.emit(Core::ModuleDataEvent::ItemAdded);
        });

    m_shapeStore.shapeRemoved.connect(
        [this](uint32_t) {
            dataChanged.emit(Core::ModuleDataEvent::ItemRemoved);
        });

    m_shapeStore.shapeUpdated.connect(
        [this](uint32_t, const ShapeEntry&) {
            dataChanged.emit(Core::ModuleDataEvent::ItemModified);
        });
}
```

### 3.3 未来扩展

- `TessellateAction` 完成后可额外 emit `DerivedDataUpdated`
- 批量导入完成后可 emit `BulkChanged`
- `ShapeStore::rename()` 被调用时可 emit `ItemRenamed`

> **注：** 当前 `ShapeStore::rename()` 触发 `shapeUpdated` 信号，因此被映射为 `ItemModified`。
> 这在当前阶段可以接受（sidebar 只需知道"有变更"即可刷新）。
> 未来如需区分 rename 与属性修改，可在 ShapeStore 中新增专用信号或在 GeometryModule 中拦截 rename 调用。

## 4. Command 层：CommandDispatcher 订阅 API

### 4.1 修改文件

- `src/libs/command/include/opengeolab/command/command_dispatcher.hpp`
- `src/libs/command/src/command_dispatcher.cpp`

### 4.2 新增接口

```cpp
/// @brief 订阅指定模块的数据变更事件
///
/// @param module_name 模块标识符（如 "geometry"）
/// @param callback 事件回调，可能在任意线程调用
/// @return 连接句柄（ScopedConnection）；销毁句柄时自动断开订阅。
///         如果模块未注册，返回默认构造的空句柄（isConnected() == false）。
[[nodiscard]] Kangaroo::Util::ScopedConnection
onModuleDataChanged(const std::string& module_name,
                    std::function<void(Core::ModuleDataEvent)> callback);
```

### 4.3 实现

```cpp
Kangaroo::Util::ScopedConnection
CommandDispatcher::onModuleDataChanged(
    const std::string& module_name,
    std::function<void(Core::ModuleDataEvent)> callback)
{
    auto module = findModule(module_name);
    if (!module) {
        return {};  // 模块未注册，返回空 ScopedConnection
    }
    return module->dataChanged.connect(std::move(callback));
}
```

### 4.4 设计说明

- 返回 `ConnectionHandle`：RAII 式生命周期管理，句柄销毁时自动断开
- 通过 `findModule()` 获取模块实例：复用已有单例缓存
- App 层只需 include `command_dispatcher.hpp`，无需知道 GeometryModule 类型

## 5. App 层：ModuleDataNotifier（QObject 桥接）

### 5.1 文件位置

- `src/app/include/opengeolab/app/module_data_notifier.h`（新建）
- `src/app/src/module_data_notifier.cpp`（新建）

### 5.2 接口设计

```cpp
#pragma once

#include <QObject>
#include <vector>
#include <kangaroo/util/signal.hpp>

namespace OpenGeoLab::Command { class CommandDispatcher; }

namespace OpenGeoLab::App {

/// @brief 模块数据变更 → Qt 信号桥接
///
/// 通过 CommandDispatcher 订阅模块事件，将 Kangaroo 信号
/// 以 QueuedConnection 方式转发为 Qt 信号，确保在主线程触发。
///
/// @note 生命周期必须晚于 CommandDispatcher，
///       析构时 ScopedConnection 自动断开所有订阅。
class ModuleDataNotifier : public QObject {
    Q_OBJECT

public:
    /// @param dispatcher CommandDispatcher 引用（生命周期由调用方保证）
    /// @param parent QObject parent
    explicit ModuleDataNotifier(Command::CommandDispatcher& dispatcher,
                                QObject* parent = nullptr);
    ~ModuleDataNotifier() override;

signals:
    /// @brief geometry 模块数据变更时在主线程发出
    void geometryDataChanged();

private:
    /// 存储 ScopedConnection，析构时自动断开
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::App
```

### 5.3 实现要点

```cpp
ModuleDataNotifier::ModuleDataNotifier(
    Command::CommandDispatcher& dispatcher, QObject* parent)
    : QObject(parent)
{
    // 订阅 geometry 模块事件
    auto handle = dispatcher.onModuleDataChanged(
        "geometry",
        [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(
                this,
                &ModuleDataNotifier::geometryDataChanged,
                Qt::QueuedConnection);
        });

    if (handle.isConnected()) {
        m_connections.push_back(std::move(handle));
    }
}

ModuleDataNotifier::~ModuleDataNotifier() = default;
// ConnectionHandle 析构时自动断开订阅
```

### 5.4 注册方式

在 `main.cpp` 中（紧跟 CommandDispatcher 创建之后）：

```cpp
OpenGeoLab::App::ModuleDataNotifier module_notifier(dispatcher);
qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0,
                             "ModuleDataNotifier", &module_notifier);
```

**注意：** App 层只 include `command_dispatcher.hpp` 和 `module_data_notifier.h`，
完全不 include geometry 层任何头文件。

### 5.5 线程安全分析

| 操作 | 线程 | 保护机制 |
|------|------|----------|
| ShapeStore.add/remove | worker (QtConcurrent) | ShapeStore 内部 mutex |
| Kangaroo signal emit (dataChanged) | worker | 无锁发射 |
| ModuleDataNotifier callback | worker | QueuedConnection 投递到主线程 |
| geometryDataChanged Qt signal | main thread | Qt event loop 保证顺序 |
| Timer/fetchShapeList | main thread | 单线程，无竞态 |
| list_shapes 执行 | worker | ShapeStore 内部 mutex 保护读取 |

### 5.6 扩展性

未来新增模块（如 mesh、analysis）时，只需在 `ModuleDataNotifier` 构造函数中增加一行订阅：

```cpp
auto mesh_handle = dispatcher.onModuleDataChanged(
    "mesh",
    [this](Core::ModuleDataEvent) {
        QMetaObject::invokeMethod(this, &ModuleDataNotifier::meshDataChanged,
                                  Qt::QueuedConnection);
    });
```

并新增对应的 Qt signal。无需修改 Command 或 Core 层。

## 6. 增强 list_shapes Action 返回数据

### 6.1 当前返回

```json
{
  "ok": true,
  "action": "list_shapes",
  "count": 2,
  "shapes": [
    { "shapeId": 0, "name": "Box_1", "hasTessellation": true,
      "topology": { "solids": 1, "faces": 6, "edges": 12, "vertices": 8 } }
  ]
}
```

### 6.2 增强后返回

```json
{
  "ok": true,
  "action": "list_shapes",
  "count": 2,
  "shapes": [
    {
      "shapeId": 0,
      "name": "Box_1",
      "shapeType": "Solid",
      "hasTessellation": true,
      "topology": {
        "solids": 1,
        "faces": 6,
        "edges": 12,
        "vertices": 8,
        "wires": 6
      },
      "boundingBox": {
        "min": [0.0, 0.0, 0.0],
        "max": [10.0, 20.0, 5.0]
      }
    }
  ]
}
```

**新增字段：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `shapeType` | string | OCC ShapeType 的人类可读名称：Compound, CompSolid, Solid, Shell, Face, Wire, Edge, Vertex |
| `topology.wires` | number | Wire 数量 |
| `boundingBox` | object | AABB 包围盒 min/max 坐标 |

### 6.3 实现方式

在 `list_shapes_action.cpp` 的循环中，对每个 ShapeEntry：

- 通过 `entry.shape.ShapeType()` 获取 `TopAbs_ShapeEnum` 并映射为字符串
- 通过 `Bnd_Box` + `BRepBndLib::Add()` 计算包围盒
- Wire 数量通过 `TopExp_Explorer(shape, TopAbs_WIRE)` 遍历统计

## 7. QML Sidebar UI

### 7.1 文件结构

| 文件 | 职责 |
|------|------|
| `sections/SidebarPanel.qml` | **修改** — 替换 boxListModel 为 shape list |
| `components/ShapeListItem.qml` | **新建** — 单个 shape 条目（折叠 + 展开） |
| `components/BoxListItem.qml` | **删除或保留** — 被 ShapeListItem 替代 |

### 7.2 SidebarPanel 改造

```qml
Item {
    id: root

    required property AppTheme theme

    // shape 列表数据（JS array）
    property var shapeList: []

    // 显隐状态（QML 端管理）
    property var visibilityState: ({})  // { shapeId: { geo: true, mesh: false } }

    // 防抖 Timer
    Timer {
        id: refreshTimer
        interval: 100
        repeat: false
        onTriggered: fetchShapeList()
    }

    // 监听 ModuleDataNotifier（通过 Command 层桥接）
    Connections {
        target: ModuleDataNotifier
        function onGeometryDataChanged() {
            refreshTimer.restart()
        }
    }

    // 监听 list_shapes 响应
    Connections {
        target: RequestService
        function onResponseReady(responseJson, muted) {
            const resp = JSON.parse(responseJson)
            if (resp.action === "list_shapes" && resp.ok) {
                root.shapeList = resp.shapes || []
            }
        }
    }

    function fetchShapeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "geometry",
            action: "list_shapes",
            param: {},
            mute: true
        }))
    }

    Component.onCompleted: fetchShapeList()

    // ... ListView using shapeList
}
```

### 7.3 ShapeListItem 设计

#### 折叠模式（单行，默认）

```
┌──────────────────────────────────────────────────┐
│ ■  #3  Box_1              👁  🔲         Solid   │
└──────────────────────────────────────────────────┘
 ↑   ↑    ↑                 ↑    ↑           ↑
颜色 ID  名称          几何可见 网格可见     类型标签
```

- **颜色块（■）**：4×16px 圆角矩形，从预定义调色板按 shapeId 分配颜色，预留与场景渲染对接
- **#ID**：shape ID，灰色小字
- **名称**：elide 超长文本
- **👁 按钮**：toggle 几何可见性（使用 SVG icon）
- **🔲 按钮**：toggle 网格可见性（使用 mesh.svg icon）
- **类型标签**：右对齐，灰色小字，如 Solid / Face / Compound

#### 展开模式（点击条目后）

```
│ ■  #3  Box_1              👁  🔲         Solid   │
│ ───────────────────────────────────────────────── │
│  Topology    6 Faces · 12 Edges · 8 Vertices     │
│  Bounds      10.0 × 20.0 × 5.0                   │
│  Tessellated ✅  (lin: 0.10 / ang: 0.50)          │
└──────────────────────────────────────────────────┘
```

- 展开用 `Column` + `NumberAnimation` on `height`
- 拓扑行：用 accent 色标签展示 F/E/V 数量
- 包围盒：显示 XYZ 尺寸（max - min）
- 曲面化状态：✅ 已完成 / ⏳ 未完成，附加参数

#### 颜色调色板

按 `shapeId % N` 循环分配，预定义 8 种颜色：

```javascript
const palette = [
    "#4FC3F7", "#81C784", "#FFB74D", "#E57373",
    "#BA68C8", "#4DB6AC", "#FFD54F", "#90A4AE"
]
```

### 7.4 显隐状态管理

几何/网格可见性是 UI 层关注点，存储在 QML 端 JS 对象中：

```javascript
property var visibilityState: ({})

function toggleGeoVisibility(shapeId) {
    if (!(shapeId in visibilityState)) {
        visibilityState[shapeId] = { geo: true, mesh: false }
    }
    visibilityState[shapeId].geo = !visibilityState[shapeId].geo
    visibilityStateChanged()
    // 后续对接场景渲染时，通知 SceneGraph 更新
}

function toggleMeshVisibility(shapeId) {
    if (!(shapeId in visibilityState)) {
        visibilityState[shapeId] = { geo: true, mesh: false }
    }
    visibilityState[shapeId].mesh = !visibilityState[shapeId].mesh
    visibilityStateChanged()
}
```

**默认值：** 新 shape 默认 `geo: true, mesh: false`

### 7.5 空态设计

当 shapeList 为空时：

```
┌──────────────────────────────────┐
│        📦                         │
│  No geometry loaded.             │
│  Create a shape or import        │
│  a model to get started.         │
└──────────────────────────────────┘
```

使用 `cubeOutline.svg` 图标 + 引导文案。

## 8. 信号流完整生命周期

### 8.1 用户通过 UI 创建 Box

```
1. QML CreateBoxPage → RequestService.submitAsync({module:"geometry", action:"create_box", ...})
2. Worker thread: CommandDispatcher.dispatch() → GeometryModule → CreateBoxAction.execute()
3. CreateBoxAction → ShapeStore.add() → Kangaroo shapeAdded.emit()
4. GeometryModule lambda → dataChanged.emit(ItemAdded)
5. ModuleDataNotifier callback (worker thread) → QueuedConnection → main thread
6. QML Connections → onGeometryDataChanged → Timer restart (100ms 防抖)
7. Timer triggered → RequestService.submitAsync("list_shapes")
8. Worker thread: ListShapesAction → 读取 ShapeStore → 返回增强 JSON
9. QML onResponseReady → 解析 JSON → 更新 shapeList → ListView 自动刷新
```

### 8.2 Python 脚本批量创建

```
1. Python: runtime.dispatch({"module":"geometry","action":"create_box",...}) × 10 次
2. 每次 ShapeStore.add() → shapeAdded → GeometryModule.dataChanged(ItemAdded)
3. ModuleDataNotifier → geometryDataChanged (投递到主线程)
4. 前 100ms 内所有 dirty 信号被 Timer 合并
5. Timer triggered → 单次 list_shapes 获取全部 shape
6. ListView 一次性刷新，不会闪烁或卡 UI
```

## 9. Main.qml 改造

### 9.1 移除 boxListModel

当前 `Main.qml` 中有：

```qml
ListModel { id: boxListModel }
```

以及 `SidebarPanel` 绑定 `boxListModel: boxListModel`。

改造后：
- 移除 `boxListModel` 声明
- `SidebarPanel` 不再需要外部传入 model，内部自管理

### 9.2 启动时自动刷新

`SidebarPanel.Component.onCompleted` 中发一次 `list_shapes` 确保初始状态同步。

## 10. 翻译要求

新增的用户可见字符串必须使用 `qsTr()`。需要在 `opengeolab_zh_CN.ts` 中添加对应翻译。

**主要翻译条目：**

| 英文 | 中文 |
|------|------|
| "Scene" | "场景" |
| "Explorer" | "浏览器" |
| "No geometry loaded." | "暂无几何体。" |
| "Create a shape or import a model to get started." | "创建形状或导入模型以开始。" |
| "Topology" | "拓扑" |
| "Bounds" | "包围盒" |
| "Tessellated" | "已曲面化" |
| "Not tessellated" | "未曲面化" |
| "%1 Faces" | "%1 个面" |
| "%1 Edges" | "%1 条边" |
| "%1 Vertices" | "%1 个顶点" |

## 11. 文件变更清单

### 新建文件

| 文件 | 职责 |
|------|------|
| `src/libs/core/include/opengeolab/core/module_data_event.hpp` | ModuleDataEvent 枚举定义 |
| `src/app/include/opengeolab/app/module_data_notifier.h` | ModuleDataNotifier 声明 |
| `src/app/src/module_data_notifier.cpp` | ModuleDataNotifier 实现 |
| `src/app/resource/qml/components/ShapeListItem.qml` | Shape 条目 UI 组件 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `src/libs/core/include/opengeolab/core/module.hpp` | 新增 `dataChanged` 信号成员 |
| `src/libs/geometry/src/geometry_module.cpp` | 连接 ShapeStore 信号 → dataChanged |
| `src/libs/command/include/opengeolab/command/command_dispatcher.hpp` | 新增 `onModuleDataChanged()` |
| `src/libs/command/src/command_dispatcher.cpp` | `onModuleDataChanged()` 实现 |
| `src/libs/geometry/src/list_shapes_action.cpp` | 增加 shapeType / boundingBox / wires 字段 |
| `src/app/src/main.cpp` | 创建 ModuleDataNotifier 并注册为 QML 单例 |
| `src/app/CMakeLists.txt` | 新增 .h/.cpp 和 QML 文件 |
| `src/app/resource/qml/sections/SidebarPanel.qml` | 替换 boxListModel → shape list 自管理 |
| `src/app/resource/qml/Main.qml` | 移除 boxListModel |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | 新增翻译条目 |

### 可能删除

| 文件 | 原因 |
|------|------|
| `src/app/resource/qml/components/BoxListItem.qml` | 被 ShapeListItem 完全替代 |

## 12. 测试策略

| 测试类型 | 覆盖范围 |
|----------|----------|
| C++ 单元测试 | ModuleDataEvent 枚举值正确性 |
| C++ 单元测试 | CommandDispatcher::onModuleDataChanged() 订阅与回调 |
| C++ 单元测试 | ModuleDataNotifier QueuedConnection 线程桥接（需 QCoreApplication） |
| C++ 单元测试 | list_shapes 增强字段正确性（shapeType、boundingBox、wires） |
| 手动验证 | 创建 box → sidebar 自动显示条目 |
| 手动验证 | import model → sidebar 自动刷新 |
| 手动验证 | 折叠/展开交互正常 |
| 手动验证 | 几何/网格显隐 toggle 状态正确 |
| 手动验证 | 空态 → 有数据 → 删除至空态的全流程 |

## 13. 设计约束与边界

- **不在本次实现：** 渲染联动（颜色块和显隐按钮目前只维护 UI 状态，不实际控制场景渲染）
- **不在本次实现：** 拾取高亮联动（选中 sidebar 条目不联动 viewport）
- **不在本次实现：** shape 重命名/编辑（条目只读展示）
- **不在本次实现：** mesh/analysis 模块的 ModuleDataNotifier 订阅（预留了扩展点）
- **颜色块**预留接口：后续对接场景渲染时，颜色和显隐状态将通过 SceneGraph 桥接层传递
- **防抖间隔**：100ms，可根据实际体验调整（50ms–200ms 区间均合理）
- **App 层零 Geometry 依赖**：App 只 include Command 层头文件，通过 `onModuleDataChanged()` 订阅事件
