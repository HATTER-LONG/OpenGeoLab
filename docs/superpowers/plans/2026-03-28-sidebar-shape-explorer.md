# Sidebar Shape Explorer 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 实现左侧 Sidebar Shape Explorer，通过 Command 层 EventBus 抽象实时展示 ShapeStore 中的几何体列表。

**架构：** Core 层新增 ModuleDataEvent 枚举和 ModuleBase::dataChanged 信号。Command 层新增 onModuleDataChanged() 订阅 API。App 层 ModuleDataNotifier 桥接 Kangaroo → Qt 信号。QML SidebarPanel 通过防抖 Timer + list_shapes 全量刷新。App 层零 Geometry 依赖。

**技术栈：** C++20, Qt 6.9, QML, Kangaroo::Util::Signal, OCC (BRepBndLib, TopExp_Explorer), doctest

**规格文档：** `docs/superpowers/specs/2026-03-28-sidebar-shape-explorer-design.md` (v2)

---

## 文件变更总览

### 新建文件

| 文件 | 职责 |
|------|------|
| `src/libs/core/include/opengeolab/core/module_data_event.hpp` | ModuleDataEvent 枚举（7 值） |
| `src/app/include/opengeolab/app/module_data_notifier.h` | ModuleDataNotifier QObject 声明 |
| `src/app/src/module_data_notifier.cpp` | ModuleDataNotifier 实现 |
| `src/app/resource/qml/components/ShapeListItem.qml` | Shape 条目 UI 组件 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `src/libs/core/include/opengeolab/core/module.hpp` | 新增 `#include` 和 `dataChanged` 信号成员 |
| `src/libs/core/CMakeLists.txt` | 新增 `module_data_event.hpp` 到 public headers |
| `src/libs/geometry/src/geometry_module.cpp` | 连接 ShapeStore 信号 → dataChanged |
| `src/libs/geometry/src/list_shapes_action.cpp` | 增加 shapeType / boundingBox / wires |
| `src/libs/command/include/opengeolab/command/command_dispatcher.hpp` | 新增 `onModuleDataChanged()` |
| `src/libs/command/src/command_dispatcher.cpp` | `onModuleDataChanged()` 实现 |
| `src/libs/command/test/command_dispatcher_test.cpp` | 新增 onModuleDataChanged 测试 |
| `src/app/src/main.cpp` | 创建 ModuleDataNotifier 并注册为 QML 单例 |
| `src/app/CMakeLists.txt` | 新增源文件和 QML 文件 |
| `src/app/resource/qml/sections/SidebarPanel.qml` | 替换 boxListModel → shape list 自管理 |
| `src/app/resource/qml/Main.qml` | 移除 boxListModel |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | 新增翻译条目 |

---

## 任务 0：基线验证

**文件：** 无修改

- [ ] 步骤 1：构建全量 `cmake --build build --config RelWithDebInfo --parallel 8`
- [ ] 步骤 2：测试 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 3：确认 8/8 测试通过
- [ ] 步骤 4：`git --no-pager status` 确认工作树干净

**预期结果：** 8/8 tests pass，无未提交更改

---

## 任务 1：Core 层 — ModuleDataEvent 枚举 + ModuleBase::dataChanged 信号

**文件：**
- 新增：`src/libs/core/include/opengeolab/core/module_data_event.hpp`
- 修改：`src/libs/core/include/opengeolab/core/module.hpp`
- 修改：`src/libs/core/CMakeLists.txt`

- [ ] 步骤 1：创建 `module_data_event.hpp`

