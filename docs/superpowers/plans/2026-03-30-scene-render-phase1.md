# Scene + Render Phase 1 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 实现轻量级场景图 + OpenGL 渲染引擎，支持 geometry/mesh 数据的 Phong 着色 + 线框叠加渲染，以及 trackball 相机交互。

**架构：** 新增两个库 libs/scene（线程安全场景图）和 libs/render（OpenGL 渲染 + QQuickFramebufferObject 视口），通过 App 层 SceneBridge 桥接 geometry/mesh 模块数据到场景图，渲染线程通过 Changeset 增量消费变更。

**技术栈：** C++20, CMake, Qt6 Quick/OpenGL, GLM 1.0.1, glad 2 (GL 4.6 core), GLSL 330 core, doctest

**规格文档：** `docs/superpowers/specs/2026-03-30-scene-render-phase1-design.md`

**参考代码：**
- Trackball: `D:\WorkSpace\OGLWorkSpace\OGL\include\render\trackball_controller.hpp` + `.cpp`
- CameraState: `D:\WorkSpace\OGLWorkSpace\OGL\include\render\render_scene_controller.hpp`

---

## 文件总览

### 新增文件

| 路径 | 职责 |
|------|------|
| `src/libs/scene/CMakeLists.txt` | scene 库构建定义 |
| `src/libs/scene/include/opengeolab/scene/transform.hpp` | 仿射变换 |
| `src/libs/scene/include/opengeolab/scene/bounding_box.hpp` | AABB 包围盒 |
| `src/libs/scene/include/opengeolab/scene/scene_node.hpp` | 场景节点结构体 |
| `src/libs/scene/include/opengeolab/scene/scene_graph.hpp` | 线程安全场景图 + Changeset |
| `src/libs/scene/src/transform.cpp` | Transform 实现 |
| `src/libs/scene/src/bounding_box.cpp` | BoundingBox 实现 |
| `src/libs/scene/src/scene_graph.cpp` | SceneGraph 实现 |
| `src/libs/scene/test/scene_graph_test.cpp` | scene 单元测试 |
| `src/libs/render/CMakeLists.txt` | render 库构建定义 |
| `src/libs/render/include/opengeolab/render/camera.hpp` | 相机状态 (eye-target-up) |
| `src/libs/render/include/opengeolab/render/trackball_controller.hpp` | Trackball 交互控制器 |
| `src/libs/render/include/opengeolab/render/shader_program.hpp` | GLSL 编译/链接封装 |
| `src/libs/render/include/opengeolab/render/gpu_mesh.hpp` | VAO/VBO/EBO RAII |
| `src/libs/render/include/opengeolab/render/render_scene.hpp` | 渲染线程侧场景快照 |
| `src/libs/render/include/opengeolab/render/render_engine.hpp` | OpenGL 两趟渲染器 |
| `src/libs/render/include/opengeolab/render/viewport_item.hpp` | QQuickFramebufferObject |
| `src/libs/render/include/opengeolab/render/viewport_renderer.hpp` | FBO 内部渲染器 |
| `src/libs/render/src/camera.cpp` | Camera 实现 |
| `src/libs/render/src/trackball_controller.cpp` | TrackballController 实现 (从参考适配) |
| `src/libs/render/src/shader_program.cpp` | ShaderProgram 实现 |
| `src/libs/render/src/gpu_mesh.cpp` | GpuMesh 实现 |
| `src/libs/render/src/render_scene.cpp` | RenderScene 实现 |
| `src/libs/render/src/render_engine.cpp` | RenderEngine 实现 |
| `src/libs/render/src/viewport_item.cpp` | ViewportItem 实现 |
| `src/libs/render/src/viewport_renderer.cpp` | ViewportRenderer 实现 |
| `src/libs/render/shaders/phong.vert` | Phong 顶点着色器 |
| `src/libs/render/shaders/phong.frag` | Phong 片段着色器 |
| `src/libs/render/shaders/edge.vert` | Edge 顶点着色器 |
| `src/libs/render/shaders/edge.frag` | Edge 片段着色器 |
| `src/libs/render/test/camera_test.cpp` | Camera + TrackballController 测试 |
| `src/app/src/scene_bridge.h` | App 层数据桥接 (header) |
| `src/app/src/scene_bridge.cpp` | App 层数据桥接 (impl) |

### 修改文件

| 路径 | 修改内容 |
|------|----------|
| `CMakeLists.txt` (根) | 添加 GLM resolve + glad CPM + scene/render subdirectory |
| `src/app/CMakeLists.txt` | 添加 scene_bridge 源文件 + 链接 Scene/Render |
| `src/app/src/main.cpp` | 注册 ViewportItem QML 类型 + 创建 SceneBridge |
| `src/app/include/opengeolab/app/module_data_notifier.h` | 添加 meshDataChanged 信号 |
| `src/app/src/module_data_notifier.cpp` | 订阅 mesh 模块 dataChanged |
| `src/app/resource/qml/sections/ViewportPanel.qml` | 替换 Canvas 占位为 ViewportItem |

