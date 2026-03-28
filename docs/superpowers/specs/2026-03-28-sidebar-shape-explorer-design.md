# Sidebar Shape Explorer 设计规格

## 目标

在左侧 SidebarPanel 中展示当前 ShapeStore 中所有顶层 shape 的实时列表，支持折叠/展开详情、几何与网格显隐控制，并通过 push 通知机制确保 UI、Python 脚本或任意来源的 shape 变更都能反映到 sidebar。

## 架构概览

```
ShapeStore (worker thread)
  ── Kangaroo::Signal ──→  ShapeStoreNotifier (QObject, app 层)
                              ── QMetaObject::invokeMethod(QueuedConnection) ──→
                                    Qt signal: shapeListDirty()  [main thread]
                                         │
                                    QML SidebarPanel
                                      ├─ Timer { interval: 100ms }  (防抖)
                                      └─ RequestService.submitAsync("list_shapes")
                                              │
                                         responseReady → 更新 JS ListModel
```

**三层职责分离：**

| 层级 | 组件 | 职责 |
|------|------|------|
| C++ geometry | ShapeStore | 数据持有，发 Kangaroo 信号 |
| C++ app | ShapeStoreNotifier | 桥接 Kangaroo → Qt 信号，线程安全 marshal |
| QML | SidebarPanel + ShapeListItem | 防抖、数据获取、UI 渲染 |

## 1. ShapeStoreNotifier（C++ 桥接层）

### 1.1 文件位置

- `src/app/include/opengeolab/app/shape_store_notifier.h`
- `src/app/src/shape_store_notifier.cpp`

### 1.2 接口设计

```cpp
class ShapeStoreNotifier : public QObject {
    Q_OBJECT

public:
    explicit ShapeStoreNotifier(OpenGeoLab::Geometry::ShapeStore& store,
                                QObject* parent = nullptr);
    ~ShapeStoreNotifier() override;

signals:
    /// 任何 shape 变更（add/remove/rename/tessellate）后在主线程发出
    void shapeListDirty();

private:
    OpenGeoLab::Geometry::ShapeStore& m_store;
};
```

### 1.3 实现要点

- 构造函数中连接 ShapeStore 的 3 个 Kangaroo 信号（`shapeAdded`、`shapeRemoved`、`shapeUpdated`）
- 回调内部使用 `QMetaObject::invokeMethod(this, &ShapeStoreNotifier::shapeListDirty, Qt::QueuedConnection)` 确保 Qt 信号在主线程发出
- 析构函数中断开 Kangaroo 信号连接
- 不传递 shape 数据，仅通知"脏了"

### 1.4 线程安全分析

- Kangaroo 信号在 worker 线程（QtConcurrent）触发
- `QueuedConnection` 将事件投递到 QObject 所属线程的事件队列（主线程）
- QML 端在主线程处理信号，无竞态
- ShapeStore 内部已有 mutex 保护，`list_shapes` action 的读取是安全的

### 1.5 注册方式

在 `main.cpp` 中：

```cpp
// 获取 ShapeStore 引用（需要暴露从 runtime 获取的路径）
auto& shape_store = /* 从 EmbeddedPythonRuntime 获取 GeometryModule::shapeStore() */;

OpenGeoLab::App::ShapeStoreNotifier shape_notifier(shape_store);
qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "ShapeStoreNotifier", &shape_notifier);
```

### 1.6 ShapeStore 访问路径

需要新增一条从 `EmbeddedPythonRuntime` → `CommandDispatcher` → `GeometryModule` → `ShapeStore` 的访问链：

- `CommandDispatcher` 新增公共方法 `findModule(name)` 返回 `shared_ptr<ModuleBase>`
- `EmbeddedPythonRuntime` 暴露 `CommandDispatcher&` 的只读访问
- 在 `main.cpp` 中获取 GeometryModule 并 `dynamic_cast` 取 `shapeStore()` 引用

## 2. 增强 list_shapes Action 返回数据

### 2.1 当前返回

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

### 2.2 增强后返回

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

### 2.3 实现方式

在 `list_shapes_action.cpp` 的循环中，对每个 ShapeEntry：

- 通过 `entry.shape.ShapeType()` 获取 `TopAbs_ShapeEnum` 并映射为字符串
- 通过 `Bnd_Box` + `BRepBndLib::Add()` 计算包围盒
- Wire 数量通过 `TopExp_Explorer(shape, TopAbs_WIRE)` 遍历统计

## 3. QML Sidebar UI

### 3.1 文件结构

| 文件 | 职责 |
|------|------|
| `sections/SidebarPanel.qml` | **修改** — 替换 boxListModel 为 shape list |
| `components/ShapeListItem.qml` | **新建** — 单个 shape 条目（折叠 + 展开） |
| `components/BoxListItem.qml` | **删除或保留** — 被 ShapeListItem 替代 |