```cpp
/// @file module_data_event.hpp
/// @brief ModuleDataEvent — generic data-change event types for modules
#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

/// @brief 模块数据变更事件类型
///
/// 用于 ModuleBase::dataChanged 信号。各模块根据自身语义选择合适的事件类型。
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

- [ ] 步骤 2：修改 `module.hpp`，在 include 区域（第 16 行后）添加：

```cpp
#include <opengeolab/core/module_data_event.hpp>
#include <kangaroo/util/signal.hpp>
```

在 `ModuleBase` class 的 public 区域（第 48 行 `virtual ~ModuleBase();` 之后）添加：

```cpp
    /// @brief 模块数据变更信号
    ///
    /// 模块在内部数据发生变化时发射此信号。
    /// 监听方通过 CommandDispatcher::onModuleDataChanged() 订阅。
    ///
    /// @note 信号可能在任意线程发射，监听方需自行处理线程安全。
    Kangaroo::Util::Signal<ModuleDataEvent> dataChanged;
```

- [ ] 步骤 3：修改 `src/libs/core/CMakeLists.txt` 第 1-8 行的 `core_public_headers`，添加：

```cmake
    include/opengeolab/core/module_data_event.hpp
```

（在 `module.hpp` 之后添加）

- [ ] 步骤 4：构建 `cmake --build build --config RelWithDebInfo --parallel 8`
- [ ] 步骤 5：测试 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`（预期 8/8 pass，新增类型无行为变更）
- [ ] 步骤 6：`clang-format -i src/libs/core/include/opengeolab/core/module_data_event.hpp src/libs/core/include/opengeolab/core/module.hpp`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
**预期结果：** 构建成功，8/8 tests pass

---

## 任务 2：Geometry 层 — 连接 ShapeStore 信号 → dataChanged

**文件：**
- 修改：`src/libs/geometry/src/geometry_module.cpp`

- [ ] 步骤 1：在 `geometry_module.cpp` 构造函数中（第 33 行 `registerAction<DeleteShapeAction>` 之后，第 34 行 `}` 之前），添加 ShapeStore 信号连接：

```cpp
    // Bridge ShapeStore signals → ModuleBase::dataChanged for event bus
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
```

需要在文件顶部（第 17 行 `#include <functional>` 之后）添加：

```cpp
#include <opengeolab/core/module_data_event.hpp>
```

- [ ] 步骤 2：构建 geometry 目标 `cmake --build build --target opengeolab_geometry --config RelWithDebInfo --parallel 8`
- [ ] 步骤 3：运行 geometry 测试 `ctest --test-dir build -C RelWithDebInfo -R geometry --output-on-failure`
- [ ] 步骤 4：`clang-format -i src/libs/geometry/src/geometry_module.cpp`

**验证命令：** `cmake --build build --target opengeolab_geometry --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo -R geometry --output-on-failure`
**预期结果：** 构建成功，geometry 测试全部通过

---

## 任务 3：Command 层 — onModuleDataChanged() 订阅 API + 测试

**文件：**
- 修改：`src/libs/command/include/opengeolab/command/command_dispatcher.hpp`
- 修改：`src/libs/command/src/command_dispatcher.cpp`
- 修改：`src/libs/command/test/command_dispatcher_test.cpp`

- [ ] 步骤 1：写失败测试。在 `command_dispatcher_test.cpp` 末尾添加：

```cpp
TEST_CASE("onModuleDataChanged fires callback when module emits dataChanged") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Command::registerBuiltinModules(factory);
    OpenGeoLab::Command::CommandDispatcher dispatcher(factory);

    int callCount = 0;
    OpenGeoLab::Core::ModuleDataEvent lastEvent{};
    auto conn = dispatcher.onModuleDataChanged(
        "geometry",
        [&](OpenGeoLab::Core::ModuleDataEvent event) {
            ++callCount;
            lastEvent = event;
        });

    CHECK(conn.isConnected());

    // Trigger via create_box (which calls ShapeStore::add → dataChanged)
    nlohmann::json req = {{"module", "geometry"},
                          {"action", "create_box"},
                          {"param", {{"width", 1.0}, {"height", 1.0}, {"depth", 1.0}}}};
    auto result = dispatcher.dispatch(req, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(callCount == 1);
    CHECK(lastEvent == OpenGeoLab::Core::ModuleDataEvent::ItemAdded);
}

TEST_CASE("onModuleDataChanged returns disconnected handle for unknown module") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Command::CommandDispatcher dispatcher(factory);

    auto conn = dispatcher.onModuleDataChanged(
        "nonexistent", [](OpenGeoLab::Core::ModuleDataEvent) {});
    CHECK_FALSE(conn.isConnected());
}
```