---

## 任务列表

### 任务 0：CMake 基础设施 — GLM + glad + 目录结构

**文件：**
- 修改：`CMakeLists.txt`（根）
- 新增：`src/libs/scene/CMakeLists.txt`
- 新增：`src/libs/render/CMakeLists.txt`
- 新增：空目录结构

**参考：** 规格 §9.1, §9.2, §9.3, §9.4

- [ ] 步骤 1：在根 `CMakeLists.txt` 第三方区域（约 line 174 后）添加 GLM resolve：
  ```cmake
  opengeolab_resolve_package(
      glm
      glm::glm
      VERSION ${OPENGEOLAB_GLM_VERSION}
      GITHUB_REPOSITORY g-truc/glm
      GIT_TAG ${OPENGEOLAB_GLM_VERSION}
      CPM_OPTIONS "GLM_BUILD_TESTS OFF")
  ```
- [ ] 步骤 2：在 GLM 之后添加 glad CPM（不使用 `opengeolab_resolve_package`，因为 glad 需要特殊选项）：
  ```cmake
  CPMAddPackage(
      NAME glad
      GITHUB_REPOSITORY Dav1dde/glad
      VERSION 2.0.8
      OPTIONS "GLAD_API gl:core=4.6" "GLAD_GENERATOR c")
  ```
  如果 CPM glad 集成有问题，备选方案：在 `src/libs/render/third_party/` 放置预生成的 `glad.c` + `glad/gl.h`
- [ ] 步骤 3：在 `add_subdirectory` 区域（line 244 后，app 之前）添加：
  ```cmake
  add_subdirectory(src/libs/scene)
  add_subdirectory(src/libs/render)
  ```
- [ ] 步骤 4：创建 `src/libs/scene/` 目录结构（含空 CMakeLists.txt + include/src/test 子目录）
- [ ] 步骤 5：创建 `src/libs/render/` 目录结构（含空 CMakeLists.txt + include/src/test/shaders 子目录）
- [ ] 步骤 6：编写 `src/libs/scene/CMakeLists.txt`，暂时只有占位头文件和源文件列表：
  ```cmake
  # 占位，后续任务填充
  set(scene_public_headers)
  set(scene_sources)

  opengeolab_add_module(
      opengeolab_scene
      ALIAS_NAME Scene
      SOURCES ${scene_sources}
      PUBLIC_HEADERS ${scene_public_headers}
      PUBLIC_LINKS OpenGeoLab::Core glm::glm)
  ```
- [ ] 步骤 7：编写 `src/libs/render/CMakeLists.txt`，暂时只有占位：
  ```cmake
  set(render_public_headers)
  set(render_sources)

  opengeolab_add_module(
      opengeolab_render
      ALIAS_NAME Render
      SOURCES ${render_sources}
      PUBLIC_HEADERS ${render_public_headers}
      PUBLIC_LINKS OpenGeoLab::Core OpenGeoLab::Scene Qt6::Quick Qt6::OpenGL glm::glm
      PRIVATE_LINKS glad)
  ```
- [ ] 步骤 8：验证 CMake 配置通过
  ```
  cmake --preset local-debug
  ```
- [ ] 步骤 9：验证构建通过（空库）
  ```
  cmake --build build --config Debug --parallel 4
  ```

---

### 任务 1：Transform + BoundingBox（Scene 核心类型）

**文件：**
- 新增：`src/libs/scene/include/opengeolab/scene/transform.hpp`
- 新增：`src/libs/scene/include/opengeolab/scene/bounding_box.hpp`
- 新增：`src/libs/scene/src/transform.cpp`
- 新增：`src/libs/scene/src/bounding_box.cpp`
- 修改：`src/libs/scene/CMakeLists.txt`（将文件加入列表）

**参考：** 规格 §3.2 Transform, §3.3 BoundingBox

- [ ] 步骤 1：编写 `transform.hpp` — `Transform` 类：
  - 成员：`glm::vec3 m_position{0}`, `glm::quat m_rotation{1,0,0,0}`, `glm::vec3 m_scale{1}`
  - 方法：`matrix() → glm::mat4`, `setPosition()`, `setRotation()`, `setScale()`, `reset()`
  - 包含 `@file` + `@brief` Doxygen 注释
