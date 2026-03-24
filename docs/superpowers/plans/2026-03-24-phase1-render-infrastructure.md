# Phase 1: 渲染基础设施 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 在 QML Viewport 中渲染带无限网格的 3D 空间，支持鼠标轨道旋转、平移、缩放。

**架构：** 新增 `libs/scene`（纯 C++ 场景数据类型）和 `libs/render`（纯 C++ OpenGL 4.5 渲染引擎），通过 `app/` 中的 `GLViewportItem`（QQuickFramebufferObject 子类）桥接到 QML。GLAD 加载 OpenGL 4.5 Core Profile 函数指针，GLM 提供数学运算。Camera 使用轨道球模型，PassManager 调度可组合渲染通道。

**技术栈：** C++20 · OpenGL 4.5 Core Profile · GLAD 2 · GLM 1.0+ · Qt 6.9 (QQuickFramebufferObject) · Kangaroo (signals) · doctest

**规格文档：** `docs/superpowers/specs/2026-03-24-cae-preprocess-roadmap-design.md`

---

## 文件结构总览

### 新增文件

```
third_party/glad/
├── CMakeLists.txt                               # GLAD 静态库目标
├── include/glad/gl.h                            # glad2 生成的 GL 4.5 Core 头文件
├── include/KHR/khrplatform.h                    # Khronos 平台类型
└── src/gl.c                                     # glad2 生成的加载器实现

src/libs/scene/
├── CMakeLists.txt                               # opengeolab_scene 模块定义
├── include/opengeolab/scene/
│   ├── bounding_box.hpp                         # 轴对齐包围盒
│   ├── render_mesh_data.hpp                     # 渲染用网格数据
│   ├── scene_node.hpp                           # 场景节点
│   └── scene_graph.hpp                          # 场景图（树结构 + 增信号）
├── src/
│   └── scene_graph.cpp                          # SceneGraph 实现
└── tests/
    └── scene_graph_test.cpp                     # SceneGraph 单元测试

src/libs/render/
├── CMakeLists.txt                               # opengeolab_render 模块定义
├── include/opengeolab/render/
│   ├── camera.hpp                               # 轨道球相机
│   ├── i_render_pass.hpp                        # 渲染通道抽象接口
│   ├── render_context.hpp                       # 每帧渲染上下文
│   ├── pass_manager.hpp                         # Pass 注册/调度器
│   ├── shader_program.hpp                       # GLSL 着色器编译/链接
│   ├── grid_pass.hpp                            # 无限网格 Pass
│   └── render_engine.hpp                        # 顶层渲染引擎
├── src/
│   ├── camera.cpp
│   ├── pass_manager.cpp
│   ├── shader_program.cpp
│   ├── grid_pass.cpp
│   └── render_engine.cpp
└── tests/
    ├── camera_test.cpp                          # Camera 矩阵/变换测试
    └── pass_manager_test.cpp                    # Pass 注册/排序测试

src/app/
├── include/opengeolab/app/
│   ├── gl_viewport_item.hpp                     # QQuickFramebufferObject 桥接
│   └── viewport_controller.hpp                  # 鼠标事件→相机操作翻译
└── src/
    ├── gl_viewport_item.cpp
    └── viewport_controller.cpp
```

### 修改文件

```
CMakeLists.txt                                   # 添加 GLM CPM、glad 子目录、scene/render 子目录
src/app/CMakeLists.txt                           # 链接 Render/Scene、注册新 QML 源、新 C++ 源
src/app/src/main.cpp                             # 升级 QSurfaceFormat 到 GL 4.5 Core
src/app/resource/qml/sections/ViewportPanel.qml  # 从 Canvas 占位替换为 GLViewportItem
src/app/resource/qml/Main.qml                    # 可能需要传递属性给 ViewportPanel
```

### 边界稳定性

- `libs/scene` 和 `libs/render` 的公共头文件是后续 Phase 2-6 的稳定接口，设计时预留扩展点。
- `app/` 中的 `GLViewportItem` 是 QML 唯一的 3D 渲染入口，后续 Phase 只扩展功能不替换组件。
- `ViewportPanel.qml` 的外部接口（`required property AppTheme theme`）保持不变。

---

## 任务 1：添加 GLM 依赖（CPM）

**文件：**
- 修改：`CMakeLists.txt`（根目录）

**接口草案：**

```cmake
# 在 Dependency versions 区域添加
set(OPENGEOLAB_GLM_VERSION 1.0.1)

# 在 Third-party 区域添加
opengeolab_resolve_package(
    glm
    glm::glm
    VERSION
    ${OPENGEOLAB_GLM_VERSION}
    GITHUB_REPOSITORY
    g-truc/glm
    GIT_TAG
    ${OPENGEOLAB_GLM_VERSION})
```

**步骤：**