需要在测试文件顶部添加 include：

```cpp
#include <opengeolab/core/module_data_event.hpp>
```

- [ ] 步骤 2：运行测试确认失败（`onModuleDataChanged` 尚未实现）

```
cmake --build build --target opengeolab_command_test --config RelWithDebInfo --parallel 8
```

（预期：编译失败，因为 `onModuleDataChanged` 不存在）

- [ ] 步骤 3：在 `command_dispatcher.hpp` 中添加声明。

在现有 include 区域（第 26 行 `#include <nlohmann/json.hpp>` 之后）添加：

```cpp
#include <kangaroo/util/signal.hpp>
```

在 `findModule()` 声明之后（第 96 行之后），添加：

```cpp
    /// @brief Subscribe to data-change events from a named module.
    ///
    /// @param module_name Module identifier (e.g. "geometry")
    /// @param callback    Invoked on the emitting thread (may be a worker thread)
    /// @return ScopedConnection that auto-disconnects on destruction;
    ///         default-constructed (isConnected() == false) if module not found.
    [[nodiscard]] Kangaroo::Util::ScopedConnection
    onModuleDataChanged(const std::string& module_name,
                        std::function<void(Core::ModuleDataEvent)> callback);
```

需要在 include 区域添加 `#include <functional>`（如果不存在）。

- [ ] 步骤 4：在 `command_dispatcher.cpp` 中实现。在文件末尾（`findModule()` 实现之后，namespace 闭合之前）添加：

```cpp
Kangaroo::Util::ScopedConnection
CommandDispatcher::onModuleDataChanged(
    const std::string& module_name,
    std::function<void(Core::ModuleDataEvent)> callback)
{
    auto module = findModule(module_name);
    if(!module) {
        return {};
    }
    return module->dataChanged.connect(std::move(callback));
}
```

- [ ] 步骤 5：构建并运行测试

```
cmake --build build --target opengeolab_command_test --config RelWithDebInfo --parallel 8
ctest --test-dir build -C RelWithDebInfo -R command --output-on-failure
```

- [ ] 步骤 6：`clang-format -i src/libs/command/include/opengeolab/command/command_dispatcher.hpp src/libs/command/src/command_dispatcher.cpp src/libs/command/test/command_dispatcher_test.cpp`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
**预期结果：** 所有测试通过（含 2 个新 command 测试）

---

## 任务 4：list_shapes 增强 — shapeType / boundingBox / wires

**文件：**
- 修改：`src/libs/geometry/src/list_shapes_action.cpp`
- 修改：`src/libs/geometry/test/geometry_module_test.cpp`

- [ ] 步骤 1：写失败测试。在 `geometry_module_test.cpp` 末尾添加：

```cpp
TEST_CASE("list_shapes returns enhanced fields: shapeType, boundingBox, wires") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);

    // Create a box first
    nlohmann::json createReq = {{"module", "geometry"},
                                {"action", "create_box"},
                                {"param", {{"width", 10.0}, {"height", 20.0}, {"depth", 5.0}}}};
    auto createResult = mod.process(createReq, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    REQUIRE(createResult["ok"] == true);

    // List shapes
    nlohmann::json listReq = {{"module", "geometry"},
                              {"action", "list_shapes"},
                              {"param", nlohmann::json::object()}};
    auto listResult = mod.process(listReq, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    REQUIRE(listResult["ok"] == true);
    REQUIRE(listResult["shapes"].size() == 1);

    auto& shape = listResult["shapes"][0];

    // shapeType
    CHECK(shape.contains("shapeType"));
    CHECK(shape["shapeType"].is_string());
    CHECK(shape["shapeType"] == "Solid");

    // topology.wires
    CHECK(shape["topology"].contains("wires"));
    CHECK(shape["topology"]["wires"].is_number());
    CHECK(shape["topology"]["wires"] == 6);

    // boundingBox
    CHECK(shape.contains("boundingBox"));
    CHECK(shape["boundingBox"].contains("min"));
    CHECK(shape["boundingBox"].contains("max"));
    CHECK(shape["boundingBox"]["min"].is_array());
    CHECK(shape["boundingBox"]["min"].size() == 3);
    CHECK(shape["boundingBox"]["max"].is_array());
    CHECK(shape["boundingBox"]["max"].size() == 3);

    // Box at origin with w=10, h=20, d=5 → min ~[0,0,0], max ~[10,20,5]
    auto maxBB = shape["boundingBox"]["max"];
    CHECK(maxBB[0].get<double>() == doctest::Approx(10.0).epsilon(0.01));
    CHECK(maxBB[1].get<double>() == doctest::Approx(20.0).epsilon(0.01));
    CHECK(maxBB[2].get<double>() == doctest::Approx(5.0).epsilon(0.01));
}
```