- [ ] 步骤 2：编写 `transform.cpp` — 使用 `glm::translate * glm::mat4_cast * glm::scale` 组合
- [ ] 步骤 3：编写 `bounding_box.hpp` — `BoundingBox` 类：
  - 成员：`glm::vec3 m_min{FLT_MAX}`, `glm::vec3 m_max{-FLT_MAX}`
  - 方法：`expand(point)`, `expand(other)`, `center()`, `radius()`, `isValid()`, `reset()`
  - 静态方法：`fromPositions(const float* data, size_t count, size_t stride)`
- [ ] 步骤 4：编写 `bounding_box.cpp`
- [ ] 步骤 5：更新 `src/libs/scene/CMakeLists.txt`，将 4 个文件加入列表
- [ ] 步骤 6：验证编译
  ```
  cmake --build build --target opengeolab_scene --config Debug --parallel 4
  ```

---

### 任务 2：SceneNode + SceneGraph（线程安全场景图）

**文件：**
- 新增：`src/libs/scene/include/opengeolab/scene/scene_node.hpp`
- 新增：`src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- 新增：`src/libs/scene/src/scene_graph.cpp`
- 新增：`src/libs/scene/test/scene_graph_test.cpp`
- 修改：`src/libs/scene/CMakeLists.txt`

**参考：** 规格 §3.4 SceneNode, §3.5 SceneGraph, §10.1 测试

- [ ] 步骤 1：编写 `scene_node.hpp` — `SceneNode` 结构体：
  ```cpp
  struct SceneNode {
      std::string id;
      Core::EntityTag entity;
      Transform transform;
      BoundingBox bounds;
      Core::VisualData visual;
      Core::RenderStyle style{Core::RenderStyle::SolidWithEdges};
      bool visible{true};
  };
  ```
- [ ] 步骤 2：编写 `scene_graph.hpp` — `SceneGraph` 类：
  - 内部结构：`struct Changeset { added, updated, removed }`
  - 公共方法：`addNode()`, `updateVisual()`, `removeNode()`, `findNode()`, `allNodeIds()`, `sceneBounds()`
  - Changeset 方法：`consumeChangeset()`, `hasChanges()`
  - 线程安全：`mutable std::shared_mutex m_mutex`
  - 节点存储：`std::unordered_map<std::string, SceneNode> m_nodes`
  - 变更缓冲：`Changeset m_pendingChanges`
- [ ] 步骤 3：编写 `scene_graph.cpp` — 实现所有方法：
  - 写操作（addNode/updateVisual/removeNode）用 `std::unique_lock`
  - 读操作（findNode/allNodeIds/sceneBounds）用 `std::shared_lock`
  - `consumeChangeset()` 用 `std::unique_lock`，使用 `std::exchange` 原子交换
- [ ] 步骤 4：编写 `scene_graph_test.cpp`（doctest），覆盖 Transform + BoundingBox + SceneGraph：
  ```
  Transform 测试：
  - 默认状态 → matrix() == identity
  - setPosition → matrix 含平移
  - setRotation → matrix 含旋转
  - setScale → matrix 含缩放
  - 组合 TRS → matrix 正确

  BoundingBox 测试：
  - 默认状态 → isValid() == false
  - expand(point) → 包含该点
  - expand(other) → 合并两个 box
  - center() → 几何中心
  - radius() → 半径 > 0
  - fromPositions → 从 float 数组构造
  - reset() → 恢复无效状态

  SceneGraph 测试：
  - addNode → findNode 能找到
  - addNode → Changeset.added 包含 id
  - updateVisual → Changeset.updated 包含 id
  - removeNode → findNode 返回 nullptr
  - removeNode → Changeset.removed 包含 id
  - consumeChangeset → pendingChanges 被清空
  - allNodeIds → 返回所有 id
  - sceneBounds → 合并所有节点包围盒
  ```
- [ ] 步骤 5：更新 CMakeLists.txt（添加源文件 + 测试目标）：
  ```cmake
  if (OPENGEOLAB_BUILD_TESTS)
      opengeolab_add_doctest_test(
          opengeolab_scene_graph_test
          SOURCES test/scene_graph_test.cpp
          LINKS OpenGeoLab::Scene)
  endif ()
  ```
- [ ] 步骤 6：构建测试
  ```
  cmake --build build --target opengeolab_scene_graph_test --config Debug --parallel 4
  ```
- [ ] 步骤 7：运行测试
  ```
  ctest --test-dir build -C Debug -R scene_graph --output-on-failure
  ```

---

### 任务 3：Camera（相机状态）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/camera.hpp`
- 新增：`src/libs/render/src/camera.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.2 Camera, 参考 `OGL/include/render/render_scene_controller.hpp` CameraState

- [ ] 步骤 1：编写 `camera.hpp` — `Camera` 类（eye-target-up 模型）：
  - 成员：`m_position{0,0,50}`, `m_target{0,0,0}`, `m_up{0,1,0}`, `m_fov{45}`, `m_nearPlane{0.1}`, `m_farPlane{10000}`
  - 方法：`viewMatrix()`, `projectionMatrix(aspect)`, getters/setters
  - `updateClipping(distance)` — 根据距离自动调整 near/far
  - `fitToBoundingBox(center, radius)` — 适配包围盒
  - `reset()` — 恢复默认
- [ ] 步骤 2：编写 `camera.cpp`：
  - `viewMatrix()` → `glm::lookAt(m_position, m_target, m_up)`
  - `projectionMatrix()` → `glm::perspective(glm::radians(m_fov), aspect, m_nearPlane, m_farPlane)`
  - `updateClipping()` → `near = max(0.01, distance * 0.001)`, `far = distance * 100`
  - `fitToBoundingBox()` → 计算距离使包围盒刚好充满视口，设置 position 和 target
- [ ] 步骤 3：更新 CMakeLists.txt（加入 camera 头文件和源文件）
- [ ] 步骤 4：验证编译
  ```
  cmake --build build --target opengeolab_render --config Debug --parallel 4
  ```

---

### 任务 4：TrackballController（Trackball 交互控制器）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/trackball_controller.hpp`
- 新增：`src/libs/render/src/trackball_controller.cpp`
- 新增：`src/libs/render/test/camera_test.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.3 TrackballController, 参考 `OGL/include/render/trackball_controller.hpp` + `.cpp`

- [ ] 步骤 1：编写 `trackball_controller.hpp` — 从参考项目适配接口，类型迁移：
  - `QVector3D` → `glm::vec3`
  - `QVector2D` → `glm::vec2`
  - `QQuaternion` → `glm::quat`
  - `QSizeF` → `float m_viewportWidth, m_viewportHeight`
  - `QPointF` → `float x, float y`
  - `Render::CameraState` → `Camera`
  - 接口：`setViewportSize(w, h)`, `setSpeed()`, `syncFromCamera()`, `begin()`, `update()`, `end()`, `wheelZoom()`
- [ ] 步骤 2：编写 `trackball_controller.cpp` — 适配参考实现（327 行）：
  - 辅助函数迁移：`lengthSquared()` → `glm::dot(v,v)`, `QVector3D::crossProduct()` → `glm::cross()`, `qSqrt()` → `std::sqrt()`, `qAcos()` → `std::acos()`, `qPow()` → `std::pow()`
  - `quatFromBasis()` → 使用 `glm::mat3` + `glm::quat_cast()` 简化
  - `QQuaternion::fromAxisAndAngle()` → `glm::angleAxis(radians, axis)`
  - `q.conjugated()` → `glm::conjugate(q)`
  - `q.rotatedVector(v)` → `q * v`（glm 直接支持 quat * vec3）
  - `q.normalize()` → `glm::normalize(q)`
  - Camera 成员直接访问 → 通过 getter/setter
- [ ] 步骤 3：编写 `camera_test.cpp`（doctest），覆盖 Camera + TrackballController：
  ```
  Camera 测试：
  - 默认状态 → viewMatrix / projectionMatrix 非零
  - fitToBoundingBox → position/target 更新
  - updateClipping → near/far 合理
  - reset → 回到默认值

  TrackballController 测试：
  - begin(orbit) + update → Camera position 变化
  - begin(pan) + update → Camera target 平移
  - wheelZoom → Camera distance 变化
  - syncFromCamera → 往返一致性
  ```
- [ ] 步骤 4：更新 CMakeLists.txt（源文件 + 测试目标）
- [ ] 步骤 5：构建测试
  ```
  cmake --build build --target opengeolab_camera_test --config Debug --parallel 4
  ```
- [ ] 步骤 6：运行测试
  ```
  ctest --test-dir build -C Debug -R camera --output-on-failure
  ```

---

### 任务 5：ShaderProgram（GLSL 编译/链接封装）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/shader_program.hpp`
- 新增：`src/libs/render/src/shader_program.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.4 ShaderProgram

**无单元测试**（需要 GL context）

- [ ] 步骤 1：编写 `shader_program.hpp`：
  - RAII：构造函数创建 program，析构函数 `glDeleteProgram`
  - `compile(vertex_source, fragment_source) → bool`
  - `bind()` / `release()`
  - Uniform setters：`setUniform(name, mat4)`, `setUniform(name, mat3)`, `setUniform(name, vec3)`, `setUniform(name, vec4)`, `setUniform(name, float)`, `setUniform(name, int)`
  - 不可复制，可移动
- [ ] 步骤 2：编写 `shader_program.cpp`：
  - `compile()` → `glCreateShader` + `glShaderSource` + `glCompileShader` + 错误日志
  - `glAttachShader` + `glLinkProgram` + 错误日志
  - Uniform → `glGetUniformLocation` + `glUniformMatrix4fv` / `glUniform3fv` 等
- [ ] 步骤 3：更新 CMakeLists.txt
- [ ] 步骤 4：验证编译
  ```
  cmake --build build --target opengeolab_render --config Debug --parallel 4
  ```

---

### 任务 6：GpuMesh（VAO/VBO/EBO RAII）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/gpu_mesh.hpp`
- 新增：`src/libs/render/src/gpu_mesh.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.5 GpuMesh

**无单元测试**（需要 GL context）

- [ ] 步骤 1：编写 `gpu_mesh.hpp`：
  - RAII：析构函数删除 VAO/VBO/EBO
  - `static fromSurface(SurfaceMesh) → GpuMesh` — stride=24, layout: [pos.xyz, norm.xyz]
  - `static fromEdges(EdgeMesh) → GpuMesh` — stride=12, layout: [pos.xyz]
  - `bind()` / `unbind()`
  - `indexCount()`, `primitiveType()`
  - 不可复制，可移动
- [ ] 步骤 2：编写 `gpu_mesh.cpp`：
  - `fromSurface()` → 交错 positions + normals 到单 VBO, 上传 indices 到 EBO
  - `fromEdges()` → positions 到 VBO, indices 到 EBO
  - Vertex attrib layout：
    - Surface: attr 0 = pos (3 float, stride 24, offset 0), attr 1 = normal (3 float, stride 24, offset 12)
    - Edge: attr 0 = pos (3 float, stride 12, offset 0)
- [ ] 步骤 3：更新 CMakeLists.txt
- [ ] 步骤 4：验证编译

---

### 任务 7：Shader 文件（GLSL 330 core）

**文件：**
- 新增：`src/libs/render/shaders/phong.vert`
- 新增：`src/libs/render/shaders/phong.frag`
- 新增：`src/libs/render/shaders/edge.vert`
- 新增：`src/libs/render/shaders/edge.frag`
- 修改：`src/libs/render/CMakeLists.txt`（添加 qt_add_resources）

**参考：** 规格 §5.1-5.4

- [ ] 步骤 1：编写 `phong.vert`（uniform 命名与规格 §5.1 一致）：
  - uniform: uModel (mat4), uView (mat4), uProjection (mat4), uNormalMatrix (mat3)
  - in: aPosition (vec3, location=0), aNormal (vec3, location=1)
  - out: vNormal (vec3), vFragPos (vec3)
  - 注：使用分离矩阵以支持世界空间光照计算
- [ ] 步骤 2：编写 `phong.frag`（与规格 §5.2 一致）：
  - uniform: uColor (vec4), uLightDir (vec3), uAmbient (float), uEyePos (vec3)
  - 双面 Blinn-Phong：diffuse `abs(dot(N, L))` + specular `pow(max(dot(N, H), 0), shininess)`
  - 输出 FragColor
- [ ] 步骤 3：编写 `edge.vert`：
  - uniform: uMVP (mat4)
  - in: aPosition (vec3, location=0)
- [ ] 步骤 4：编写 `edge.frag`：
  - uniform: uLineColor (vec4)
  - 直接输出 `FragColor = uLineColor`
- [ ] 步骤 5：在 CMakeLists.txt 末尾添加 `qt_add_resources`：
  ```cmake
  qt_add_resources(opengeolab_render "shaders"
      PREFIX "/shaders"
      FILES
          shaders/phong.vert
          shaders/phong.frag
          shaders/edge.vert
          shaders/edge.frag)
  ```
- [ ] 步骤 6：验证编译

---

### 任务 8：RenderScene（渲染线程侧场景快照）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/render_scene.hpp`
- 新增：`src/libs/render/src/render_scene.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.6 RenderScene

**无单元测试**（需要 GL context）

- [ ] 步骤 1：编写 `render_scene.hpp`：
  ```cpp
  struct RenderNode {
      std::string id;
      std::vector<GpuMesh> surfaces;    ///< Phong pass
      std::vector<GpuMesh> edges;       ///< Edge pass
      glm::vec4 surfaceColor{0.7f, 0.7f, 0.8f, 1.0f};
      glm::vec4 edgeColor{0.1f, 0.1f, 0.1f, 1.0f};
      glm::mat4 modelMatrix{1.0f};
      bool visible{true};
  };

  class RenderScene {
  public:
      void applyChangeset(const Scene::SceneGraph::Changeset& changeset,
                          const Scene::SceneGraph& graph);
      [[nodiscard]] const std::vector<RenderNode>& nodes() const;
      void clear();
  private:
      std::vector<RenderNode> m_nodes;
      std::unordered_map<std::string, size_t> m_indexMap;

      RenderNode createRenderNode(const Scene::SceneNode& node);
  };
  ```
- [ ] 步骤 2：编写 `render_scene.cpp`：
  - `applyChangeset()` → 遍历 added（createRenderNode + 加入 m_nodes），updated（替换），removed（移除）
  - `createRenderNode()` → 从 VisualData 的 SurfaceMesh/EdgeMesh 创建 GpuMesh（跳过空的）
- [ ] 步骤 3：更新 CMakeLists.txt
- [ ] 步骤 4：验证编译

---

### 任务 9：RenderEngine（OpenGL 两趟渲染器）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/render_engine.hpp`
- 新增：`src/libs/render/src/render_engine.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.7 RenderEngine

- [ ] 步骤 1：编写 `render_engine.hpp`：
  ```cpp
  class RenderEngine {
  public:
      bool initialize();          ///< 首次调用时编译 shader
      void resize(int w, int h);
      void render(const RenderScene& scene, const Camera& camera);
  private:
      ShaderProgram m_phongShader;
      ShaderProgram m_edgeShader;
      int m_width{1};
      int m_height{1};
      bool m_initialized{false};

      void renderSurfaces(const RenderScene& scene, const glm::mat4& vp);
      void renderEdges(const RenderScene& scene, const glm::mat4& vp);
      static std::string loadShaderSource(const QString& resource_path);
  };
  ```
- [ ] 步骤 2：编写 `render_engine.cpp`：
  - `initialize()` → gladLoadGL, 编译两组 shader（从 Qt resource 加载）
  - `render()` →
    1. `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`
    2. `glEnable(GL_DEPTH_TEST)`
    3. `renderSurfaces()` → Phong pass，正常 `glDepthRange(0.0, 1.0)`
    4. `renderEdges()` → Edge pass，`glDepthRange(0.0, 0.9999)` 解决 z-fighting
    5. 恢复 `glDepthRange(0.0, 1.0)`
  - `renderSurfaces()` → 计算 MVP, ModelView, NormalMatrix, 设置 uniform, 遍历 node.surfaces
  - `renderEdges()` → 计算 MVP, 设置 uniform, `glLineWidth(1.5)`, 遍历 node.edges
  - `loadShaderSource()` → `QFile(":/shaders/xxx").readAll()`
- [ ] 步骤 3：更新 CMakeLists.txt
- [ ] 步骤 4：验证编译

---

### 任务 10：ViewportItem + ViewportRenderer（QQuickFramebufferObject）

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/viewport_item.hpp`
- 新增：`src/libs/render/include/opengeolab/render/viewport_renderer.hpp`
- 新增：`src/libs/render/src/viewport_item.cpp`
- 新增：`src/libs/render/src/viewport_renderer.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

**参考：** 规格 §4.8 ViewportItem, §4.9 ViewportRenderer, 参考 `OGL/include/app/opengl_viewport.hpp`

- [ ] 步骤 1：编写 `viewport_item.hpp`：
  - 继承 `QQuickFramebufferObject`
  - 成员：`Camera m_camera`, `TrackballController m_trackball`, `SceneGraph* m_sceneGraph`
  - 方法：`createRenderer()`, `setSceneGraph()`, `sceneGraph()`, `camera()`, `fitAll()`
  - 重载：`mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `wheelEvent`, `geometryChange`
  - 在构造函数中：`setAcceptedMouseButtons(Qt::AllButtons)`, `setFlag(ItemAcceptsInputMethod)`