- [ ] 在 `OPENGEOLAB_DOCTEST_VERSION` 后添加 `OPENGEOLAB_GLM_VERSION` 变量
- [ ] 在 nlohmann_json 的 `opengeolab_resolve_package` 之后添加 GLM 的 `opengeolab_resolve_package`
- [ ] 运行 `cmake -S . -B build` 确认 configure 成功，GLM 被下载
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4` 确认构建无回归
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake -S . -B build 2>&1 | Select-String -Pattern "glm|GLM"
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** CMake 输出含 "Fetching glm from GitHub" 或 "Using installed glm package"；全量构建通过。

---

## 任务 2：添加 GLAD 2 生成文件

**文件：**
- 新增：`third_party/glad/CMakeLists.txt`
- 新增：`third_party/glad/include/glad/gl.h`
- 新增：`third_party/glad/include/KHR/khrplatform.h`
- 新增：`third_party/glad/src/gl.c`
- 修改：`CMakeLists.txt`（根目录）

**步骤：**

- [ ] 用 glad2 Python 工具生成 GL 4.5 Core Profile 加载器：
  ```powershell
  # 使用项目的 Python 环境
  python -m pip install glad2
  python -m glad --api "gl:core=4.5" --out-path third_party/glad c
  ```
  如果 pip install 失败，可从 https://gen.glad.sh/ 手动下载（选 gl:core=4.5, C/C++, No loader, No extensions）。
  注意：必须使用 `glad2`（`pip install glad2`，版本 ≥ 2.0），旧版 `glad`（1.x）CLI 语法完全不同。
- [ ] 创建 `third_party/glad/CMakeLists.txt`：
  ```cmake
  add_library(glad STATIC src/gl.c)
  target_include_directories(glad PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
  set_target_properties(glad PROPERTIES FOLDER "third_party" POSITION_INDEPENDENT_CODE ON)
  ```
- [ ] 在根 `CMakeLists.txt` 的 Sub-projects 区域、`add_subdirectory(src/libs/base)` **之前**添加：
  ```cmake
  add_subdirectory(third_party/glad)
  ```
- [ ] 运行 `cmake -S . -B build && cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 确认 `glad` 静态库生成在 `build/lib/`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
Get-ChildItem build/lib -Filter "glad*"
```

**预期结果：** `build/lib/glad.lib`（或 `.a`）存在；构建无错误。

---

## 任务 3：创建 libs/scene 模块 — 数据类型

**文件：**
- 新增：`src/libs/scene/CMakeLists.txt`
- 新增：`src/libs/scene/include/opengeolab/scene/render_mesh_data.hpp`
- 新增：`src/libs/scene/include/opengeolab/scene/bounding_box.hpp`
- 新增：`src/libs/scene/include/opengeolab/scene/scene_node.hpp`
- 修改：`CMakeLists.txt`（根目录）— 添加 `add_subdirectory(src/libs/scene)`

**接口草案：**

```cpp
// render_mesh_data.hpp
#pragma once
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Scene {

enum class PrimitiveType { Triangles, Lines, Points };

/** @brief 渲染用网格数据，由几何/网格适配器生成。 */
struct RenderMeshData {
    std::vector<float> positions;    ///< xyz interleaved
    std::vector<float> normals;      ///< xyz interleaved
    std::vector<uint32_t> indices;
    PrimitiveType topology = PrimitiveType::Triangles;
};

}  // namespace OpenGeoLab::Scene
```

```cpp
// bounding_box.hpp
#pragma once
#include <glm/vec3.hpp>
#include <limits>

namespace OpenGeoLab::Scene {

/** @brief 轴对齐包围盒。 */
struct BoundingBox {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] glm::vec3 center() const;
    [[nodiscard]] glm::vec3 size() const;
    void expand(const glm::vec3& point);
    void merge(const BoundingBox& other);
};

}  // namespace OpenGeoLab::Scene
```

```cpp
// scene_node.hpp
#pragma once
#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace OpenGeoLab::Scene {

enum class EntityType { Body, Face, Edge, Vertex, MeshRegion };

/** @brief 场景树节点，可包含几何体或网格。 */
struct SceneNode {
    int id = 0;
    std::string name;
    EntityType type = EntityType::Body;
    glm::mat4 transform{1.0f};
    BoundingBox bounds;
    std::vector<RenderMeshData> meshes;
    std::vector<SceneNode> children;
    bool visible = true;
    bool selected = false;
};

}  // namespace OpenGeoLab::Scene
```

**CMakeLists.txt 草案：**

```cmake
# BoundingBox 有 inline 实现，可以放在头文件中（header-only 部分）。
# 但 SceneGraph 有 .cpp，所以模块需要有 SOURCES。
# 此任务只创建数据类型头文件（header-only），SceneGraph 在任务 4 添加。
# 先用一个占位 .cpp（空）来满足 opengeolab_add_module 的 SOURCES 要求。

set(scene_public_headers
    include/opengeolab/scene/render_mesh_data.hpp
    include/opengeolab/scene/bounding_box.hpp
    include/opengeolab/scene/scene_node.hpp)

