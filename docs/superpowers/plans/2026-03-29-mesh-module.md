# Mesh 网格模块实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 为 OpenGeoLab 新增独立 Mesh 模块，使用 Gmsh 4.15.0 实现 2D/3D 网格剖分，含数据管理、渲染数据生成和配套 UI。

**架构：** MeshModule 遵循现有 ModuleBase + IAction + PluginComponentFactory 模式，拥有独立 MeshStore 管理网格数据。GmshBridge 封装 Gmsh C++ API 将 OCC shape 转为网格。MeshVisualBuilder 将网格转为 Core::VisualData 供渲染。UI 在 Mesh Tab 新增 2D/3D 两个功能页面。

**技术栈：** C++20, CMake 3.25+, Gmsh 4.15.0, Qt 6.9 QML, Kangaroo PluginComponentFactory, nlohmann_json, doctest

**规格文档：** `docs/superpowers/specs/2026-03-29-mesh-module-design.md`

---

## 文件清单

### 新增文件

| 文件路径 | 职责 |
|---------|------|
| `src/libs/mesh/CMakeLists.txt` | Mesh 模块构建定义 |
| `src/libs/mesh/include/opengeolab/mesh/mesh_types.hpp` | ElementType, MeshNodeArray, ElementBlock, GeoEntityRef |
| `src/libs/mesh/include/opengeolab/mesh/mesh_entry.hpp` | MeshEntry, ElementLocator |
| `src/libs/mesh/include/opengeolab/mesh/mesh_params.hpp` | SurfaceMeshParams, VolumeMeshParams |
| `src/libs/mesh/include/opengeolab/mesh/mesh_store.hpp` | MeshStore 数据存储 |
| `src/libs/mesh/include/opengeolab/mesh/gmsh_bridge.hpp` | Gmsh API 封装 |
| `src/libs/mesh/include/opengeolab/mesh/mesh_visual_builder.hpp` | 网格 → VisualData 转换 |
| `src/libs/mesh/include/opengeolab/mesh/mesh_module.hpp` | MeshModule (ModuleBase 子类) |
| `src/libs/mesh/include/opengeolab/mesh/generate_surface_mesh_action.hpp` | 2D 面剖分 action |
| `src/libs/mesh/include/opengeolab/mesh/generate_volume_mesh_action.hpp` | 3D 体剖分 action |
| `src/libs/mesh/include/opengeolab/mesh/delete_mesh_action.hpp` | 删除网格 action |
| `src/libs/mesh/include/opengeolab/mesh/query_mesh_action.hpp` | 查询网格 action |
| `src/libs/mesh/include/opengeolab/mesh/list_meshes_action.hpp` | 列出网格 action |
| `src/libs/mesh/src/mesh_entry.cpp` | ElementLocator 实现 |
| `src/libs/mesh/src/mesh_params.cpp` | 参数 JSON 序列化 |
| `src/libs/mesh/src/mesh_store.cpp` | MeshStore 实现 |
| `src/libs/mesh/src/gmsh_bridge.cpp` | Gmsh 桥接实现 |
| `src/libs/mesh/src/mesh_visual_builder.cpp` | 渲染数据生成实现 |
| `src/libs/mesh/src/mesh_module.cpp` | MeshModule 构造与 action 注册 |
| `src/libs/mesh/src/generate_surface_mesh_action.cpp` | 2D action 实现 |
| `src/libs/mesh/src/generate_volume_mesh_action.cpp` | 3D action 实现 |
| `src/libs/mesh/src/delete_mesh_action.cpp` | 删除 action 实现 |
| `src/libs/mesh/src/query_mesh_action.cpp` | 查询 action 实现 |
| `src/libs/mesh/src/list_meshes_action.cpp` | 列出 action 实现 |
| `src/libs/mesh/test/mesh_types_test.cpp` | 类型辅助函数测试 |
| `src/libs/mesh/test/mesh_entry_test.cpp` | ElementLocator O(1) 查找测试 |
| `src/libs/mesh/test/mesh_store_test.cpp` | MeshStore 增删查测试 |
| `src/libs/mesh/test/gmsh_bridge_test.cpp` | Gmsh 集成测试 |
| `src/app/resource/icons/meshSurface.svg` | 2D 面网格图标 |
| `src/app/resource/icons/meshVolume.svg` | 3D 体网格图标 |
| `src/app/resource/qml/components/ShapeSelector.qml` | Shape 下拉选择组件 |
| `src/app/resource/qml/components/pages/MeshSurfacePage.qml` | 2D 面剖分页面 |
| `src/app/resource/qml/components/pages/MeshVolumePage.qml` | 3D 体剖分页面 |