- [ ] 步骤 2：构建测试确认失败

```
cmake --build build --target opengeolab_geometry_module_test --config RelWithDebInfo --parallel 8
ctest --test-dir build -C RelWithDebInfo -R geometry_module --output-on-failure
```

（预期：测试失败，因为 shapeType/boundingBox/wires 字段不存在）

- [ ] 步骤 3：修改 `list_shapes_action.cpp`。在文件顶部（第 7 行之后）添加 include：

```cpp
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
```

添加辅助函数（在 namespace 内，`ListShapesAction::ListShapesAction` 之前）：

```cpp
namespace {

/// Map OCC ShapeType enum to human-readable string.
const char* shapeTypeToString(TopAbs_ShapeEnum type) {
    switch(type) {
    case TopAbs_COMPOUND:  return "Compound";
    case TopAbs_COMPSOLID: return "CompSolid";
    case TopAbs_SOLID:     return "Solid";
    case TopAbs_SHELL:     return "Shell";
    case TopAbs_FACE:      return "Face";
    case TopAbs_WIRE:      return "Wire";
    case TopAbs_EDGE:      return "Edge";
    case TopAbs_VERTEX:    return "Vertex";
    default:               return "Shape";
    }
}

} // anonymous namespace
```

修改 JSON 构建循环（第 31-38 行），替换为：

```cpp
            // Compute bounding box
            Bnd_Box bbox;
            BRepBndLib::Add(entry->shape, bbox);
            double xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
            if(!bbox.IsVoid()) {
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            }

            shapes.push_back(
                {{"shapeId", id},
                 {"name", entry->name},
                 {"shapeType", shapeTypeToString(entry->shape.ShapeType())},
                 {"hasTessellation", entry->visualData != nullptr},
                 {"topology",
                  {{"solids", entry->solidMap.Extent()},
                   {"faces", entry->faceMap.Extent()},
                   {"edges", entry->edgeMap.Extent()},
                   {"vertices", entry->vertexMap.Extent()},
                   {"wires", entry->wireMap.Extent()}}},
                 {"boundingBox",
                  {{"min", {xmin, ymin, zmin}},
                   {"max", {xmax, ymax, zmax}}}}});
```

- [ ] 步骤 4：构建并运行测试确认通过

```
cmake --build build --target opengeolab_geometry_module_test --config RelWithDebInfo --parallel 8
ctest --test-dir build -C RelWithDebInfo -R geometry_module --output-on-failure
```

- [ ] 步骤 5：`clang-format -i src/libs/geometry/src/list_shapes_action.cpp src/libs/geometry/test/geometry_module_test.cpp`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
**预期结果：** 所有测试通过

---

## 任务 5：App 层 — ModuleDataNotifier + main.cpp 注册

**文件：**
- 新增：`src/app/include/opengeolab/app/module_data_notifier.h`
- 新增：`src/app/src/module_data_notifier.cpp`
- 修改：`src/app/src/main.cpp`
- 修改：`src/app/CMakeLists.txt`