set(scene_sources src/scene_graph.cpp)  # 任务 4 填充

opengeolab_add_module(
    opengeolab_scene
    ALIAS_NAME Scene
    SOURCES ${scene_sources}
    PUBLIC_HEADERS ${scene_public_headers}
    PUBLIC_LINKS glm::glm)
```

注意：`bounding_box.hpp` 的 inline 方法实现可以直接写在头文件中（简单数学），避免额外 .cpp。

**步骤：**

- [ ] 创建目录结构 `src/libs/scene/include/opengeolab/scene/` 和 `src/libs/scene/src/`
- [ ] 创建 `render_mesh_data.hpp`（enum + struct）
- [ ] 创建 `bounding_box.hpp`（struct + inline 方法）
- [ ] 创建 `scene_node.hpp`（enum + struct）
- [ ] 创建 `src/scene_graph.cpp`（空占位，任务 4 填充）
- [ ] 创建 `CMakeLists.txt`
- [ ] 在根 `CMakeLists.txt` 添加 `add_subdirectory(src/libs/scene)`（在 geometry 之后）
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过，`opengeolab_scene` 库生成。

---

## 任务 4：实现 SceneGraph + 测试

**文件：**
- 新增：`src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- 修改：`src/libs/scene/src/scene_graph.cpp`（从空占位填充为实现）
- 新增：`src/libs/scene/tests/scene_graph_test.cpp`
- 修改：`src/libs/scene/CMakeLists.txt`（添加头文件 + 测试）

**接口草案：**

```cpp
// scene_graph.hpp
#pragma once
#include <opengeolab/scene/scene_node.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <functional>

namespace OpenGeoLab::Scene {

/**
 * @brief 全局场景图，持有所有节点的树结构。
 *
 * 根节点 ID 固定为 0，不可删除。
 * 增删节点后通过 changeCallback 通知监听方。
 */
class OPENGEOLAB_SCENE_EXPORT SceneGraph {
public:
    SceneGraph();

    [[nodiscard]] SceneNode& root();
    [[nodiscard]] const SceneNode& root() const;

    /** @brief 按 ID 递归查找节点，未找到返回 nullptr。 */
    [[nodiscard]] SceneNode* findById(int id);

    /** @brief 添加节点为 parentId 的子节点。返回分配的 ID。 */
    int addNode(SceneNode node, int parentId = 0);

    /** @brief 递归删除节点及其子树。不可删除根节点。 */
    bool removeNode(int id);

    /** @brief 计算整棵树的世界包围盒。 */
    [[nodiscard]] BoundingBox worldBounds() const;

    /** @brief 节点增删后的回调。Phase 2+ 将替换为 Kangaroo Signal。 */
    std::function<void()> onChanged;

private:
    SceneNode root_;
    int nextId_ = 1;
};

}  // namespace OpenGeoLab::Scene
```

**测试要点：**

```cpp
// scene_graph_test.cpp
TEST_CASE("SceneGraph") {
    SUBCASE("root node exists with id 0") { ... }
    SUBCASE("addNode returns unique id") { ... }
    SUBCASE("findById returns correct node") { ... }
    SUBCASE("findById returns nullptr for missing id") { ... }
    SUBCASE("removeNode removes node and children") { ... }
    SUBCASE("removeNode on root returns false") { ... }
    SUBCASE("addNode to non-root parent") { ... }
    SUBCASE("worldBounds covers all nodes") { ... }
    SUBCASE("onChanged callback fires on addNode") { ... }
    SUBCASE("onChanged callback fires on removeNode") { ... }
}
```

**步骤：**

- [ ] 编写 `scene_graph_test.cpp` — 先写失败测试
- [ ] 运行测试确认失败（链接或编译错误，因为实现为空）
- [ ] 在 `scene_graph.hpp` 中定义类接口
- [ ] 在 `scene_graph.cpp` 中实现所有方法
- [ ] 更新 `CMakeLists.txt`：PUBLIC_HEADERS 添加 scene_graph.hpp，底部添加 doctest
  ```cmake
  if (OPENGEOLAB_BUILD_TESTS)
      opengeolab_add_doctest_test(
          opengeolab_scene_graph_test
          SOURCES tests/scene_graph_test.cpp
          LINKS OpenGeoLab::Scene)
  endif ()
  ```
- [ ] 运行测试确认全部通过
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure -R scene_graph
```

**预期结果：** 所有 SceneGraph 测试用例通过。

---

## 任务 5：创建 libs/render 模块 + Camera

**文件：**
- 新增：`src/libs/render/CMakeLists.txt`
- 新增：`src/libs/render/include/opengeolab/render/camera.hpp`
- 新增：`src/libs/render/src/camera.cpp`
- 修改：`CMakeLists.txt`（根目录）— 添加 `add_subdirectory(src/libs/render)`

**接口草案：**

```cpp
// camera.hpp
#pragma once
#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/render/render_export.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Render {