- [ ] 步骤 2：编写 `viewport_renderer.hpp`：
  - 继承 `QQuickFramebufferObject::Renderer`
  - 成员：`RenderEngine m_engine`, `RenderScene m_renderScene`, `Camera m_cameraCopy`
  - 方法：`createFramebufferObject()`, `synchronize()`, `render()`
- [ ] 步骤 3：编写 `viewport_item.cpp`：
  - 鼠标交互映射：
    - Ctrl+左键 → Orbit
    - Shift+左键 / 中键 → Pan
    - 右键 → Zoom
    - 滚轮 → wheelZoom
  - `geometryChange()` → `m_trackball.setViewportSize(w, h)` + `update()`
  - `fitAll()` → 从 sceneGraph 获取 sceneBounds，Camera.fitToBoundingBox
- [ ] 步骤 4：编写 `viewport_renderer.cpp`：
  - `createFramebufferObject()` → 设置多重采样格式
  - `synchronize()` →
    1. 从 ViewportItem 复制 Camera
    2. 从 SceneGraph `consumeChangeset()`（需要 shared_lock 读 graph）
    3. `m_renderScene.applyChangeset()`
  - `render()` →
    1. 首次调用 `m_engine.initialize()`
    2. `m_engine.resize(fbo->width(), fbo->height())`
    3. `m_engine.render(m_renderScene, m_cameraCopy)`
    4. `update()` 请求持续重绘（有 changeset 时）