**注：** 此任务不使用 TDD，因为 ModuleDataNotifier 的核心行为（QueuedConnection 跨线程投递）需要 QCoreApplication 事件循环，不适合纯单元测试。验证通过手动集成测试和编译通过。

- [ ] 步骤 1：创建 `module_data_notifier.h`

```cpp
/// @file module_data_notifier.h
/// @brief ModuleDataNotifier — bridges module data-change events to Qt signals
#pragma once

#include <kangaroo/util/signal.hpp>

#include <QObject>

#include <vector>

namespace OpenGeoLab::Command {
class CommandDispatcher;
}

namespace OpenGeoLab::App {

/// @brief Bridges Kangaroo module data-change events to Qt signals.
///
/// Subscribes to module events via CommandDispatcher::onModuleDataChanged()
/// and forwards them as Qt signals using QueuedConnection, ensuring they
/// arrive on the main thread.
///
/// @note Lifetime must exceed the CommandDispatcher it subscribes to.
///       ScopedConnection handles auto-disconnect on destruction.
class ModuleDataNotifier : public QObject {
    Q_OBJECT

public:
    /// @param dispatcher CommandDispatcher reference (caller owns lifetime)
    /// @param parent QObject parent
    explicit ModuleDataNotifier(Command::CommandDispatcher& dispatcher,
                                QObject* parent = nullptr);
    ~ModuleDataNotifier() override;

signals:
    /// @brief Emitted on main thread when geometry module data changes.
    void geometryDataChanged();

private:
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::App
```

- [ ] 步骤 2：创建 `module_data_notifier.cpp`

```cpp
/// @file module_data_notifier.cpp
/// @brief ModuleDataNotifier implementation
#include "opengeolab/app/module_data_notifier.h"

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/core/module_data_event.hpp>

namespace OpenGeoLab::App {

ModuleDataNotifier::ModuleDataNotifier(Command::CommandDispatcher& dispatcher,
                                       QObject* parent)
    : QObject(parent) {
    auto handle = dispatcher.onModuleDataChanged(
        "geometry",
        [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(
                this, &ModuleDataNotifier::geometryDataChanged, Qt::QueuedConnection);
        });

    if(handle.isConnected()) {
        m_connections.push_back(std::move(handle));
    }
}

ModuleDataNotifier::~ModuleDataNotifier() = default;

} // namespace OpenGeoLab::App
```

- [ ] 步骤 3：修改 `src/app/CMakeLists.txt`

在 `qt_add_executable` 中（第 9-11 行）添加新源文件：

```cmake
qt_add_executable(
    opengeolab_app src/main.cpp src/translation_manager.cpp
    src/request_service.cpp src/log_event_model.cpp
    src/log_filter_proxy_model.cpp src/module_data_notifier.cpp)
```

在 `qt_add_qml_module` 的 `SOURCES` 部分（第 41-45 行）添加头文件：

```cmake
    SOURCES
    include/opengeolab/app/translation_manager.h
    include/opengeolab/app/request_service.h
    include/opengeolab/app/log_event_model.h
    include/opengeolab/app/log_filter_proxy_model.h
    include/opengeolab/app/module_data_notifier.h
```

- [ ] 步骤 4：修改 `main.cpp`

在 include 区域（第 10 行 `#include "opengeolab/app/request_service.h"` 之后）添加：

```cpp
#include "opengeolab/app/module_data_notifier.h"
```

在 RequestService 创建和注册之后（第 58 行之后），添加：

```cpp
    OpenGeoLab::App::ModuleDataNotifier module_notifier(dispatcher);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0,
                                 "ModuleDataNotifier", &module_notifier);
```

- [ ] 步骤 5：构建全量

```
cmake --build build --config RelWithDebInfo --parallel 8
```

- [ ] 步骤 6：运行测试 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 7：`clang-format -i src/app/include/opengeolab/app/module_data_notifier.h src/app/src/module_data_notifier.cpp src/app/src/main.cpp`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
**预期结果：** 构建成功，所有测试通过