/** @brief 用于录制/回放的完整相机状态快照。 */
struct CameraState {
    glm::vec3 eye{0.0f, 5.0f, 10.0f};
    glm::vec3 target{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fovDeg = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    bool orthographic = false;
    float orthoWidth = 10.0f;
};

/** @brief 世界空间射线，用于拾取。 */
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

/**
 * @brief 轨道球相机，支持透视/正交、旋转、平移、缩放。
 *
 * 使用球坐标(theta, phi, distance)围绕 target 旋转。
 * 所有矩阵计算使用 GLM，无 OpenGL 依赖。
 */
class OPENGEOLAB_RENDER_EXPORT Camera {
public:
    Camera();

    void setPerspective(float fovDeg, float aspect, float nearPlane, float farPlane);
    void setOrthographic(float width, float aspect, float nearPlane, float farPlane);

    /** @brief 轨道旋转：deltaTheta 水平角增量(rad)，deltaPhi 垂直角增量(rad)。 */
    void orbit(float deltaTheta, float deltaPhi);

    /** @brief 平移：dx/dy 为屏幕空间偏移量，内部转换为世界空间。 */
    void pan(float dx, float dy);

    /** @brief 缩放：factor > 1 放大，< 1 缩小。 */
    void zoom(float factor);

    /** @brief 适配视图到包围盒。 */
    void fitAll(const Scene::BoundingBox& bbox);

    void setAspect(float aspect);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix() const;
    [[nodiscard]] glm::vec3 position() const;

    [[nodiscard]] CameraState captureState() const;
    void restoreState(const CameraState& state);

    /** @brief 屏幕坐标→世界射线（用于拾取）。 */
    [[nodiscard]] Ray screenToWorldRay(float screenX, float screenY,
                                        int viewportWidth, int viewportHeight) const;

private:
    CameraState state_;
    float aspect_ = 1.0f;

    void updateFromSpherical();
    // 球坐标参数（从 state_.eye/target 推导）
    float theta_ = 0.0f;   ///< 水平角 (radians)
    float phi_ = 0.7f;     ///< 垂直角 (radians)，范围 (0, pi)
    float distance_ = 12.0f;
};

}  // namespace OpenGeoLab::Render
```

**CMakeLists.txt 草案：**

```cmake
set(render_public_headers
    include/opengeolab/render/camera.hpp)

set(render_sources
    src/camera.cpp)

opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME Render
    SOURCES ${render_sources}
    PUBLIC_HEADERS ${render_public_headers}
    PUBLIC_LINKS OpenGeoLab::Scene glm::glm)
```

注意：glad 暂不链接，Camera 不依赖 GL 函数。任务 7（ShaderProgram）再添加 `PRIVATE_LINKS glad`。后续任务会添加更多源文件和头文件。

**步骤：**

- [ ] 创建目录结构
- [ ] 创建 `camera.hpp`
- [ ] 创建 `camera.cpp` — 实现所有方法
- [ ] 创建 `CMakeLists.txt`
- [ ] 在根 `CMakeLists.txt` 添加 `add_subdirectory(src/libs/render)`（在 scene 之后）
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过，`opengeolab_render` 库生成。

---

## 任务 6：Camera 单元测试

**文件：**
- 新增：`src/libs/render/tests/camera_test.cpp`
- 修改：`src/libs/render/CMakeLists.txt`（添加测试）

**测试要点：**

```cpp
TEST_CASE("Camera") {
    SUBCASE("default state produces valid view/projection") { ... }
    SUBCASE("setPerspective updates projection matrix") { ... }
    SUBCASE("setOrthographic updates projection matrix") { ... }
    SUBCASE("orbit changes eye position around target") { ... }
    SUBCASE("orbit clamps phi to avoid pole singularity") { ... }
    SUBCASE("pan shifts both eye and target") { ... }
    SUBCASE("zoom changes distance to target") { ... }
    SUBCASE("zoom clamps to minimum distance") { ... }
    SUBCASE("fitAll centers camera on bounding box") { ... }
    SUBCASE("captureState/restoreState round-trips") { ... }
    SUBCASE("screenToWorldRay center pixel points forward") { ... }
}
```

**步骤：**

- [ ] 编写 `camera_test.cpp`
- [ ] 在 `CMakeLists.txt` 底部添加：
  ```cmake
  if (OPENGEOLAB_BUILD_TESTS)
      opengeolab_add_doctest_test(
          opengeolab_render_camera_test
          SOURCES tests/camera_test.cpp
          LINKS OpenGeoLab::Render)
  endif ()
  ```
- [ ] 运行测试确认全部通过
- [ ] 如有失败，修复 camera.cpp 实现
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure -R camera
```

**预期结果：** 所有 Camera 测试用例通过。

---

## 任务 7：ShaderProgram 工具类

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/shader_program.hpp`
- 新增：`src/libs/render/src/shader_program.cpp`
- 修改：`src/libs/render/CMakeLists.txt`（添加源文件 + 头文件）

**接口草案：**

```cpp
// shader_program.hpp
#pragma once
#include <opengeolab/render/render_export.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <cstdint>
#include <string>
#include <string_view>