### 3.2 SidebarPanel 改造

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

    // 监听 ShapeStoreNotifier
    Connections {
        target: ShapeStoreNotifier
        function onShapeListDirty() {
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

### 3.3 ShapeListItem 设计

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

### 3.4 显隐状态管理

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

### 3.5 空态设计

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

## 4. 信号流完整生命周期

### 4.1 用户通过 UI 创建 Box

```
1. QML CreateBoxPage → RequestService.submitAsync({module:"geometry", action:"create_box", ...})
2. Worker thread: CommandDispatcher → GeometryModule → CreateBoxAction.execute()
3. CreateBoxAction → ShapeStore.add() → Kangaroo shapeAdded.emit()
4. ShapeStoreNotifier callback (worker thread) → QueuedConnection → main thread
5. QML Connections → onShapeListDirty → Timer restart (100ms 防抖)
6. Timer triggered → RequestService.submitAsync("list_shapes")
7. Worker thread: ListShapesAction → 读取 ShapeStore → 返回 JSON
8. QML onResponseReady → 解析 JSON → 更新 shapeList → ListView 自动刷新
```

### 4.2 Python 脚本批量创建

```
1. Python: runtime.dispatch({"module":"geometry","action":"create_box",...}) × 10 次
2. 每次 ShapeStore.add() → shapeAdded → ShapeStoreNotifier → shapeListDirty
3. 前 100ms 内所有 dirty 信号被 Timer 合并
4. Timer triggered → 单次 list_shapes 获取全部 shape
5. ListView 一次性刷新，不会闪烁或卡 UI
```

### 4.3 线程安全保证

| 操作 | 线程 | 保护机制 |
|------|------|----------|
| ShapeStore.add/remove | worker (QtConcurrent) | ShapeStore 内部 mutex |
| Kangaroo signal emit | worker | 无锁发射 |
| ShapeStoreNotifier 回调 | worker | QueuedConnection 投递到主线程 |
| shapeListDirty Qt signal | main thread | Qt event loop 保证顺序 |
| Timer/fetchShapeList | main thread | 单线程，无竞态 |
| list_shapes 执行 | worker | ShapeStore 内部 mutex 保护读取 |

## 5. Main.qml 改造

### 5.1 移除 boxListModel

当前 `Main.qml` 中有：

```qml
ListModel { id: boxListModel }
```

以及 `SidebarPanel` 绑定 `boxListModel: boxListModel`。

改造后：
- 移除 `boxListModel` 声明
- `SidebarPanel` 不再需要外部传入 model，内部自管理

### 5.2 启动时自动刷新

`SidebarPanel.Component.onCompleted` 中发一次 `list_shapes` 确保初始状态同步。

## 6. 翻译要求

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

## 7. 文件变更清单

### 新建文件

| 文件 | 职责 |
|------|------|
| `src/app/include/opengeolab/app/shape_store_notifier.h` | ShapeStoreNotifier 声明 |
| `src/app/src/shape_store_notifier.cpp` | ShapeStoreNotifier 实现 |
| `src/app/resource/qml/components/ShapeListItem.qml` | Shape 条目 UI 组件 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `src/app/src/main.cpp` | 创建 ShapeStoreNotifier 并注册为 QML 单例 |
| `src/app/CMakeLists.txt` | 新增 .h/.cpp 和 QML 文件 |
| `src/app/resource/qml/sections/SidebarPanel.qml` | 替换 boxListModel → shape list 自管理 |
| `src/app/resource/qml/Main.qml` | 移除 boxListModel |
| `src/libs/geometry/src/list_shapes_action.cpp` | 增加 shapeType / boundingBox / wires 字段 |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | 新增翻译条目 |
| `src/libs/command/include/opengeolab/command/command_dispatcher.hpp` | 新增 `findModule()` 公共接口 |
| `src/libs/command/src/command_dispatcher.cpp` | `findModule()` 实现 |

### 可能删除

| 文件 | 原因 |
|------|------|
| `src/app/resource/qml/components/BoxListItem.qml` | 被 ShapeListItem 完全替代 |

## 8. 暴露 CommandDispatcher 的设计

### 8.1 CommandDispatcher 新增公共接口

```cpp
/**
 * @brief Retrieves a module by name, creating and caching it if necessary.
 * @param module_name Module identifier (e.g. "geometry")
 * @return Shared pointer to the module, or nullptr if not registered
 */
[[nodiscard]] std::shared_ptr<Core::ModuleBase> findModule(const std::string& module_name);
```

此方法复用已有的 `getModule()` 私有逻辑，仅需将其改为公共并保持缓存语义。

### 8.2 EmbeddedPythonRuntime 暴露 Dispatcher

需要在 `EmbeddedPythonRuntime` 中新增一个访问 `CommandDispatcher` 的方法（只读引用或返回 dispatcher 的 const 引用），使 `main.cpp` 可以获取 dispatcher → findModule("geometry") → shapeStore()。

具体实现取决于 EmbeddedPythonRuntime 的内部结构，可能需要：
- `CommandDispatcher& dispatcher()` 访问器
- 或 `std::shared_ptr<ModuleBase> findModule(const std::string&)` 便捷转发

## 9. 测试策略

| 测试类型 | 覆盖范围 |
|----------|----------|
| C++ 单元测试 | list_shapes 增强字段正确性（shapeType、boundingBox） |
| C++ 单元测试 | findModule() 返回正确的缓存模块 |
| 手动验证 | 创建 box → sidebar 自动显示条目 |
| 手动验证 | import model → sidebar 自动刷新 |
| 手动验证 | 折叠/展开交互正常 |
| 手动验证 | 几何/网格显隐 toggle 状态正确 |
| 手动验证 | 空态 → 有数据 → 删除至空态的全流程 |

## 10. 设计约束与边界

- **不在本次实现：** 渲染联动（颜色块和显隐按钮目前只维护 UI 状态，不实际控制场景渲染）
- **不在本次实现：** 拾取高亮联动（选中 sidebar 条目不联动 viewport）
- **不在本次实现：** shape 重命名/编辑（条目只读展示）
- **颜色块**预留接口：后续对接场景渲染时，颜色和显隐状态将通过 SceneGraph 桥接层传递
- **防抖间隔**：100ms，可根据实际体验调整（50ms–200ms 区间均合理）