- [ ] 步骤 5：更新 CMakeLists.txt
- [ ] 步骤 6：验证编译
  ```
  cmake --build build --target opengeolab_render --config Debug --parallel 4
  ```

---

### 任务 11：QML 集成（ViewportPanel 改造 + 类型注册）

**文件：**
- 修改：`src/app/resource/qml/sections/ViewportPanel.qml`
- 修改：`src/app/src/main.cpp`
- 修改：`src/app/CMakeLists.txt`

**参考：** 规格 §8.1 ViewportPanel.qml, §7.3 main.cpp

- [ ] 步骤 1：在 `main.cpp` 中添加 ViewportItem QML 类型注册：
  ```cpp
  #include <opengeolab/render/viewport_item.hpp>
  // 在 QML engine 创建之前：
  qmlRegisterType<OpenGeoLab::Render::ViewportItem>(
      "OpenGeoLab.Render", 1, 0, "ViewportItem");
  ```
- [ ] 步骤 2：在 `src/app/CMakeLists.txt` 的 `target_link_libraries` 中添加：
  ```cmake
  OpenGeoLab::Scene
  OpenGeoLab::Render
  ```
- [ ] 步骤 3：改造 `ViewportPanel.qml` — 替换 Canvas 占位为 ViewportItem + Fit All 按钮：
  ```qml
  import OpenGeoLab.Render 1.0

  Item {
      id: root
      required property AppTheme theme

      ViewportItem {
          id: viewport
          anchors.fill: parent
      }

      // Fit All 按钮（右上角）
      Rectangle {
          anchors.top: parent.top
          anchors.right: parent.right
          anchors.margins: 8
          width: 32; height: 32
          radius: root.theme.radiusSmall
          color: fitAllArea.containsMouse
              ? root.theme.surfaceHover : root.theme.surfaceSecondary
          opacity: 0.85

          Text {
              anchors.centerIn: parent
              text: "⊞"
              font.pixelSize: 18
              color: root.theme.textPrimary
          }

          MouseArea {
              id: fitAllArea
              anchors.fill: parent
              hoverEnabled: true
              onClicked: viewport.fitAll()
          }
      }

      // 空态提示（Phase 2: 场景非空时隐藏）
      Text {
          anchors.centerIn: parent
          text: qsTr("3D Viewport")
          font.pixelSize: 28
          font.weight: Font.Bold
          color: root.theme.tint(root.theme.textTertiary, 0.35)
          visible: false
      }
  }
  ```