namespace OpenGeoLab::Render {

/**
 * @brief GLSL 着色器程序的 RAII 封装。
 *
 * 支持从字符串编译 vertex/fragment shader 并链接。
 * 提供 uniform setter 便捷方法。
 * 析构时释放 GL 资源。
 */
class OPENGEOLAB_RENDER_EXPORT ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /** @brief 从源码编译 vertex + fragment shader 并链接。成功返回 true。 */
    bool compile(std::string_view vertexSrc, std::string_view fragmentSrc);

    void use() const;
    [[nodiscard]] uint32_t id() const;

    // Uniform setters
    void setMat4(std::string_view name, const glm::mat4& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;
    void setFloat(std::string_view name, float value) const;
    void setInt(std::string_view name, int value) const;

private:
    uint32_t programId_ = 0;

    /** @brief 编译单个 shader，返回 shader ID。失败返回 0。 */
    static uint32_t compileShader(uint32_t type, std::string_view source);
};

}  // namespace OpenGeoLab::Render
```

**实现说明：**
- `compile()` 内部调用 `glCreateShader`/`glShaderSource`/`glCompileShader`/`glCreateProgram`/`glLinkProgram`
- 编译/链接失败时通过 `spdlog::error` 输出错误日志并返回 false
- `#include <glad/gl.h>` 仅在 .cpp 中使用，不暴露到公共头文件
- 此任务需要在 `CMakeLists.txt` 中添加 `PRIVATE_LINKS glad`（Camera 之前不需要 glad）
- 析构时调用 `glDeleteProgram`

**不写测试的理由：** ShaderProgram 的每个方法都调用 GL 函数，需要有效的 GL context。在 Phase 1 中不搭建离屏 GL 测试环境，通过集成测试（任务 14）验证。

**步骤：**

- [ ] 创建 `shader_program.hpp`
- [ ] 创建 `shader_program.cpp`
- [ ] 更新 `CMakeLists.txt` 添加新文件
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过，无警告。

---

## 任务 8：IRenderPass 接口 + PassManager

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/i_render_pass.hpp`
- 新增：`src/libs/render/include/opengeolab/render/render_context.hpp`
- 新增：`src/libs/render/include/opengeolab/render/pass_manager.hpp`
- 新增：`src/libs/render/src/pass_manager.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**接口草案：**

```cpp
// render_context.hpp
#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace OpenGeoLab::Render {

/** @brief 每帧传递给各 Pass 的只读渲染上下文。 */
struct RenderContext {
    int viewportWidth = 0;
    int viewportHeight = 0;
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f};
    glm::vec4 clearColor{0.15f, 0.15f, 0.17f, 1.0f};
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

}  // namespace OpenGeoLab::Render
```

```cpp
// i_render_pass.hpp
#pragma once
#include <opengeolab/render/render_context.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief 渲染通道抽象接口。
 *
 * PassManager 按优先级依次调用 setup→execute→teardown。
 * 每个 Pass 管理自己的 GPU 资源生命周期。
 */
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    /** @brief 分配 GPU 资源（首次调用或窗口 resize 后）。 */
    virtual void setup(int width, int height) = 0;

    /** @brief 执行渲染。 */
    virtual void execute(const RenderContext& ctx) = 0;

    /** @brief 释放 GPU 资源。 */
    virtual void teardown() = 0;
};

}  // namespace OpenGeoLab::Render
```

```cpp
// pass_manager.hpp
#pragma once
#include <opengeolab/render/i_render_pass.hpp>
#include <opengeolab/render/render_export.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Pass 注册中心与执行调度器。
 *
 * 按 priority 升序执行已启用的 Pass。
 * priority 数字越小越先执行。
 */
class OPENGEOLAB_RENDER_EXPORT PassManager {
public:
    void registerPass(std::string name,
                      std::unique_ptr<IRenderPass> pass,
                      int priority);
    void setPassEnabled(std::string_view name, bool enabled);
    [[nodiscard]] bool isPassEnabled(std::string_view name) const;

    /** @brief 对所有已注册 Pass 调用 setup。 */
    void setupAll(int width, int height);

    /** @brief 按优先级执行所有已启用的 Pass。 */
    void executeAll(const RenderContext& ctx);

    /** @brief 对所有已注册 Pass 调用 teardown。 */
    void teardownAll();

    [[nodiscard]] std::size_t passCount() const;

private:
    struct PassEntry {
        std::string name;
        std::unique_ptr<IRenderPass> pass;
        int priority;
        bool enabled = true;
    };
    std::vector<PassEntry> passes_;

    void sortByPriority();
};

}  // namespace OpenGeoLab::Render
```

**步骤：**

- [ ] 创建 `render_context.hpp`
- [ ] 创建 `i_render_pass.hpp`
- [ ] 创建 `pass_manager.hpp` 和 `pass_manager.cpp`
- [ ] 更新 `CMakeLists.txt`
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过。