### 修改文件

| 文件路径 | 修改内容 |
|---------|---------|
| `CMakeLists.txt` | 添加 `find_package(gmsh REQUIRED)` 和 `add_subdirectory(src/libs/mesh)` |
| `CMakeUserPresets.json` | 添加 `gmsh_DIR` 路径到 local-debug / local-relwithdebinfo |
| `src/libs/command/src/module_registry.cpp` | 注册 MeshModule |
| `src/libs/command/CMakeLists.txt` | 添加 `OpenGeoLab::Mesh` 到 PRIVATE_LINKS |
| `src/app/CMakeLists.txt` | 添加新 QML 文件和图标到 qt_add_qml_module |
| `src/app/resource/qml/RibbonConfig.qml` | Mesh Tab 改为 2D/3D 入口 |
| `src/app/resource/qml/MainPages.qml` | 注册 meshSurface / meshVolume 页面 |

---

## 任务列表

### 任务 0：CMake 基础设施

**文件：**
- 修改：`CMakeLists.txt`
- 修改：`CMakeUserPresets.json`
- 新增：`src/libs/mesh/CMakeLists.txt`

- [ ] 步骤 1：在 `CMakeUserPresets.json` 的 `local-debug` preset 中添加 `"gmsh_DIR": "D:/WorkSpace/OpenSource/GMesh/gmsh_4_15_0-debug/share/gmsh"`，在 `local-relwithdebinfo` preset 中添加 `"gmsh_DIR": "D:/WorkSpace/OpenSource/GMesh/gmsh_4_15_0-relwithdebinfo/share/gmsh"`
- [ ] 步骤 2：在 `CMakeLists.txt` 的 `find_package(OpenCASCADE REQUIRED)` 之后添加 `find_package(gmsh REQUIRED)`
- [ ] 步骤 3：在 `CMakeLists.txt` 的 `add_subdirectory(src/libs/geometry)` 之后添加 `add_subdirectory(src/libs/mesh)`
- [ ] 步骤 4：创建目录结构 `src/libs/mesh/include/opengeolab/mesh/`、`src/libs/mesh/src/`、`src/libs/mesh/test/`
- [ ] 步骤 5：创建 `src/libs/mesh/CMakeLists.txt`，使用 `opengeolab_add_module` 定义 `opengeolab_mesh` 目标（ALIAS_NAME Mesh），PUBLIC_LINKS 包含 `OpenGeoLab::Core` 和 `gmsh::shared`，PRIVATE_LINKS 包含 `OpenGeoLab::Geometry` 和 OCC 库 `TKernel TKMath TKBRep`。暂时只包含一个空的 placeholder 源文件 `src/mesh_module.cpp`
- [ ] 步骤 6：创建最小化的 `mesh_module.hpp` 和 `mesh_module.cpp` 占位（只有空类声明），确保 CMake 配置和构建通过
- [ ] 步骤 7：验证 — `cmake --preset local-debug` 配置通过且 `cmake --build build --target opengeolab_mesh --config Debug --parallel 4` 构建通过

**验证命令：**
```
cmake --preset local-debug
cmake --build build --target opengeolab_mesh --config Debug --parallel 4
```

---