---

## 任务 6：QML — ShapeListItem + SidebarPanel 改造 + Main.qml 清理

**文件：**
- 新增：`src/app/resource/qml/components/ShapeListItem.qml`
- 修改：`src/app/resource/qml/sections/SidebarPanel.qml`
- 修改：`src/app/resource/qml/Main.qml`
- 修改：`src/app/CMakeLists.txt`（QML_FILES）

**注：** QML 组件不适合 TDD。验证通过编译和手动运行应用。

- [ ] 步骤 1：创建 `ShapeListItem.qml`

```qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"

/// @brief Single shape entry — collapsed header with expandable details.
Item {
    id: root

    required property AppTheme theme
    required property int shapeId
    required property string name
    required property string shapeType
    required property bool hasTessellation
    required property var topology
    required property var boundingBox
    required property bool geoVisible
    required property bool meshVisible

    signal toggleGeoVisibility(int shapeId)
    signal toggleMeshVisibility(int shapeId)

    property bool expanded: false

    readonly property var palette: [
        "#4FC3F7", "#81C784", "#FFB74D", "#E57373",
        "#BA68C8", "#4DB6AC", "#FFD54F", "#90A4AE"
    ]

    implicitWidth: parent ? parent.width : 260
    implicitHeight: col.implicitHeight

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusSmall
        color: root.expanded ? root.theme.tint(root.theme.accentPrimary, 0.08)
                             : "transparent"
        border.color: root.expanded ? root.theme.accentPrimary : "transparent"
        border.width: 1
    }

    Column {
        id: col
        width: parent.width
        padding: 6

        // --- Collapsed header row ---
        RowLayout {
            width: parent.width - 12
            spacing: 6

            // Color block
            Rectangle {
                width: 4
                height: 16
                radius: 2
                color: root.palette[root.shapeId % root.palette.length]
            }

            // ID
            Text {
                text: "#" + root.shapeId
                font.pixelSize: 11
                color: root.theme.textTertiary
            }

            // Name
            Text {
                Layout.fillWidth: true
                text: root.name
                font.pixelSize: 13
                color: root.theme.textPrimary
                elide: Text.ElideRight
            }

            // Geo visibility toggle
            Text {
                text: root.geoVisible ? "\u{1F441}" : "\u{1F441}\u{200D}\u{1F5E8}"
                font.pixelSize: 14
                opacity: root.geoVisible ? 1.0 : 0.3
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleGeoVisibility(root.shapeId)
                }
            }

            // Mesh visibility toggle
            Text {
                text: "\u25A6"
                font.pixelSize: 14
                opacity: root.meshVisible ? 1.0 : 0.3
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleMeshVisibility(root.shapeId)
                }
            }

            // Type label
            Text {
                text: root.shapeType
                font.pixelSize: 11
                color: root.theme.textTertiary
            }
        }

        // --- Click area for expand/collapse ---
        MouseArea {
            width: parent.width - 12
            height: col.children[0].height
            z: -1
            onClicked: root.expanded = !root.expanded
        }

        // --- Expanded detail rows ---
        Column {
            id: detailCol
            width: parent.width - 12
            visible: root.expanded
            spacing: 4
            topPadding: 4

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.borderSubtle
            }

            // Topology row
            Text {
                width: parent.width
                text: {
                    let t = root.topology || {}
                    let parts = []
                    if (t.faces !== undefined)    parts.push(qsTr("%1 Faces").arg(t.faces))
                    if (t.edges !== undefined)    parts.push(qsTr("%1 Edges").arg(t.edges))
                    if (t.vertices !== undefined) parts.push(qsTr("%1 Vertices").arg(t.vertices))
                    return qsTr("Topology") + "    " + parts.join(" \u00B7 ")
                }
                font.pixelSize: 12
                color: root.theme.textSecondary
                wrapMode: Text.WordWrap
            }

            // Bounds row
            Text {
                width: parent.width
                text: {
                    let bb = root.boundingBox || {}
                    let mn = bb.min || [0,0,0]
                    let mx = bb.max || [0,0,0]
                    let dx = (mx[0] - mn[0]).toFixed(1)
                    let dy = (mx[1] - mn[1]).toFixed(1)
                    let dz = (mx[2] - mn[2]).toFixed(1)
                    return qsTr("Bounds") + "    " + dx + " \u00D7 " + dy + " \u00D7 " + dz
                }
                font.pixelSize: 12
                color: root.theme.textSecondary
            }

            // Tessellation status
            Text {
                width: parent.width
                text: root.hasTessellation ? qsTr("Tessellated") + " \u2705"
                                           : qsTr("Not tessellated") + " \u23F3"
                font.pixelSize: 12
                color: root.theme.textSecondary
            }
        }
    }
}
```