---

## 任务 9：PassManager 单元测试

**文件：**
- 新增：`src/libs/render/tests/pass_manager_test.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**测试要点：**

```cpp
// 使用 MockPass（继承 IRenderPass，记录调用顺序）

TEST_CASE("PassManager") {
    SUBCASE("registerPass increases count") { ... }
    SUBCASE("executeAll calls passes in priority order") { ... }
    SUBCASE("disabled pass is skipped during executeAll") { ... }
    SUBCASE("setPassEnabled toggles correctly") { ... }
    SUBCASE("setupAll calls setup on all passes") { ... }
    SUBCASE("teardownAll calls teardown on all passes") { ... }
    SUBCASE("duplicate priority is allowed") { ... }
}
```

**步骤：**

- [ ] 编写 `pass_manager_test.cpp`（含 MockPass 定义）
- [ ] 在 `CMakeLists.txt` 添加测试目标：
  ```cmake
  opengeolab_add_doctest_test(
      opengeolab_render_pass_manager_test
      SOURCES tests/pass_manager_test.cpp
      LINKS OpenGeoLab::Render)
  ```
- [ ] 运行测试确认全部通过
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure -R pass_manager
```

**预期结果：** 所有 PassManager 测试通过。

---

## 任务 10：GridPass — 无限网格

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/grid_pass.hpp`
- 新增：`src/libs/render/src/grid_pass.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**接口草案：**

```cpp
// grid_pass.hpp
#pragma once
#include <opengeolab/render/i_render_pass.hpp>
#include <opengeolab/render/shader_program.hpp>
#include <opengeolab/render/render_export.hpp>
#include <cstdint>

namespace OpenGeoLab::Render {

/**
 * @brief 在 XZ 平面渲染无限网格。
 *
 * 使用全屏三角形 + fragment shader 中的世界空间投影技术。
 * 网格线随距离衰减，支持主副网格线。
 *
 * 算法参考: "Rendering Infinite Grids" (Ben Golus / Alex Evans)
 * - Vertex shader: 将全屏三角形的 NDC 坐标反投影到世界空间 XZ 平面
 * - Fragment shader: 用 fwidth() 计算网格线抗锯齿，按深度衰减 alpha
 */
class OPENGEOLAB_RENDER_EXPORT GridPass final : public IRenderPass {
public:
    void setup(int width, int height) override;
    void execute(const RenderContext& ctx) override;
    void teardown() override;

private:
    ShaderProgram shader_;
    uint32_t vao_ = 0;
    bool initialized_ = false;
};

}  // namespace OpenGeoLab::Render
```

**实现说明：**
- Vertex shader：渲染 3 个顶点组成的全屏三角形（无需顶点缓冲，用 `gl_VertexID` 生成 NDC 坐标），通过 inverse(VP) 反投影到世界空间
- Fragment shader：在 XZ 平面上用 `fract(worldPos.xz)` + `fwidth()` 画网格线，计算主网格线（1m 间距）和次网格线（0.1m），按到相机距离衰减 alpha
- `setup()` 中编译 shader 并创建空 VAO（OpenGL 4.5 Core 要求绑定 VAO 才能 draw）
- `execute()` 中设置 uniforms（view、projection、cameraPos）并调用 `glDrawArrays(GL_TRIANGLES, 0, 3)`
- `teardown()` 释放 VAO

**不写测试的理由：** GridPass 是纯 GL 渲染代码，需要 GL context。通过集成测试验证。

**步骤：**

- [ ] 创建 `grid_pass.hpp`
- [ ] 创建 `grid_pass.cpp`（含内嵌 GLSL 着色器字符串）
- [ ] 更新 `CMakeLists.txt`
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过。

---

## 任务 11：RenderEngine 顶层

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/render_engine.hpp`
- 新增：`src/libs/render/src/render_engine.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**接口草案：**

```cpp
// render_engine.hpp
#pragma once
#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/pass_manager.hpp>
#include <opengeolab/render/render_export.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief 顶层渲染引擎，组合 Camera + PassManager。
 *
 * 由 app/ 中的 GLViewportItem::Renderer 持有。
 * initialize() 在 GL context 可用后调用。
 * render() 在每帧渲染时调用。
 */
class OPENGEOLAB_RENDER_EXPORT RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    /** @brief 在 GL context 可用后调用，初始化内置 Pass。 */
    void initialize();

    /** @brief 窗口尺寸变化时调用。 */
    void resize(int width, int height);

    /** @brief 执行一帧渲染。 */
    void render();

    Camera& camera();
    const Camera& camera() const;
    PassManager& passManager();

private:
    Camera camera_;
    PassManager passManager_;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};

}  // namespace OpenGeoLab::Render
```