### 任务 1：核心数据类型 + 测试

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/mesh_types.hpp`
- 新增：`src/libs/mesh/test/mesh_types_test.cpp`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §4.1

- [ ] 步骤 1：写 `mesh_types_test.cpp`，测试 `nodesPerElement()` 对所有 ElementType 返回正确值，测试 `elementDimension()` 返回正确维度，测试 `MeshNodeArray::count()` 和 `MeshNodeArray::position()` 正确性
- [ ] 步骤 2：创建 `mesh_types.hpp`，定义 `ElementType` 枚举（值与 Gmsh element type 对应）、`nodesPerElement()`、`elementDimension()` constexpr 函数、`MeshNodeArray` 结构体（含 `coords` vector、`count()`、`position()` 方法）、`GeoEntityRef` 结构体、`ElementBlock` 结构体
- [ ] 步骤 3：在 `CMakeLists.txt` 中添加 test target：`opengeolab_add_doctest_test(opengeolab_mesh_types_test SOURCES test/mesh_types_test.cpp LINKS OpenGeoLab::Mesh)`
- [ ] 步骤 4：构建并运行测试
- [ ] 步骤 5：验证全部测试通过

**验证命令：**
```
cmake --build build --target opengeolab_mesh_types_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R mesh_types --output-on-failure
```

---

### 任务 2：ElementLocator + MeshEntry + 测试

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/mesh_entry.hpp`
- 新增：`src/libs/mesh/src/mesh_entry.cpp`
- 新增：`src/libs/mesh/test/mesh_entry_test.cpp`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §4.2

- [ ] 步骤 1：写 `mesh_entry_test.cpp`，测试：
  - `ElementLocator::build()` 从多个 blocks 构建前缀和
  - `ElementLocator::locate()` 对第一个、最后一个、跨 block 边界的 element ID 返回正确 Location
  - `ElementLocator::totalCount()` 返回正确总数
  - 边界情况：空 blocks、单 block、element ID 1
- [ ] 步骤 2：实现 `mesh_entry.hpp`（`ElementLocator` 类 + `MeshEntry` 结构体）和 `mesh_entry.cpp`（`ElementLocator::build()` 和 `locate()` 实现，使用 `std::upper_bound` 在前缀和数组上二分）
- [ ] 步骤 3：添加 test target 到 CMakeLists.txt
- [ ] 步骤 4：构建并运行测试，确认 O(log B) 查找正确

**验证命令：**
```
cmake --build build --target opengeolab_mesh_entry_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R mesh_entry --output-on-failure
```

---

### 任务 3：MeshStore + 测试

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/mesh_store.hpp`
- 新增：`src/libs/mesh/src/mesh_store.cpp`
- 新增：`src/libs/mesh/test/mesh_store_test.cpp`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §4.3

- [ ] 步骤 1：写 `mesh_store_test.cpp`，测试：
  - `add()` 返回递增 ID，`find()` 返回正确 entry
  - `remove()` 后 `find()` 返回 nullptr
  - `allMeshIds()` 返回当前有效 ID 列表
  - `findByShapeId()` 返回关联指定 shape 的所有 mesh
  - 信号 `meshAdded` / `meshRemoved` 正确触发（使用 Kangaroo Signal connect 验证）
- [ ] 步骤 2：实现 `mesh_store.hpp` 和 `mesh_store.cpp`。内部使用 `std::vector<std::unique_ptr<MeshEntry>>`，ID = index + 1，mutex 保护，信号在锁外 emit
- [ ] 步骤 3：添加 test target，链接 `OpenGeoLab::Mesh`
- [ ] 步骤 4：构建并运行测试

**验证命令：**
```
cmake --build build --target opengeolab_mesh_store_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R mesh_store --output-on-failure
```

---

### 任务 4：Mesh 参数 + JSON 序列化

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/mesh_params.hpp`
- 新增：`src/libs/mesh/src/mesh_params.cpp`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §5.1

- [ ] 步骤 1：实现 `mesh_params.hpp`，定义 `SurfaceMeshParams` 和 `VolumeMeshParams` 结构体，各含 `fromJson()` / `toJson()` 静态/成员方法。参数字段见规格：minSize, maxSize, algorithm, quadDominant/hexDominant, order, optimize, optimizeAlgorithm(仅 Volume)
- [ ] 步骤 2：实现 `mesh_params.cpp`，`fromJson` 使用 `param.value("key", default)` 模式
- [ ] 步骤 3：将源文件添加到 CMakeLists.txt 的 SOURCES 和 PUBLIC_HEADERS
- [ ] 步骤 4：构建 opengeolab_mesh target 确认编译通过