- [ ] 步骤 4：验证编译
  ```
  cmake --build build --config Debug --parallel 4
  ```

---

### 任务 12：SceneBridge + ModuleDataNotifier 扩展（数据流桥接）

**文件：**
- 新增：`src/app/src/scene_bridge.h`
- 新增：`src/app/src/scene_bridge.cpp`
- 修改：`src/app/include/opengeolab/app/module_data_notifier.h`
- 修改：`src/app/src/module_data_notifier.cpp`
- 修改：`src/app/src/main.cpp`
- 修改：`src/app/CMakeLists.txt`

**参考：** 规格 §7.2 SceneBridge, §7.4 ModuleDataNotifier 扩展

- [ ] 步骤 1：在 `module_data_notifier.h` 添加信号：
  ```cpp
  Q_SIGNALS:
      void geometryDataChanged();
      void meshDataChanged();  // 新增
  ```
- [ ] 步骤 2：在 `module_data_notifier.cpp` 构造函数中添加 mesh 订阅：
  ```cpp
  auto meshHandle = dispatcher.onModuleDataChanged("mesh",
      [this](Core::ModuleDataEvent /*event*/) {
          QMetaObject::invokeMethod(this, &ModuleDataNotifier::meshDataChanged,
                                   Qt::QueuedConnection);
      });
  if (meshHandle.isConnected()) {
      m_connections.push_back(std::move(meshHandle));
  }
  ```