- [ ] 步骤 2：替换 `SidebarPanel.qml` 为自管理版本

```qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../theme"
import "../components"

/// @brief Left sidebar — shape explorer with real-time list.
Item {
    id: root

    required property AppTheme theme

    property var shapeList: []
    property var visibilityState: ({})

    implicitWidth: 280

    function fetchShapeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "geometry",
            action: "list_shapes",
            param: {},
            mute: true
        }))
    }

    function toggleGeoVisibility(shapeId) {
        let state = root.visibilityState
        if (!(shapeId in state)) {
            state[shapeId] = { geo: true, mesh: false }
        }
        state[shapeId].geo = !state[shapeId].geo
        root.visibilityState = state
    }

    function toggleMeshVisibility(shapeId) {
        let state = root.visibilityState
        if (!(shapeId in state)) {
            state[shapeId] = { geo: true, mesh: false }
        }
        state[shapeId].mesh = !state[shapeId].mesh
        root.visibilityState = state
    }

    function geoVisible(shapeId) {
        return !(shapeId in root.visibilityState)
               || root.visibilityState[shapeId].geo
    }

    function meshVisible(shapeId) {
        return (shapeId in root.visibilityState)
               && root.visibilityState[shapeId].mesh
    }

    Timer {
        id: refreshTimer
        interval: 100
        repeat: false
        onTriggered: root.fetchShapeList()
    }

    Connections {
        target: ModuleDataNotifier
        function onGeometryDataChanged() {
            refreshTimer.restart()
        }
    }

    Connections {
        target: RequestService
        function onResponseReady(responseJson, muted) {
            try {
                const resp = JSON.parse(responseJson)
                if (resp.action === "list_shapes" && resp.ok) {
                    root.shapeList = resp.shapes || []
                }
            } catch (e) {
                // Ignore non-JSON or unrelated responses
            }
        }
    }

    Component.onCompleted: fetchShapeList()

    SectionCard {
        anchors.fill: parent
        anchors.margins: 0
        theme: root.theme
        title: qsTr("Scene")
        subtitle: qsTr("Explorer")

        // Empty state
        Column {
            visible: root.shapeList.length === 0
            width: parent.width
            spacing: 8
            topPadding: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "\uD83D\uDCE6"
                font.pixelSize: 32
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No geometry loaded.")
                font.pixelSize: 13
                color: root.theme.textTertiary
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Create a shape or import a model to get started.")
                font.pixelSize: 12
                color: root.theme.textTertiary
                wrapMode: Text.WordWrap
            }
        }

        // Shape list
        ListView {
            visible: root.shapeList.length > 0
            width: parent.width
            height: contentHeight
            model: root.shapeList
            clip: true
            spacing: 2
            interactive: false

            delegate: ShapeListItem {
                required property var modelData
                required property int index

                width: ListView.view.width
                theme: root.theme
                shapeId: modelData.shapeId ?? 0
                name: modelData.name ?? ""
                shapeType: modelData.shapeType ?? "Shape"
                hasTessellation: modelData.hasTessellation ?? false
                topology: modelData.topology ?? {}
                boundingBox: modelData.boundingBox ?? {}
                geoVisible: root.geoVisible(modelData.shapeId ?? 0)
                meshVisible: root.meshVisible(modelData.shapeId ?? 0)

                onToggleGeoVisibility: (sid) => root.toggleGeoVisibility(sid)
                onToggleMeshVisibility: (sid) => root.toggleMeshVisibility(sid)
            }
        }
    }
}
```