**验证命令：**
```
cmake --build build --target opengeolab_mesh --config Debug --parallel 4
```

**说明：** 参数序列化是纯值对象，不单独写测试（在 action 集成测试中覆盖）。

---

### 任务 5：GmshBridge + 集成测试

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/gmsh_bridge.hpp`
- 新增：`src/libs/mesh/src/gmsh_bridge.cpp`
- 新增：`src/libs/mesh/test/gmsh_bridge_test.cpp`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §5.2, §5.3

- [ ] 步骤 1：写 `gmsh_bridge_test.cpp`，测试：
  - 用 `BRepPrimAPI_MakeBox` 创建一个简单 box shape
  - 调用 `GmshBridge::generateSurfaceMesh(shape, defaultParams, nullProgress)` → 验证返回的 MeshEntry 有 nodeCount > 0 和 elementCount > 0，且 surfaceBlocks 非空
  - 调用 `GmshBridge::generateVolumeMesh(shape, defaultParams, nullProgress)` → 验证 volumeBlocks 非空
  - 验证 node coords 维度正确 (count * 3 == coords.size())
  - 验证 ElementBlock::connectivity 长度整除 nodesPerElem()
- [ ] 步骤 2：实现 `gmsh_bridge.hpp`，声明 `GmshSession` RAII 类和 `GmshBridge` namespace 中的 `generateSurfaceMesh` / `generateVolumeMesh` 函数
- [ ] 步骤 3：实现 `gmsh_bridge.cpp`：
  - `GmshSession` 构造函数调用 `gmsh::initialize()`，析构调用 `gmsh::finalize()`
  - `generateSurfaceMesh`：创建 GmshSession → `gmsh::model::add` → `gmsh::model::occ::importShapesNativePointer` → `synchronize` → 设置参数 → `generate(2)` → 可选 optimize → `getNodes` 提取到 MeshNodeArray → `getElements` 按 entity 提取到 ElementBlocks → 构建 ElementLocator → 返回 MeshEntry
  - `generateVolumeMesh`：类似，但 `generate(3)`
  - 提取 elements 时记录 GeoEntityRef（dim, gmshTag, sourceLocalId）
- [ ] 步骤 4：添加 test target（需链接 `OpenGeoLab::Mesh` 和 OCC 的 `TKPrim` 来创建 box）
- [ ] 步骤 5：构建并运行测试

**验证命令：**
```
cmake --build build --target opengeolab_gmsh_bridge_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R gmsh_bridge --output-on-failure
```

**风险：** Gmsh 初始化可能与 OCC 冲突。如果测试失败，改用 BRep 文件中间路径（降级方案，见规格 §13）。

---

### 任务 6：MeshVisualBuilder

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/mesh_visual_builder.hpp`
- 新增：`src/libs/mesh/src/mesh_visual_builder.cpp`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §6

- [ ] 步骤 1：实现 `mesh_visual_builder.hpp`，声明 `buildVisualData(const MeshEntry&)` → `Core::VisualData` 和 `buildEntityTags(const MeshEntry&)` → `MeshTags`
- [ ] 步骤 2：实现 `mesh_visual_builder.cpp`：
  - **2D 面单元** → `SurfaceMesh`：遍历 surfaceBlocks，将 tri/quad connectivity + node coords 组装为 positions (float) + 计算面法线 + 0-based indices
  - **3D 体网格外表面** → `SurfaceMesh`：遍历 volumeBlocks，提取每个体单元的面（tet→4 tri, hex→6 quad），用 canonical key 去重，只保留出现 1 次的边界面
  - **单元边线** → `EdgeMesh`：从 surfaceBlocks (或 boundary faces) 提取边
  - **节点** → `PointSet`：直接从 nodes.coords 转 float
  - **EntityTags**：为每个 node 生成 `{MeshNode, nodeId}`，为每个面三角形生成 `{MeshElement, elementId}`，为每条边线段生成 `{MeshEdge, edgeId}`