- [ ] 步骤 3：编写 `scene_bridge.h`：
  ```cpp
  class SceneBridge : public QObject {
      Q_OBJECT
  public:
      SceneBridge(Command::CommandDispatcher& dispatcher,
                  Scene::SceneGraph& sceneGraph,
                  QObject* parent = nullptr);
  public slots:
      void onGeometryDataChanged();
      void onMeshDataChanged();
  private:
      void syncGeometryToScene();
      void syncMeshToScene();
      Command::CommandDispatcher& m_dispatcher;
      Scene::SceneGraph& m_sceneGraph;
      std::unordered_map<uint32_t, std::string> m_shapeNodeMap;
      std::unordered_map<uint32_t, std::string> m_meshNodeMap;
  };
  ```
- [ ] 步骤 4：编写 `scene_bridge.cpp`：
  - `syncGeometryToScene()` →
    1. `dispatcher.findModule("geometry")` → `dynamic_pointer_cast<GeometryModule>` → `shapeStore()`
    2. 获取当前所有 shape ID
    3. 对比 m_shapeNodeMap：新增 → addNode，删除 → removeNode，已有 → updateVisual
  - `syncMeshToScene()` → 同理用 MeshModule + meshStore()
- [ ] 步骤 5：在 `main.cpp` 中创建 SceneBridge 并连接信号：
  ```cpp
  #include "scene_bridge.h"
  #include <opengeolab/scene/scene_graph.hpp>

  // 在 dispatcher 和 module_notifier 创建之后：
  OpenGeoLab::Scene::SceneGraph sceneGraph;
  SceneBridge sceneBridge(dispatcher, sceneGraph);

  QObject::connect(&module_notifier, &ModuleDataNotifier::geometryDataChanged,
                   &sceneBridge, &SceneBridge::onGeometryDataChanged);
  QObject::connect(&module_notifier, &ModuleDataNotifier::meshDataChanged,
                   &sceneBridge, &SceneBridge::onMeshDataChanged);

  // 在 QML engine 加载后，找到 ViewportItem 设置 sceneGraph
  // (或通过 context property 注入)
  ```