- [ ] 步骤 3：修改 `Main.qml`

删除第 19-21 行的 `ListModel { id: boxListModel }`。

修改 SidebarPanel 绑定（约第 219-224 行），移除 `boxListModel: boxListModel`：

```qml
                        SidebarPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: appTheme
                        }
```

- [ ] 步骤 4：修改 `src/app/CMakeLists.txt` 的 `QML_FILES` 部分（第 70 行 `resource/qml/components/BoxListItem.qml` 之后）添加：

```cmake
    resource/qml/components/ShapeListItem.qml
```

- [ ] 步骤 5：构建 `cmake --build build --config RelWithDebInfo --parallel 8`
- [ ] 步骤 6：运行测试 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 8 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
**预期结果：** 构建成功，所有测试通过

---

## 任务 7：翻译更新

**文件：**
- 修改：`src/app/resource/translations/opengeolab_zh_CN.ts`

- [ ] 步骤 1：在翻译文件中找到现有的 `<context><name>SidebarPanel</name>` 块，更新内容：

替换旧的 SidebarPanel context，添加新的翻译字符串（Scene、Explorer 保留，新增 No geometry loaded 等）。

新增 `ShapeListItem` context：

```xml
<context>
    <name>ShapeListItem</name>
    <message>
        <source>%1 Faces</source>
        <translation>%1 个面</translation>
    </message>
    <message>
        <source>%1 Edges</source>
        <translation>%1 条边</translation>
    </message>
    <message>
        <source>%1 Vertices</source>
        <translation>%1 个顶点</translation>
    </message>
    <message>
        <source>Topology</source>
        <translation>拓扑</translation>
    </message>
    <message>
        <source>Bounds</source>
        <translation>包围盒</translation>
    </message>
    <message>
        <source>Tessellated</source>
        <translation>已曲面化</translation>
    </message>
    <message>
        <source>Not tessellated</source>
        <translation>未曲面化</translation>
    </message>
</context>
```

更新 SidebarPanel context，替换旧的 "No geometry yet..." 为：

```xml
<context>
    <name>SidebarPanel</name>
    <message>
        <source>Scene</source>
        <translation>场景</translation>
    </message>
    <message>
        <source>Explorer</source>
        <translation>浏览器</translation>
    </message>
    <message>
        <source>No geometry loaded.</source>
        <translation>暂无几何体。</translation>
    </message>
    <message>
        <source>Create a shape or import a model to get started.</source>
        <translation>创建形状或导入模型以开始。</translation>
    </message>
</context>
```

- [ ] 步骤 2：构建验证翻译编译通过

```
cmake --build build --config RelWithDebInfo --parallel 8
```

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 8`
**预期结果：** 构建成功（翻译文件编译无错误）

---

## 任务 8：最终验证 + 提交

**文件：** 无新修改

- [ ] 步骤 1：全量构建 `cmake --build build --config RelWithDebInfo --parallel 8`
- [ ] 步骤 2：全量测试 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 3：clang-format 所有修改过的 C++ 文件
- [ ] 步骤 4：`git --no-pager diff --stat` 确认变更范围
- [ ] 步骤 5：向用户确认是否提交
- [ ] 步骤 6：提交

**建议 commit message：**

```
feat(app): add sidebar shape explorer with command-layer event bus

Add ModuleDataEvent enum (7 types) in core, dataChanged signal in
ModuleBase, onModuleDataChanged() subscription API in CommandDispatcher,
and ModuleDataNotifier QObject bridge in app. SidebarPanel now
self-manages a real-time shape list via debounced list_shapes requests.
list_shapes enhanced with shapeType, boundingBox, and wire count.
App layer has zero dependency on geometry types.
```

**预期结果：** 所有测试通过，变更已提交