- [ ] 步骤 3：添加到 CMakeLists.txt
- [ ] 步骤 4：在已有的 `gmsh_bridge_test.cpp` 中补充一个 TEST_CASE，对 bridge 生成的 MeshEntry 调用 `buildVisualData`，验证 VisualData 非空且 surfaces/edges/points 各至少有一个条目

**验证命令：**
```
cmake --build build --target opengeolab_gmsh_bridge_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R gmsh_bridge --output-on-failure
```

---

### 任务 7：Actions 实现

**文件：**
- 新增：`src/libs/mesh/include/opengeolab/mesh/generate_surface_mesh_action.hpp`
- 新增：`src/libs/mesh/include/opengeolab/mesh/generate_volume_mesh_action.hpp`
- 新增：`src/libs/mesh/include/opengeolab/mesh/delete_mesh_action.hpp`
- 新增：`src/libs/mesh/include/opengeolab/mesh/query_mesh_action.hpp`
- 新增：`src/libs/mesh/include/opengeolab/mesh/list_meshes_action.hpp`
- 新增：对应的 5 个 `.cpp` 文件
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** 规格 §11, 参考 `create_box_action.hpp/cpp` 模式

- [ ] 步骤 1：实现 `GenerateSurfaceMeshAction`：
  - `ACTION_NAME = "generate_surface_mesh"`
  - 构造函数接收 `MeshStore& store` 和 `Kangaroo::Util::PluginComponentFactory& factory`
  - `describe()` 返回参数 schema（shapeId, name, minSize, maxSize, algorithm, quadDominant, order, optimize）
  - `execute()`：从 factory 获取 GeometryModule → ShapeStore::find(shapeId) → 获取 OCC shape → 调用 GmshBridge::generateSurfaceMesh → 设置 name 和 sourceShapeId → MeshStore::add → MeshStore::buildVisualData → 返回 `{ok:true, meshId, name, nodeCount, elementCount, elementTypes}`
- [ ] 步骤 2：实现 `GenerateVolumeMeshAction`：类似 surface，`ACTION_NAME = "generate_volume_mesh"`，调用 `generateVolumeMesh`
- [ ] 步骤 3：实现 `DeleteMeshAction`：`ACTION_NAME = "delete_mesh"`，调用 `MeshStore::remove(meshId)` → 返回 `{ok:true, action, meshId}`
- [ ] 步骤 4：实现 `QueryMeshAction`：`ACTION_NAME = "query_mesh"`，调用 `MeshStore::find(meshId)` → 返回 mesh 统计信息
- [ ] 步骤 5：实现 `ListMeshesAction`：`ACTION_NAME = "list_meshes"`，调用 `MeshStore::allMeshIds()` → 返回列表
- [ ] 步骤 6：将所有头文件和源文件添加到 CMakeLists.txt
- [ ] 步骤 7：构建 opengeolab_mesh 确认编译通过

**验证命令：**
```
cmake --build build --target opengeolab_mesh --config Debug --parallel 4
```

---

### 任务 8：MeshModule + 模块注册

**文件：**
- 修改：`src/libs/mesh/include/opengeolab/mesh/mesh_module.hpp`（扩展占位）
- 修改：`src/libs/mesh/src/mesh_module.cpp`（扩展占位）
- 修改：`src/libs/command/src/module_registry.cpp`
- 修改：`src/libs/command/CMakeLists.txt`
- 修改：`src/libs/mesh/CMakeLists.txt`

**参考：** `geometry_module.cpp` 模式、`module_registry.cpp` 模式

- [ ] 步骤 1：完善 `mesh_module.hpp`：
  - `MeshModule final : public Core::ModuleBase`
  - 包含 `MeshStore m_meshStore` 成员
  - `static constexpr std::string_view MODULE_NAME{"mesh"}`
  - `MeshStore& meshStore()` / `const MeshStore& meshStore() const`
- [ ] 步骤 2：完善 `mesh_module.cpp`：
  - 构造函数调用 `ModuleBase(MODULE_NAME, "Mesh generation and management module.", factory)`
  - `registerAction<GenerateSurfaceMeshAction>(std::ref(m_meshStore), std::ref(factory))`
  - `registerAction<GenerateVolumeMeshAction>(std::ref(m_meshStore), std::ref(factory))`
  - `registerAction<DeleteMeshAction>(std::ref(m_meshStore))`
  - `registerAction<QueryMeshAction>(std::ref(m_meshStore))`
  - `registerAction<ListMeshesAction>(std::ref(m_meshStore))`
  - 桥接 `m_meshStore.meshAdded` → `dataChanged.emit(ItemAdded)` 等信号
