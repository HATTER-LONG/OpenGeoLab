# OpenGeoLab

OpenGeoLab 是一个面向 CAD/CAE 场景的桌面原型工程，使用 Qt Quick(QML) 构建界面，使用 OpenGL 做实时显示，使用 OpenCASCADE 管理几何拓扑，并逐步引入网格生成、选择编辑和 AI 辅助分析能力。

英文说明见 README.en.md，协议说明见 docs/json_protocols.md，面向大模型的几何/网格协议设计见 docs/llm_geometry_mesh_protocol.md。

## 功能概览

- 导入 BREP / STEP(STP) 模型。
- 基于 OCC 管理点、边、线框、面、实体和 Part 等几何层级。
- 通过 OpenGL 渲染几何与网格，并提供基础视口交互。
- 支持基于拾取的查询、高亮与选择。
- 支持基于几何对象的网格生成，以及基于局部区域的网格平滑。
- 为后续 trim、offset、网格质量诊断与 AI 协助修复预留扩展点。

## 模块结构

- include/：跨模块公共接口；模块间依赖应经由这里暴露。
- src/app/：应用入口、QML 后端桥接、异步服务、视口对象。
- src/geometry/：几何文档、实体关系、OCC 形体装配、几何渲染数据构建。
- src/mesh/：网格文档、节点/单元管理、关系索引、网格渲染数据构建。
- src/render/：渲染快照、场景控制、GPU pass、拾取与高亮。
- src/io/：BREP / STEP 读取服务。
- resources/qml/：QML 页面与组件。
- test/：单元测试与测试构建脚本。

更完整的架构说明见 docs/architecture.md。

## 依赖

- CMake 3.14+
- Qt 6.8+：Core、Gui、Qml、Quick、OpenGL
- OpenCASCADE
- gmsh
- Ninja + MSVC 或等价的 Windows C++20 工具链
- 可选：HDF5 / HighFive

项目使用 CPM 拉取部分三方库，包括 Kangaroo、nlohmann/json、Catch2、fmt、spdlog 等。

## 构建

以下命令以 Windows + Ninja 为例。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target opengeolab
```

生成的可执行文件位于 build/bin/OpenGeoLabApp.exe。

如果需要启用测试：

```powershell
cmake -S . -B build -G Ninja -DENABLE_TEST=ON
cmake --build build --target OpenGeoLabTests
ctest --test-dir build --output-on-failure
```

## QML 与后端协议

- QML 统一通过 BackendService.request(module, JSON.stringify(params)) 发送请求。
- 各模块服务在 processRequest 中按照 action 分发。
- 结果信号：operationFinished(moduleName, actionName, result)
- 失败信号：operationFailed(moduleName, actionName, error)

完整字段说明见 docs/json_protocols.md。

## 本轮工程化改进

- GeometryDocumentImpl 与 MeshDocumentImpl 增加文档级读写锁，修正后台服务与渲染数据构建之间的并发边界。
- RenderSceneController 改为共享快照交换，避免后台线程直接改写正在被渲染线程消费的数据。
- BackendService 修正请求失败信号参数与取消语义，避免伪中断和错误字段错位。
- 测试工程升级到 C++20，并补充 mesh 文档与渲染构建测试。

## 开发约定

- 注释风格参考 doxygen_comment_style.md。
- include 中的头文件视为模块边界；避免直接跨模块依赖 src 实现细节。
- 新增长耗时操作时，优先考虑取消、进度回传和线程边界。