**实现说明：**
- `initialize()`：创建 GridPass 并注册到 PassManager（priority 100）；调用 `passManager_.setupAll()`
- `resize()`：更新宽高，调用 `camera_.setAspect()`，调用 `passManager_.setupAll()` 重新分配资源
- `render()`：构造 RenderContext，设置 `glViewport`/`glClear`，调用 `passManager_.executeAll(ctx)`

**步骤：**

- [ ] 创建 `render_engine.hpp`
- [ ] 创建 `render_engine.cpp`
- [ ] 更新 `CMakeLists.txt`
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过。

---

## 任务 12：GLViewportItem（QQuickFramebufferObject 桥接）

**文件：**
- 新增：`src/app/include/opengeolab/app/gl_viewport_item.hpp`
- 新增：`src/app/src/gl_viewport_item.cpp`
- 修改：`src/app/CMakeLists.txt`

**接口草案：**

```cpp
// gl_viewport_item.hpp
#pragma once
#include <opengeolab/render/render_engine.hpp>

#include <QQuickFramebufferObject>

namespace OpenGeoLab::App {

class ViewportController;

/**
 * @brief QML 中的 3D 视口组件，桥接 RenderEngine 和 Qt Scene Graph。
 *
 * 继承 QQuickFramebufferObject，在 Qt 的 render thread 中
 * 通过 Renderer 子类调用 RenderEngine 进行 OpenGL 渲染。
 */
class GLViewportItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ViewportController* controller READ controller CONSTANT)

public:
    explicit GLViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    ViewportController* controller() const;
    Render::RenderEngine& renderEngine();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    Render::RenderEngine renderEngine_;
    ViewportController* controller_;
};

}  // namespace OpenGeoLab::App
```

**Renderer 内部类实现说明（在 .cpp 中定义）：**

```cpp
class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
    void synchronize(QQuickFramebufferObject* item) override;
    void render() override;

private:
    Render::RenderEngine* engine_ = nullptr;
    bool gladInitialized_ = false;
};
```

- `createFramebufferObject()`：创建带深度附件的 FBO（`GL_DEPTH24_STENCIL8`）
- `synchronize()`：从 GLViewportItem 获取 RenderEngine 指针、同步尺寸
- `render()`：首次调用时 `gladLoadGL()` 初始化 GLAD，然后调用 `engine_->render()`

**GLAD 初始化方式：**

```cpp
void GLViewportRenderer::render() {
    if (!gladInitialized_) {
        int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(
            QOpenGLContext::currentContext()->getProcAddress));
        if (!version) {
            qWarning("Failed to initialize GLAD");
            return;
        }
        engine_->initialize();
        gladInitialized_ = true;
    }
    engine_->render();
    // 重置 Qt 的 GL 状态，避免与 Scene Graph 冲突
    window()->resetOpenGLState();
}
```