- [ ] 步骤 3：在 `module_registry.cpp` 中添加 `#include <opengeolab/mesh/mesh_module.hpp>` 和注册代码
- [ ] 步骤 4：在 `src/libs/command/CMakeLists.txt` 的链接中添加 `OpenGeoLab::Mesh` 到 PRIVATE_LINKS
- [ ] 步骤 5：构建全量并运行所有现有测试确保无回归

**验证命令：**
```
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

---

### 任务 9：SVG 图标

**文件：**
- 新增：`src/app/resource/icons/meshSurface.svg`
- 新增：`src/app/resource/icons/meshVolume.svg`

- [ ] 步骤 1：在 `D:\WorkSpace\OGLWorkSpace\ionicons-8.0.13\src\svg` 中查找合适的 mesh 相关图标。如果没有合适的，创建简洁的 SVG 图标：
  - `meshSurface.svg`：一个平面上覆盖三角网格线的图标（参考现有 mesh.svg 风格）
  - `meshVolume.svg`：一个立方体上覆盖网格线的图标
- [ ] 步骤 2：确保 SVG 尺寸为 512x512，单色（currentColor 或 #000），与现有图标风格一致

**说明：** 不需要 TDD，纯资源文件。

---

### 任务 10：QML UI — ShapeSelector 组件

**文件：**
- 新增：`src/app/resource/qml/components/ShapeSelector.qml`
- 修改：`src/app/CMakeLists.txt`（添加到 QML_FILES）

**参考：** `ParamField.qml` 和 `DimensionInput.qml` 组件风格

- [ ] 步骤 1：创建 `ShapeSelector.qml`，功能：
  - 属性：`property var theme`、`property string label`、`property int selectedShapeId: -1`、`property var shapeModel: []`
  - 外部由页面传入 shapeModel（从 ModuleDataNotifier 获取的 shape 列表）
  - 使用 ComboBox 显示 shape 名称列表
  - 选中时 emit `shapeSelected(int shapeId)` 信号
  - 样式：与 ParamField 对齐，使用 theme 颜色
- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 `QML_FILES` 中添加 `resource/qml/components/ShapeSelector.qml`

**说明：** QML 组件不做 TDD，在集成构建时验证。

---

### 任务 11：QML UI — MeshSurfacePage + MeshVolumePage

**文件：**
- 新增：`src/app/resource/qml/components/pages/MeshSurfacePage.qml`
- 新增：`src/app/resource/qml/components/pages/MeshVolumePage.qml`
- 修改：`src/app/CMakeLists.txt`

**参考：** `CreateBoxPage.qml` 完整模式

- [ ] 步骤 1：创建 `MeshSurfacePage.qml`：
  - 继承 `FunctionPageBase`，`pageTitle: qsTr("2D Surface Mesh")`，`pageIcon: "meshSurface"`，`actionId: "meshSurface"`
  - 属性：meshName, selectedShapeId, minSize(0.1), maxSize(10.0), algorithm(6), elementType("triangle"), order(1), optimize(true)
  - `getParameters()` 返回 `{module:"mesh", action:"generate_surface_mesh", param:{shapeId, name, minSize, maxSize, algorithm, quadDominant:(elementType==="quad"), order, optimize}}`
  - 表单字段（从上到下）：ParamField(name)、ShapeSelector(shape)、DimensionInput×2(minSize/maxSize)、ComboBox(Algorithm: MeshAdapt/Delaunay/Frontal-Delaunay/BAMG → 值 1/5/6/7)、ComboBox(Element: Triangle/Quad/Mixed)、ComboBox(Order: 1st/2nd)、CheckBox(Optimize)
  - 所有用户可见文本用 `qsTr()` 包裹
- [ ] 步骤 2：创建 `MeshVolumePage.qml`：
  - 类似 MeshSurfacePage，`pageTitle: qsTr("3D Volume Mesh")`，`pageIcon: "meshVolume"`，`actionId: "meshVolume"`
  - 额外属性：optimizeAlgorithm(0)
  - `getParameters()` 返回 action: "generate_volume_mesh"，参数包含 hexDominant 和 optimizeAlgorithm
  - Algorithm 选项：Delaunay/Frontal/HXT → 值 1/4/10
  - 额外 ComboBox：Optimize Algorithm (Default/Netgen/HighOrder → 值 0/1/2)
- [ ] 步骤 3：在 `src/app/CMakeLists.txt` 的 `QML_FILES` 中添加两个页面和图标到 `RESOURCES`

**验证：** 构建 app target 确认 QML 编译通过。

---

### 任务 12：UI 注册 — RibbonConfig + MainPages

**文件：**
- 修改：`src/app/resource/qml/RibbonConfig.qml`
- 修改：`src/app/resource/qml/MainPages.qml`

- [ ] 步骤 1：修改 `RibbonConfig.qml` 中 Mesh Tab（第二个数组元素），将现有的 `generateMesh` / `smoothMesh` 替换为：
  ```qml
  {
      "title": qsTr("Generate"),
      "actions": [
          { "key": "meshSurface", "title": qsTr("2D"), "icon": "meshSurface", "accentOne": "accentB", "accentTwo": "accentA" },
          { "key": "meshVolume", "title": qsTr("3D"), "icon": "meshVolume", "accentOne": "accentB", "accentTwo": "accentA" }
      ]
  }
  ```
  保留 Inspect group 中的 `queryMesh`
- [ ] 步骤 2：在 `MainPages.qml` 的 `componentMap` 中添加：
  ```qml
  "meshSurface": { path: "components/pages/MeshSurfacePage.qml" },
  "meshVolume":  { path: "components/pages/MeshVolumePage.qml" }
  ```
- [ ] 步骤 3：构建 app 并运行 QML page registration 测试

**验证命令：**
```
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug -R qml_pages --output-on-failure
```

---

### 任务 13：全量构建 + 集成验证

**文件：** 无新增

- [ ] 步骤 1：Debug 全量构建
  ```
  cmake --preset local-debug
  cmake --build build --config Debug --parallel 4
  ```
- [ ] 步骤 2：运行全部测试
  ```
  ctest --test-dir build -C Debug --output-on-failure
  ```
- [ ] 步骤 3：RelWithDebInfo 全量构建验证
  ```
  cmake --preset local-relwithdebinfo
  cmake --build build --config RelWithDebInfo --parallel 4
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```
- [ ] 步骤 4：如有测试失败，修复后重新验证
- [ ] 步骤 5：请求用户确认后 git commit（遵循 git workflow 规则：提交前必须询问用户）

**提交信息：**
```
feat(mesh): add mesh module with Gmsh integration

- Add MeshModule with MeshStore, GmshBridge, MeshVisualBuilder
- Support 2D surface and 3D volume mesh generation via Gmsh 4.15
- Implement generate/delete/query/list mesh actions
- Add MeshSurfacePage and MeshVolumePage QML UI
- Integrate Gmsh cmake config into user presets
- Register MeshModule in command dispatcher

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
```

---

## 依赖关系

```
任务 0 (CMake)
  └─ 任务 1 (数据类型)
       └─ 任务 2 (ElementLocator + MeshEntry)
            └─ 任务 3 (MeshStore)
                 ├─ 任务 4 (参数)
                 │    └─ 任务 5 (GmshBridge)
                 │         └─ 任务 6 (VisualBuilder)
                 │              └─ 任务 7 (Actions)
                 │                   └─ 任务 8 (MeshModule + 注册)
                 │                        └─ 任务 13 (集成验证)
                 └───────────────────────────────────┘
  任务 9 (图标) ─── 可与任务 1~8 并行
  任务 10 (ShapeSelector) ── 依赖任务 0
  任务 11 (QML 页面) ── 依赖任务 9, 10
  任务 12 (UI 注册) ── 依赖任务 11
       └─ 任务 13
```