- [ ] 步骤 6：在 `src/app/CMakeLists.txt` 的 `qt_add_executable` 源文件列表添加：
  ```cmake
  src/scene_bridge.cpp
  ```
  在 `qt_add_qml_module` 的 SOURCES 中添加：
  ```cmake
  src/scene_bridge.h
  ```
- [ ] 步骤 7：验证编译
  ```
  cmake --build build --config Debug --parallel 4
  ```

---

### 任务 13：最终验证 — 全量构建 + 测试 + 格式化

**文件：** 所有新增和修改的文件

- [ ] 步骤 1：全量构建（RelWithDebInfo）
  ```
  cmake --preset local-relwithdebinfo
  cmake --build build --config RelWithDebInfo --parallel 4
  ```
- [ ] 步骤 2：全量测试
  ```
  ctest --test-dir build -C RelWithDebInfo --output-on-failure
  ```
- [ ] 步骤 3：clang-format 格式化所有新文件
  ```
  clang-format -i src/libs/scene/include/opengeolab/scene/*.hpp
  clang-format -i src/libs/scene/src/*.cpp
  clang-format -i src/libs/scene/test/*.cpp
  clang-format -i src/libs/render/include/opengeolab/render/*.hpp
  clang-format -i src/libs/render/src/*.cpp
  clang-format -i src/libs/render/test/*.cpp
  clang-format -i src/app/src/scene_bridge.*
  ```
- [ ] 步骤 4：clang-tidy 检查关键文件（命名规范等）
  ```
  clang-tidy src/libs/scene/src/*.cpp -- -std=c++20
  clang-tidy src/libs/render/src/camera.cpp src/libs/render/src/trackball_controller.cpp -- -std=c++20
  ```
- [ ] 步骤 5：手动验证（启动应用）：
  1. 创建一个 Box（Geometry → Create Box）
  2. 确认 Box 出现在 3D 视口中（Phong 着色 + 线框）
  3. Ctrl+左键拖拽 → 轨道旋转
  4. Shift+左键拖拽 → 平移
  5. 右键拖拽 → 缩放
  6. 滚轮 → 缩放
  7. 网格剖分 → 确认网格渲染到视口
- [ ] 步骤 6：确认无回归 — 所有原有测试仍通过
- [ ] 步骤 7：请求用户确认后 git commit

---

## 注意事项

1. **glad 集成风险**：glad 2.x CPM 集成可能需要特殊配置。如果 CPM 方式失败，回退方案是使用 glad 在线生成器手动生成 `glad.c` + `glad/gl.h` 放在 `src/libs/render/third_party/glad/`。
2. **TrackballController 适配**：参考实现 327 行，类型迁移量大但算法不变。关键：`QQuaternion::fromAxisAndAngle(axis, degrees)` → glm 用弧度制 `glm::angleAxis(radians, axis)`。
3. **线程安全**：SceneGraph 的 shared_mutex 是 Phase 1 的核心同步机制。测试要验证并发读写不会死锁。
4. **Qt 资源系统**：shader 文件通过 `qt_add_resources` 嵌入，运行时用 `QFile(":/shaders/phong.vert")` 加载。
5. **测试框架**：使用 doctest（不是 GTest），遵循现有 `TEST_CASE` + `CHECK` 模式。
6. **Git 工作流**：提交前必须征求用户确认。