注意：`QOpenGLContext::currentContext()->getProcAddress` 返回 `QFunctionPointer` (`void (*)()`），需要 reinterpret_cast 到 `GLADloadfunc`。

**步骤：**

- [ ] 创建 `gl_viewport_item.hpp`
- [ ] 创建 `gl_viewport_item.cpp`（含 GLViewportRenderer 内部实现）
- [ ] 在 `src/app/CMakeLists.txt` 中：
  - 添加 `src/gl_viewport_item.cpp` 到 `qt_add_executable`
  - 添加 `include/opengeolab/app/gl_viewport_item.hpp` 到 `qt_add_qml_module` 的 SOURCES
  - 在 `target_link_libraries` 中添加 `OpenGeoLab::Render` 和 `OpenGeoLab::Scene`
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过。GLViewportItem 可作为 QML 元素使用。

---

## 任务 13：ViewportController

**文件：**
- 新增：`src/app/include/opengeolab/app/viewport_controller.hpp`
- 新增：`src/app/src/viewport_controller.cpp`
- 修改：`src/app/CMakeLists.txt`

**接口草案：**

```cpp
// viewport_controller.hpp
#pragma once
#include <opengeolab/render/camera.hpp>

#include <QObject>
#include <QPointF>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief 鼠标事件→相机操作的翻译层。
 *
 * 中键拖拽→轨道旋转，Shift+中键→平移，滚轮→缩放。
 * 左键保留给拾取（Phase 3），右键保留给上下文菜单。
 */
class ViewportController : public QObject {
    Q_OBJECT

public:
    explicit ViewportController(Render::Camera& camera, QObject* parent = nullptr);

    /** @brief 由 GLViewportItem 的鼠标事件转发调用。 */
    void onMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                      Qt::KeyboardModifiers modifiers);
    void onMouseMove(const QPointF& pos, Qt::MouseButtons buttons,
                     Qt::KeyboardModifiers modifiers);
    void onMouseRelease(const QPointF& pos);
    void onWheel(float angleDelta);

signals:
    /** @brief 相机状态变更，通知 QML 刷新视口。 */
    void cameraChanged();

private:
    Render::Camera& camera_;
    QPointF lastPos_;
    bool orbiting_ = false;
    bool panning_ = false;
};

}  // namespace OpenGeoLab::App
```

**交互映射：**

| 输入 | 动作 |
|------|------|
| 中键拖拽 | orbit（轨道旋转） |
| Shift + 中键拖拽 | pan（平移） |
| 滚轮 | zoom（缩放） |
| 左键 | 预留给 Pick（Phase 3） |
| 右键 | 预留给上下文菜单 |

**步骤：**

- [ ] 创建 `viewport_controller.hpp`
- [ ] 创建 `viewport_controller.cpp`
- [ ] 在 `src/app/CMakeLists.txt` 添加新文件：
  - `src/viewport_controller.cpp` 加入 `qt_add_executable` 源文件列表
  - `include/opengeolab/app/viewport_controller.hpp` 加入 `qt_add_qml_module` 的 SOURCES（与 gl_viewport_item.hpp 同级）
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

**预期结果：** 构建通过。

---

## 任务 14：QML 集成 + OpenGL 4.5 表面格式

**文件：**
- 修改：`src/app/src/main.cpp`
- 修改：`src/app/resource/qml/sections/ViewportPanel.qml`
- 修改：`src/app/resource/qml/Main.qml`（如需传递新属性）

**main.cpp 改动：**

```cpp
// 将当前的 QSurfaceFormat 升级为 GL 4.5 Core Profile
QSurfaceFormat fmt;
fmt.setVersion(4, 5);
fmt.setProfile(QSurfaceFormat::CoreProfile);
fmt.setDepthBufferSize(24);
fmt.setStencilBufferSize(8);
fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
fmt.setSwapInterval(1);
QSurfaceFormat::setDefaultFormat(fmt);
```

**ViewportPanel.qml 改动：**

将整个文件内容替换为使用 `GLViewportItem`：

```qml
pragma ComponentBehavior: Bound

import QtQuick
import OpenGeoLab.App    // GLViewportItem 所在 QML 模块
import "../theme"

Item {
    id: root

    required property AppTheme theme

    GLViewportItem {
        id: viewport
        anchors.fill: parent

        // 框选覆盖层预留（Phase 3）
    }

    // 相机变更时触发视口刷新
    Connections {
        target: viewport.controller
        function onCameraChanged() {
            viewport.update()
        }
    }
}
```

**步骤：**

- [ ] 修改 `main.cpp` 中的 `QSurfaceFormat` 设置
- [ ] 替换 `ViewportPanel.qml` 内容
- [ ] 检查 `Main.qml` 是否需要调整（ViewportPanel 外部接口不变，应无需改）
- [ ] 运行 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 运行所有测试确认无回归：
  ```powershell
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```
- [ ] 提交本任务改动

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

**预期结果：** 全量构建通过，所有测试通过。

---

## 任务 15：端到端验证 + clang-format

**文件：** 全部新增/修改文件

**步骤：**

- [ ] 对所有新增 C++ 文件运行 clang-format：
  ```powershell
  clang-format -i src/libs/scene/include/opengeolab/scene/*.hpp
  clang-format -i src/libs/scene/src/*.cpp
  clang-format -i src/libs/scene/tests/*.cpp
  clang-format -i src/libs/render/include/opengeolab/render/*.hpp
  clang-format -i src/libs/render/src/*.cpp
  clang-format -i src/libs/render/tests/*.cpp
  clang-format -i src/app/include/opengeolab/app/gl_viewport_item.hpp
  clang-format -i src/app/include/opengeolab/app/viewport_controller.hpp
  clang-format -i src/app/src/gl_viewport_item.cpp
  clang-format -i src/app/src/viewport_controller.cpp
  ```
- [ ] 全量构建：`cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 全量测试：`ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 启动应用手动验证：
  ```powershell
  .\build\bin\opengeolab_app.exe
  ```
  验证项：
  - 应用启动无崩溃
  - ViewportPanel 区域显示 3D 网格（XZ 平面）
  - 中键拖拽：轨道旋转
  - Shift + 中键拖拽：平移
  - 滚轮：缩放
  - 窗口 resize 后网格正常重绘
- [ ] 提交最终格式化改动（如有）

**验证命令：**
```powershell
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
.\build\bin\opengeolab_app.exe
```

**预期结果：** 构建通过，测试通过，应用启动可看到 3D 网格并支持相机操控。

---

## 后续计划

Phase 1 完成后，将依次编写以下实现计划（每个 Phase 一份独立计划）：

- **Phase 2**: OCC 几何与显示（`libs/occ` + GeometryPass + WireframePass + 参数对话框）
- **Phase 3**: 拾取与选择（PickPass + PickEngine + SelectionSet + HighlightPass）
- **Phase 4**: Command 系统与脚本录制（Command + CommandStack + CommandRecorder + Python 导出）
- **Phase 5**: GMSH 网格剖分（`libs/mesh` + MeshAdapter + MeshSettingsDialog）
- **Phase 6**: CAE 扩展（边界条件 + 高级可视化 + 布尔操作 + STEP 导入导出）
