# OpenGeoLab 渲染系统移植指南

> 基于 commit `83797b5` (FBO GPU Picking) 和 `c02acda` (Render & Selection Enhancement) 的完整渲染架构分析与分模块移植方案。

---

## 一、整体架构概览

```
┌──────────────────────────────────────────────────────────────────┐
│                    Qt QML / GUI 线程                             │
│  ┌──────────────┐  ┌──────────────────────┐  ┌───────────────┐  │
│  │  GLViewport  │  │ RenderSceneController│  │ RenderSelect  │  │
│  │ (鼠标/键盘)  │  │  (相机/文档桥接)     │  │   Manager     │  │
│  └──────┬───────┘  └──────────┬───────────┘  └───────┬───────┘  │
│         │                     │                      │          │
│         │    GLViewportRender::synchronize()          │          │
│         │    构建 SceneFrameState ← 合并所有 GUI 状态  │          │
├─────────┼─────────────────────┼──────────────────────┼──────────┤
│         ▼                     ▼                      ▼          │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │           RenderSceneImpl (渲染线程, 无单例访问)          │   │
│  │  ┌──────────────┐                                        │   │
│  │  │ PickResolver │ ← GL-free 实体解析 (优先级 + 层级)     │   │
│  │  └──────────────┘                                        │   │
│  │  ┌────────────────┐ ┌────────────┐ ┌──────────────────┐ │   │
│  │  │ GeometryPass   │ │  MeshPass  │ │    PickPass      │ │   │
│  │  │ (CAD 几何渲染) │ │ (FEM 网格) │ │ (离屏 FBO 拾取)  │ │   │
│  │  └───────┬────────┘ └─────┬──────┘ └────────┬─────────┘ │   │
│  │          │                │                 │            │   │
│  │  ┌───────┴────────────────┴─────────────────┴──────────┐│   │
│  │  │              共享核心层 (Core)                        ││   │
│  │  │  GpuBuffer  │  ShaderProgram  │  PickFbo            ││   │
│  │  └─────────────────────────────────────────────────────┘│   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                  数据构建层 (Builder)                     │   │
│  │  GeometryRenderBuilder  │  MeshRenderBuilder             │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

### 渲染帧生命周期

```
每帧流程:
1. synchronize()  — GUI线程阻塞, 渲染线程执行:
   a. 捕获光标位置和拾取动作
   b. 从 RenderSceneController 构建 SceneFrameState
      (renderData指针、camera、view/proj矩阵、xRay、displayMode)
   c. 调用 m_renderScene->synchronize(state):
      - 缓存 SceneFrameState
      - 更新 GeometryPass / MeshPass GPU 缓冲 (仅在 dirty 时)
      - 同步 meshDisplayMode
      - 重建 PickResolver 查找表 (仅在 geometryDirty 时)
2. render()       — 渲染线程执行 (无单例访问):
   a. 重置 GL 状态 (Qt SceneGraph 可能污染)
   b. 清屏
   c. 使用缓存的 m_frameState 执行 GeometryPass + MeshPass
3. processPicking() — 如有点击事件:
   a. 渲染到 PickFBO (使用缓存矩阵)
   b. 读取 7x7 像素区域
   c. 委托 PickResolver 解析最高优先级实体
   d. Wire/Part 模式反向解析
   e. 更新 RenderSelectManager
4. processHover()   — 每帧如拾取启用:
   a. 同上流程但更新 hover 状态
```

---

## 二、文件清单与模块归属

### 模块划分总表

| 模块 | 目录/文件 | 职责 |
|------|-----------|------|
| **Module 0: 数据类型** | `include/render/render_types.hpp` | 实体类型枚举、掩码、域判断 |
| | `include/render/render_data.hpp` | 顶点、DrawRange、语义树、RenderData |
| | `include/util/color_map.hpp` + `src/util/color_map.cpp` | 颜色调色板 |
| | `include/util/core_identity.hpp` | 通用 ID 模板 |
| **Module 1: 核心框架** | `include/render/render_scene.hpp` | IRenderScene 抽象接口 + SceneFrameState |
| | `include/render/pick_resolver.hpp` + `src/render/pick_resolver.cpp` | GL-free 拾取解析 (优先级+层级) |
| | `include/render/render_scene_controller.hpp` + `.cpp` | 相机、文档桥接、场景控制 |
| | `include/render/render_select_manager.hpp` + `.cpp` | 拾取/选择/悬停状态管理 |
| | `src/render/core/gpu_buffer.hpp/.cpp` | VAO/VBO/IBO 管理 |
| | `src/render/core/shader_program.hpp/.cpp` | Shader 编译封装 |
| | `src/render/render_sceneImpl.hpp/.cpp` | IRenderScene 具体实现 (编排) |
| **Module 2: Geometry Pass** | `src/render/pass/geometry_pass.hpp/.cpp` | CAD 几何渲染通道 |
| | `src/render/builder/geometry_render_builder.hpp/.cpp` | OCC→GPU 数据构建 |
| **Module 3: Mesh Pass** | `src/render/pass/mesh_pass.hpp/.cpp` | FEM 网格渲染通道 |
| | `src/render/builder/mesh_render_builder.hpp/.cpp` | 网格→GPU 数据构建 |
| **Module 4: Pick Pass** | `src/render/pass/pick_pass.hpp/.cpp` | 离屏拾取通道 |
| | `src/render/core/pick_fbo.hpp/.cpp` | 离屏 FBO 管理 |
| **Module 5: 视口集成** | `include/app/opengl_viewport.hpp` + `src/app/opengl_viewport.cpp` | QML 视口 (GUI线程) |
| | `include/app/opengl_viewport_render.hpp` + `src/app/opengl_viewport_render.cpp` | 渲染器 (渲染线程) |
| | `include/render/trackball_controller.hpp` + `.cpp` | 轨迹球相机控制 |

---

## 三、分模块移植方案

### ========================================================
### Module 0: 数据类型与基础工具 (无 OpenGL 依赖)
### ========================================================

**目标**: 定义所有渲染相关的数据结构，不包含任何 GL 调用。此模块可独立编译验证。

**涉及文件**:
- `include/render/render_types.hpp`
- `include/render/render_data.hpp`
- `include/util/color_map.hpp` + `src/util/color_map.cpp`
- `include/util/core_identity.hpp`

#### 关键数据结构

**1. RenderEntityType** (`render_types.hpp:16-37`)
```cpp
enum class RenderEntityType : uint8_t {
    // 几何域 (BRep)
    Vertex=0, Edge=1, Wire=2, Face=3, Shell=4, Solid=5, CompSolid=6, Compound=7, Part=8,
    // 网格域 (FEM)
    MeshNode=9, MeshLine=10, MeshTriangle=11, MeshQuad4=12,
    MeshTetra4=13, MeshHexa8=14, MeshPrism6=15, MeshPyramid5=16,
    None=17,
};
```
- `RenderEntityTypeMask` — 32位掩码，用于按类型过滤
- `isGeometryDomain()` / `isMeshDomain()` — 判断实体所属领域
- `toRenderEntityType()` — 从 Geometry/Mesh 类型映射

**2. RenderVertex** (`render_data.hpp:125-130`) — 48字节 GPU 顶点
```cpp
struct RenderVertex {
    float m_position[3];   // 世界坐标 (12B)
    float m_normal[3];     // 法线 (12B)
    float m_color[4];      // RGBA 颜色 (16B)
    uint64_t m_pickId;     // 编码拾取 ID (8B) → GPU 中用 uvec2 传递
};
```

**3. PickId** (`render_data.hpp:96-113`) — 编码方案
```
布局: [56位 UID | 8位 Type]
编码: (uid << 8) | type
解码: type = encoded & 0xFF, uid = encoded >> 8
零值 = 背景 (无拾取)
```

**4. DrawRange / DrawRangeEx** (`render_data.hpp:139-183`)
```cpp
struct DrawRange {
    uint32_t m_vertexOffset, m_vertexCount;
    uint32_t m_indexOffset, m_indexCount;  // 0 = 非索引绘制
    PrimitiveTopology m_topology;         // Points/Lines/Triangles
};

struct DrawRangeEx {
    DrawRange m_range;
    RenderNodeKey m_entityKey;  // 实体标识 (type + uid)
    uint64_t m_partUid;         // 父零件 UID
    uint64_t m_wireUid;         // 父线框 UID (Edge 专属)
};
```

**5. RenderNode** (`render_data.hpp:197-208`) — 语义树节点
```cpp
struct RenderNode {
    RenderNodeKey m_key;            // 实体标识
    RenderColor m_color;            // 显示颜色
    bool m_visible{true};
    BoundingBox3D m_bbox;
    map<RenderPassType, vector<DrawRange>> m_drawRanges;  // 每 Pass 的绘制范围
    vector<RenderNode> m_children;   // Part → Solid → Face/Edge/Vertex 层级
};
```

**6. PickResolutionData** (`render_data.hpp`) — Pick 辅助查找表
```cpp
/// Pick-specific lookup tables for entity hierarchy resolution.
/// 仅用于 RenderSceneImpl 的 processHover / processPicking。
struct PickResolutionData {
    map<uint64_t, vector<uint64_t>> m_edgeToWireUids;    // Edge→Wire 查找表
    map<uint64_t, vector<uint64_t>> m_wireToEdgeUids;    // Wire→Edge 反查
    map<uint64_t, uint64_t> m_wireToFaceUid;             // Wire→Face 归属
    map<uint64_t, pair<NodeId,NodeId>> m_meshLineNodes;  // 网格线→节点对
    void clear();
    void clearGeometry();
    void clearMesh();
};
```

**7. RenderData** (`render_data.hpp`) — 顶层数据容器
```cpp
struct RenderData {
    vector<RenderNode> m_roots;                          // 语义树根
    map<RenderPassType, RenderPassData> m_passData;      // 扁平 GPU 缓冲
    PickResolutionData m_pickData;                        // Pick 辅助查找表
    BoundingBox3D m_sceneBBox;
    bool m_geometryDirty, m_meshDirty;
};
```
> 注意: pick 辅助查找表已提取到 `PickResolutionData` 中，与渲染数据分离。
> `RenderVertex.m_pickId` 和 `DrawRangeEx` 的 pick 相关字段保留原位，
> 因为它们同时服务于渲染高亮和 pick 解析。

**8. ColorMap** (`color_map.hpp`) — 颜色服务
- `getColorForPartId(uid)` — 按 UID 循环取调色板颜色
- `getEdgeVertexHoverColor()` — 边/顶点悬停色 #ff7f00
- `getFaceHoverColor()` — 面悬停色 #4b55e9
- `getEdgeVertexSelectionColor()` — 边/顶点选中色 #ff165d
- `getFaceSelectionColor()` — 面选中色 #4116ff
- `getBackgroundColor()` — 视口背景色

**验证方法**:
```
✅ 可独立编译为静态库
✅ 单元测试: PickId 编解码、RenderEntityType 枚举映射、ColorMap 颜色索引
✅ 无 OpenGL 上下文依赖
```

---

### ========================================================
### Module 1: 核心渲染框架 (通用 GL 基础设施)
### ========================================================

**目标**: 搭建 OpenGL 基础设施 — shader 编译、GPU buffer 管理、渲染场景接口。
此阶段只初始化 GL 资源，清屏显示背景色，不画任何几何体。

**涉及文件**:
- `include/render/render_scene.hpp` — 抽象接口 + SceneFrameState
- `include/render/pick_resolver.hpp` + `src/render/pick_resolver.cpp` — GL-free 拾取解析
- `src/render/core/shader_program.hpp/.cpp` — Shader 管理
- `src/render/core/gpu_buffer.hpp/.cpp` — VAO/VBO/IBO 管理
- `src/render/render_sceneImpl.hpp/.cpp` — 场景编排 (简化版)
- `include/render/render_scene_controller.hpp` + `.cpp` — 相机与数据桥接
- `include/render/render_select_manager.hpp` + `.cpp` — 选择状态管理

#### 关键类详解

**1. SceneFrameState** (`render_scene.hpp`) — 每帧同步数据快照
```cpp
/// All GUI-thread state needed by the renderer, captured during synchronize().
struct SceneFrameState {
    const RenderData* renderData{nullptr};   // 指向 controller 的数据快照
    QVector3D cameraPos;                     // 相机世界坐标
    QMatrix4x4 viewMatrix;                   // 观察矩阵
    QMatrix4x4 projMatrix;                   // 投影矩阵
    bool xRayMode{false};                    // X光模式
    RenderDisplayModeMask meshDisplayMode{RenderDisplayModeMask::None};
};
```
- 在 `GLViewportRender::synchronize()` 中构建，打包所有 GUI 线程状态
- `RenderSceneImpl` 缓存此结构，render 期间不再访问任何单例

**2. IRenderScene** (`render_scene.hpp`)
```cpp
class IRenderScene {
    virtual void initialize() = 0;                          // 创建 GPU 资源
    virtual bool isInitialized() const = 0;                 // 查询初始化状态
    virtual void setViewportSize(QSize) = 0;                // 视口大小变化
    virtual void synchronize(const SceneFrameState&) = 0;   // 每帧同步数据
    virtual void render() = 0;                              // 每帧渲染 (无参数)
    virtual void processPicking(const PickingInput&) = 0;   // 点击拾取
    virtual void processHover(int px, int py) = 0;          // 悬停更新 (无矩阵参数)
    virtual void cleanup() = 0;                             // 释放资源
};
```
- **关键变化**: `render()` 和 `processHover()` 不再接收相机/矩阵参数
- 所有状态通过 `synchronize()` 预先缓存到 `SceneFrameState`
- 渲染线程执行期间不访问 GUI 线程单例
- 使用 `SceneRendererFactory` 工厂模式创建实例
- 渲染器通过组件系统注册: `"SceneRenderer"` ID

**3. ShaderProgram** (`shader_program.hpp/.cpp`)
```cpp
class ShaderProgram {
    bool compile(vertex_src, fragment_src);  // 编译链接
    void bind() / release();                 // 激活/停用
    // Uniform 设置
    setUniformMatrix4(name, QMatrix4x4);
    setUniformVec3(name, QVector3D);
    setUniformVec4(name, r, g, b, a);
    setUniformFloat(name, val);
    setUniformInt(name, val);
    setUniformUvec2(name, x, y);             // 整数 uniform (用于 PickId)
};
```
- 所有 GL 操作通过 `QOpenGLContext::currentContext()` 获取函数指针
- 编译失败时输出详细着色器日志

**4. GpuBuffer** (`gpu_buffer.hpp/.cpp`)
```cpp
class GpuBuffer {
    void initialize();                    // 创建 VAO/VBO/IBO
    void upload(RenderPassData& data);    // 上传顶点/索引数据
    void bindForDraw() / unbind();        // 绑定/解绑 VAO
};

// 顶点属性布局 (48B stride):
//   location 0: position  vec3   offset  0
//   location 1: normal    vec3   offset 12
//   location 2: color     vec4   offset 24
//   location 3: pickId    uvec2  offset 40  (glVertexAttribIPointer!)
```

**5. CameraState** (`render_scene_controller.hpp:23-60`)
```cpp
struct CameraState {
    QVector3D m_position{0,0,50}, m_target{0,0,0}, m_up{0,1,0};
    float m_fov{45}, m_nearPlane{0.1f}, m_farPlane{10000};

    QMatrix4x4 viewMatrix();          // lookAt 矩阵
    QMatrix4x4 projectionMatrix(aspect);  // 正交投影
    void fitToBoundingBox(bbox);      // 自适应包围盒 (1.5x 对角线距离)
    void updateClipping(distance);    // 动态近远面
};
```
- **注意**: 使用正交投影 (非透视)，适合技术 CAD 查看

**6. RenderSceneController** (`render_scene_controller.hpp:78-200`)
```cpp
class RenderSceneController : Singleton {
    CameraState m_cameraState;
    RenderData m_renderData;           // 完整渲染数据快照
    map<UID, PartVisibility> m_partVisibility;  // 零件可见性
    atomic<bool> m_xRayMode;           // X光模式
    atomic<uint8_t> m_meshDisplayMode; // 网格显示模式掩码

    void refreshScene();               // 从文档重建 RenderData
    void fitToScene();                 // 相机适配场景
    // 相机预设: setFrontView / setBackView / setTopView 等
    void toggleXRayMode();             // 切换 X光
    void cycleMeshDisplayMode();       // 循环网格显示模式
    const RenderData& renderData();    // 给渲染线程读取

    Signal<SceneUpdateType> m_sceneNeedsUpdate;  // 通知视口重绘
};
```
- **关键设计**: Controller 在 GUI 线程构建 RenderData，渲染线程只读访问
- 通过 `subscribeToSceneNeedsUpdate()` 信号通知视口调度重绘
- 订阅 GeometryDocument / MeshDocument 变化事件，自动重建数据

**7. RenderSelectManager** (`render_select_manager.hpp`)
```cpp
class RenderSelectManager : Singleton {
    // 拾取控制
    void setPickEnabled(bool);
    void setPickTypes(RenderEntityTypeMask);
    RenderEntityTypeMask normalizePickTypes(mask);  // 互斥规则

    // 选择管理
    bool addSelection(uid, type);
    bool removeSelection(uid, type);
    void clearSelection();

    // 悬停管理
    void setHoverEntity(uid, type, partUid, wireUid);
    void clearHover();

    // Wire 边追踪
    void setHoveredWireEdges(edgeUids);
    void addSelectedWireEdges(wireUid, edgeUids);

    // 信号
    Signal<bool> m_pickEnabledChanged;
    Signal<RenderEntityTypeMask> m_pickSettingsChanged;
    Signal<PickResult, SelectionChangeAction> m_selectionChanged;
    Signal<> m_hoverChanged;
};
```

**互斥规则** (`normalizePickTypes`):
```
互斥组 (一次只能拾取一种):
  - Part (独占)
  - Solid (独占)
  - Wire (独占)
  - 所有 Mesh 类型 (与几何类型互斥)

可组合:
  - Vertex + Edge + Face (几何拓扑元素可组合)
  - MeshNode + MeshLine + MeshElement (网格拓扑可组合)
```

**8. PickResolver** (`pick_resolver.hpp/.cpp`) — GL-free 拾取解析
```cpp
class PickResolver {
    void rebuild(triangles, lines, points, pickData);  // 重建查找表
    ResolvedPickResult resolve(pickIds);               // 优先级解析
    const vector<uint64_t>& wireEdges(wireUid);        // Wire→Edge 查询
    void clear();
};

struct ResolvedPickResult {
    uint64_t uid;                // 实体 UID
    RenderEntityType type;       // 实体类型
    uint64_t partUid;            // 父 Part UID
    uint64_t wireUid;            // 父 Wire UID (Edge 专属)
    uint64_t faceContextUid;     // 面上下文 (Wire 消歧用)
};
```
- **纯数据逻辑**: 无 OpenGL 依赖，可独立单元测试
- 从 `RenderSceneImpl` 提取的职责: 优先级排序 + 层级解析
- 内部维护: `m_entityToPartUid`, `m_edgeToWireUids`, `m_wireToEdgeUids`, `m_wireToFaceUid`
- 优先级: Vertex > MeshNode > Edge > MeshLine > Face > Shell > Wire > Solid > Part > MeshElements

**9. RenderSceneImpl — 简化版 (仅框架)**

移植第一步只需实现:
```cpp
void RenderSceneImpl::synchronize(const SceneFrameState& state) {
    m_frameState = state;  // 缓存所有 GUI 线程状态
    // 此时不更新任何 Pass 缓冲
}

void RenderSceneImpl::render() {
    auto* f = QOpenGLContext::currentContext()->functions();
    // 重置 GL 状态
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    f->glDepthMask(GL_TRUE);
    f->glEnable(GL_DEPTH_TEST);
    // 清屏
    const auto& bg = ColorMap::instance().getBackgroundColor();
    f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 此时不执行任何 Pass — 使用 m_frameState 中的矩阵 (无单例访问)
}
```

**验证方法**:
```
✅ 启动应用后显示灰色背景 (ColorMap 背景色)
✅ ShaderProgram 可编译简单顶点/片段着色器
✅ GpuBuffer 可创建 VAO 并上传空数据
✅ CameraState 计算 view/projection 矩阵正确
✅ RenderSceneController 单例可访问
✅ PickResolver 可独立单元测试 (优先级排序、层级解析)
✅ SceneFrameState 在 synchronize() 中正确构建
✅ render() 期间无 RenderSceneController::instance() 调用
✅ 无几何体渲染 — 纯背景色
```

---

### ========================================================
### Module 2: Geometry Pass — CAD 几何渲染
### ========================================================

**目标**: 渲染 BRep 几何体 — 光照表面、线框、顶点，支持高亮和 X-ray 模式。

**涉及文件**:
- `src/render/pass/geometry_pass.hpp/.cpp`
- `src/render/builder/geometry_render_builder.hpp/.cpp`

#### GeometryRenderBuilder — 数据构建

**输入** (`geometry_render_builder.hpp:23-27`):
```cpp
struct GeometryRenderInput {
    const EntityIndex& m_entityIndex;
    const EntityRelationshipIndex& m_relationshipIndex;
    TessellationOptions m_options;
};
```

**构建流程**:
1. 遍历所有 Part 实体
2. 对每个 Part 下的 Face → `generateFaceMesh()` — OCC 三角化 → RenderVertex + DrawRangeEx
3. 对每个 Edge → `generateEdgeMesh()` — OCC 离散化 → 线段顶点
4. 对每个 Vertex → `generateVertexMesh()` — 单点
5. 构建 `m_pickData.m_edgeToWireUids` / `m_pickData.m_wireToEdgeUids` / `m_pickData.m_wireToFaceUid` 查找表
6. 输出到 `RenderData.m_passData[Geometry]`

**颜色策略**:
- Face 颜色: 从 Part 颜色派生 (每个 Part 从 ColorMap 调色板取色)
- Edge 颜色: `ColorMap::getEdgeColor()` 固定色
- Vertex 颜色: `ColorMap::getVertexColor()` 固定色

#### GeometryPass — 渲染执行

**着色器** (`geometry_pass.cpp` 内联 GLSL):

Surface Shader (光照):
```glsl
// 顶点着色器
layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec4 a_color;
// → 输出 world pos, normal, color

// 片段着色器 — 光照模型:
ambient     = 0.35
headlamp    = abs(dot(normal, view_dir)) * 0.55   // 头灯 (方向跟随视线)
sky_light   = max(dot(normal, up), 0) * 0.15       // 天光
ground      = max(dot(normal, -up), 0) * 0.05      // 地面反光
// X-ray 模式: 预乘 alpha 混合, 25% 不透明度
// 高亮: hover/select 时混合高亮色
```

Flat Shader (线框/点):
```glsl
// 简单 passthrough, 覆盖颜色 (使用 uniform 高亮色)
```

**绘制顺序** (`geometry_pass.cpp render()`):
```
1. 绘制表面三角形 (GL_TRIANGLES)
   - glEnable(GL_POLYGON_OFFSET_FILL)  // 防止 Z-fighting
   - glPolygonOffset(1.0, 1.0)
   - 遍历 m_triangleRanges, 设置 per-entity 高亮 uniform
   - X-ray 模式下启用 GL_BLEND (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)

2. 绘制线框边 (GL_LINES)
   - glLineWidth(1.0 / 2.0 / 1.5)  // 普通/悬停/选中
   - 遍历 m_lineRanges, 检查 RenderSelectManager 高亮状态

3. 绘制顶点 (GL_POINTS)
   - glPointSize(5.0 / 10.0 / 7.5)  // 普通/悬停/选中
   - 遍历 m_pointRanges
```

**高亮逻辑** (`geometry_pass.cpp`):
```
对每个 DrawRangeEx:
  1. Part 模式 → 检查 isPartHovered/isPartSelected → 整个 Part 高亮
  2. Wire 模式 → 检查 isWireHovered/isWireSelected → 展开到所有 Edge
  3. 普通模式 → 检查 isEntityHovered/isSelected → 单实体高亮
```

**验证方法**:
```
✅ 加载 STEP 文件后显示光照表面
✅ 线框边正确叠加在表面上 (无 Z-fighting)
✅ 顶点以点形式显示
✅ 多 Part 时每个 Part 颜色不同
✅ X-ray 模式: 面半透明 (25% 不透明度)
✅ (暂不需要高亮 — 等 Module 4 Pick 完成后才验证)
```

---

### ========================================================
### Module 3: Mesh Pass — FEM 网格渲染
### ========================================================

**目标**: 渲染有限元网格 — 表面/线框/节点，支持显示模式切换。

**涉及文件**:
- `src/render/pass/mesh_pass.hpp/.cpp`
- `src/render/builder/mesh_render_builder.hpp/.cpp`

#### MeshRenderBuilder — 数据构建

**输入** (`mesh_render_builder.hpp:18-22`):
```cpp
struct MeshRenderInput {
    span<const MeshNode> m_nodes;
    span<const MeshElement> m_elements;
    RenderColor m_surfaceColor{0.65, 0.75, 0.85, 1.0};
};
```

**缓冲区布局** (关键设计):
```
GPU 顶点缓冲:
  [ Surface Triangles | Wireframe Lines | Node Points ]
  |← surfaceCount →|← wireframeCount →|← nodeCount →|

不使用索引 — 全顶点绘制 (用 glDrawArrays + offset)
```

**构建流程**:
1. **表面三角化**: 遍历元素 → Tri3/Quad4/Tetra4/Hexa8/Prism6/Pyramid5 → 生成外表面三角形
   - 每个三角形顶点带法线 (面法线)、表面颜色、PickId
   - PickId = encode(MeshElementType, elementUID)
2. **线框生成**: 遍历元素边 → 线段顶点
   - 颜色 = `ColorMap::getMeshLineColor()`
   - PickId = encode(MeshLine, sequentialLineId)
   - 构建 `m_pickData.m_meshLineNodes` 查找表 (lineId → nodeA, nodeB)
3. **节点生成**: 遍历所有节点 → 点顶点
   - 颜色 = `ColorMap::getMeshNodeColor()`
   - PickId = encode(MeshNode, nodeId)

#### MeshPass — 渲染执行

**着色器** (`mesh_pass.cpp` 内联 GLSL):

与 GeometryPass 类似，但增加了 **着色器端拾取高亮**:
```glsl
// 片段着色器额外 uniform:
uniform uvec2 u_hoverPickId;        // 当前悬停实体 PickId
uniform uvec2 u_selectPickIds[32];  // 当前选中实体 PickId 数组
uniform int u_selectCount;          // 选中数量

// 比较逻辑:
if (v_pickId == u_hoverPickId) → 悬停高亮
for (i in selectPickIds) if (v_pickId == selectPickIds[i]) → 选中高亮
```
- 这是 MeshPass 的**独特设计**: 高亮在主渲染中完成，不需额外 Pass
- 支持 "仅高亮模式": 线框模式下只渲染被悬停/选中的面

**显示模式** (位掩码循环):
```
模式 1: Wireframe + Points (默认)
模式 2: Surface + Points
模式 3: Surface + Points + Wireframe
→ 通过 RenderSceneController::cycleMeshDisplayMode() 切换
```

**绘制** (`mesh_pass.cpp render()`):
```cpp
// 根据显示模式决定画什么:
if (Surface bit) → glDrawArrays(GL_TRIANGLES, 0, surfaceVertexCount)
if (Wireframe bit) → glDrawArrays(GL_LINES, surfaceCount, wireframeCount)
if (Points bit) → glDrawArrays(GL_POINTS, surfaceCount+wireCount, nodeCount)
```

**验证方法**:
```
✅ 加载网格后以线框+节点模式显示
✅ 按显示模式切换按钮循环三种模式
✅ 网格边和几何线框颜色不同 (可区分)
✅ 不同元素类型 (Tri/Quad/Tet/Hex) 均正确渲染
✅ (着色器端高亮需 Module 4 完成后验证)
```

---

### ========================================================
### Module 4: Pick Pass — GPU 拾取
### ========================================================

**目标**: 实现离屏 FBO 拾取，支持精确的实体选中和悬停反馈。

**涉及文件**:
- `src/render/core/pick_fbo.hpp/.cpp`
- `src/render/pass/pick_pass.hpp/.cpp`
- `include/render/pick_resolver.hpp` + `src/render/pick_resolver.cpp` (GL-free 解析)
- `src/render/render_sceneImpl.cpp` (processHover / processPicking 编排)

#### PickFbo — 离屏帧缓冲

```cpp
class PickFbo {
    // FBO 配置:
    //   颜色附件: GL_RG32UI 纹理 (2 x uint32 = 64位 PickId)
    //   深度附件: GL_DEPTH_COMPONENT24 渲染缓冲
    //   采样: GL_NEAREST (整数纹理不可插值)

    bool initialize(w, h);                // 创建 FBO
    bool resize(w, h);                    // 重建附件
    void bind() / unbind();               // 绑定/解绑 + glViewport
    uint64_t readPickId(x, y);            // 单像素读回
    vector<uint64_t> readPickRegion(cx, cy, radius);  // 区域读回
};
```

**PickId 在 FBO 中的存储**:
```
uint64_t pickId = (uid << 8) | type
→ GPU 写入: fragPickId = uvec2(low32, high32)
→ CPU 读回: glReadPixels(GL_RG_INTEGER, GL_UNSIGNED_INT)
→ 重组: (high << 32) | low
```

#### PickPass — 拾取渲染

**拾取着色器**:
```glsl
// 顶点着色器
layout(location=0) in vec3 a_position;
layout(location=3) in uvec2 a_pickId;
out flat uvec2 v_pickId;
// MVP 变换, 点大小 12px

// 片段着色器
in flat uvec2 v_pickId;
layout(location=0) out uvec2 fragPickId;
void main() { fragPickId = v_pickId; }
```

**选择性渲染** (`pick_pass.cpp renderToFbo()`):
```
仅渲染 pickMask 匹配的图元:
几何缓冲:
  - Face/Solid/Part/Shell/Wire mask → 画 triangleRanges
  - Edge mask → 画 lineRanges (线宽=3.0 便于拾取)
  - Vertex mask → 画 pointRanges (点大小=12px)

网格缓冲:
  - MeshElement mask → 画 surface 三角形
  - MeshLine mask → 画 wireframe 线段
  - MeshNode mask → 画 node 点
```

#### 拾取解析架构

**职责划分**:
- `PickResolver` (GL-free): 优先级排序 + 实体层级解析 (可独立测试)
- `RenderSceneImpl`: GL 编排 — 渲染到 FBO、读取像素、委托 PickResolver、更新 SelectManager

**processHover()** (`render_sceneImpl.cpp`):
```
1. 检查拾取是否启用
2. 构建 effectiveMask:
   - Wire 模式 → mask |= Edge | Face (用于共享边消歧)
   - Part 模式 → mask |= Face | Edge (用于零件归属查找)
3. 渲染到 PickFBO (使用缓存的 m_frameState 矩阵)
4. 读取 7x7 区域 (readPickRegion(px, py, 3))
5. 委托 m_pickResolver.resolve(pickIds):
   - 按优先级排序: Vertex > MeshNode > Edge > MeshLine > Face > ...
   - 解析 partUid (resolvePartUid)
   - 解析 wireUid (resolveWireUid — 用 Face 上下文消歧共享边)
   - 返回 ResolvedPickResult
6. 过滤:
   - Wire 模式下拾取到 Face → 清除悬停
   - Part 模式下拾取到 Face → 解析到父 Part
7. 更新 RenderSelectManager:
   - setHoverEntity(uid, type, partUid, wireUid)
   - setHoveredWireEdges(wireEdges)
```

**processPicking()** (`render_sceneImpl.cpp`):
```
与 processHover 类似，但:
1. 支持 Add/Remove 动作
2. 同样委托 m_pickResolver.resolve() 解析
3. Wire 反向查找: resolved.wireUid → 作为 Wire 选中
4. Part 反向查找: resolved.partUid → 作为 Part 选中
5. 点击空白: 清空所有选择
6. 选中 Wire 时: addSelectedWireEdges 存储完整边环
```

**共享边消歧** — 关键细节:
```
问题: 一条 Edge 可能属于两个 Face, 因此属于两个 Wire
解决: resolveWireUid(edgeUid, faceUid)
  1. 查找 edge 所属的所有 Wire
  2. 如果有 Face 上下文, 优先选择属于该 Face 的 Wire
  3. Face 上下文来自 7x7 区域中同时拾取到的 Face ID
```

**验证方法**:
```
✅ 启用拾取后鼠标悬停高亮 (Vertex→Edge→Face 优先级)
✅ 点击选中实体，再次点击取消选中
✅ Part 模式: 悬停/点击整个零件高亮
✅ Wire 模式: 悬停/点击整条线框高亮 (包括共享边)
✅ 网格元素拾取: 节点/线/三角形/四面体等
✅ Ctrl+点击 多选
✅ 点击空白清空选择
```

---

### ========================================================
### Module 5: 视口集成与交互 (后续合并)
### ========================================================

**说明**: 此模块在渲染完成后合并，包含 UI 交互和鼠标操作。

**涉及文件**:
- `include/app/opengl_viewport.hpp` + `src/app/opengl_viewport.cpp`
- `include/app/opengl_viewport_render.hpp` + `src/app/opengl_viewport_render.cpp`
- `include/render/trackball_controller.hpp` + `.cpp`
- QML 文件: `ViewToolBar.qml`, `Selector.qml`, `ViewToolButton.qml` 等

#### GLViewport (GUI 线程)

```cpp
class GLViewport : public QQuickFramebufferObject {
    // 鼠标事件 → 轨迹球相机控制
    mousePressEvent   → trackball.begin(pos, mode, camera)
    mouseMoveEvent    → trackball.update(pos, camera) → update()
    mouseReleaseEvent → trackball.end() + 设置 pendingPickAction
    wheelEvent        → trackball.wheelZoom(steps, camera)
    hoverMoveEvent    → m_cursorPos = pos → update()  // 触发悬停拾取

    // 按钮映射:
    左键     → Orbit (旋转)
    中键     → Pan (平移)
    右键     → Zoom (缩放)
    Ctrl+左键 → Pick Add
    Ctrl+右键 → Pick Remove
};
```

#### GLViewportRender (渲染线程)

```cpp
class GLViewportRender : public QQuickFramebufferObject::Renderer {
    void synchronize(item) {
        // GUI→渲染线程数据传递:
        // 1. 捕获光标位置
        m_cursorPos = viewport->cursorPosition();
        m_devicePixelRatio = viewport->currentDevicePixelRatio();
        // 2. 捕获待处理的拾取动作
        if (pendingPickAction) → 构建 PickingInput (不含矩阵)
        // 3. 构建 SceneFrameState 并同步到渲染场景
        auto& controller = RenderSceneController::instance();
        SceneFrameState state;
        state.renderData = &controller.renderData();
        state.cameraPos = camera.m_position;
        state.viewMatrix = camera.viewMatrix();
        state.projMatrix = camera.projectionMatrix(aspect);
        state.xRayMode = controller.isXRayMode();
        state.meshDisplayMode = controller.meshDisplayMode();
        m_renderScene->synchronize(state);
        // 此后渲染线程不再访问 controller
    }

    void render() {
        // 1. 懒初始化 m_renderScene
        // 2. 设置 viewport 大小
        // 3. m_renderScene->render()  ← 无参数，使用缓存状态
        // 4. 如有 pendingPick → m_renderScene->processPicking(input)
        // 5. 如拾取启用 → m_renderScene->processHover(px, py)  ← 无矩阵参数
        // 6. 每次使用 PickFBO 后重新绑定视口 FBO
    }
};
```
- **关键变化**: `render()` 中不再调用 `RenderSceneController::instance()`
- 所有 controller 访问集中在 `synchronize()` (GUI 线程阻塞期间安全访问)

#### TrackballController
```cpp
class TrackballController {
    enum Mode { None, Orbit, Pan, Zoom };

    void begin(pos, mode, camera);    // 开始拖拽
    void update(pos, camera);         // 更新相机
    void end();                       // 结束拖拽
    void wheelZoom(steps, camera);    // 滚轮缩放

    // 参数:
    orbitScale = 2.2     // 旋转灵敏度
    panScale = 0.0015    // 平移灵敏度
    zoomBase = 0.90      // 指数缩放底数
};
```

---

## 四、依赖关系图

```
Module 0 (数据类型)
    │
    ├──→ Module 1 (核心框架)
    │       │
    │       ├──→ Module 2 (Geometry Pass)
    │       │
    │       ├──→ Module 3 (Mesh Pass)
    │       │
    │       └──→ Module 4 (Pick Pass)
    │               │
    │               └──→ 依赖 Module 2/3 的 GPU Buffer 和 DrawRangeEx
    │
    └──→ Module 5 (视口集成)
            │
            └──→ 依赖 Module 1 (IRenderScene + Controller)
```

**推荐移植顺序**:
```
0 → 1 → 2 → 3 → 4 → 5
```
每个模块完成后可独立验证，不影响后续模块的开发。

---

## 五、外部依赖清单

| 依赖 | 用途 | 替代方案 |
|------|------|---------|
| `QOpenGLContext` / `QOpenGLFunctions` / `QOpenGLExtraFunctions` | 所有 GL 调用 | 可替换为 GLAD/GLEW |
| `QMatrix4x4` / `QVector3D` / `QQuaternion` | 数学运算 | 可替换为 glm |
| `QQuickFramebufferObject` | Qt Quick OpenGL 集成 | 平台相关 |
| `Kangaroo::Util::FactoryTraits` | 组件工厂 | 自定义工厂或直接实例化 |
| `Kangaroo::Util::NonCopyMoveable` | 禁止拷贝基类 | 手写 delete 规则 |
| `Util::Signal` / `Util::ScopedConnection` | 信号/槽 | Qt Signal/Slot 或 sigslot 库 |
| OCC (OpenCascade) | BRep 三角化/离散化 | 仅 GeometryRenderBuilder 依赖 |

---

## 六、关键设计模式总结

### 1. 语义树 + 扁平缓冲 并存
- `RenderNode` 树反映几何/网格层级关系 (精细可见性控制)
- `RenderPassData` 扁平数组 (高效 GPU 提交)
- `DrawRange`/`DrawRangeEx` 映射树节点到缓冲区域

### 2. 64位 PickId 编码
- `[56-bit UID | 8-bit Type]` — 一个 uint64 编码完整实体身份
- GPU 端用 `uvec2` 传输 (GLSL 无原生 uint64)
- FBO 用 `RG32UI` 格式存储两个 uint32 分量

### 3. 多通道架构 (Multi-Pass)
- GeometryPass: 光照 + 线框 + 点
- MeshPass: 表面/线框/节点 (可切换模式)
- PickPass: 离屏 ID 渲染 (仅按需)
- 每个 Pass 独立管理 Shader + Buffer + 状态

### 4. 区域拾取 + 优先级解析
- 不是单像素拾取, 而是 7x7 区域
- 按优先级排序: Vertex > Edge > Face > Wire > Part
- 解决了"点击细线很难拾中"的交互问题

### 5. 线程安全设计
- `RenderSelectManager`: mutex 守护所有状态
- `RenderSceneController`: 原子操作 (xray, displayMode)
- GUI 线程写入状态, 渲染线程只读
- `synchronize()` 在场景图锁下执行, 保证数据一致
- `SceneFrameState` 在 `synchronize()` 中一次性构建，`render()` 期间只使用缓存副本
- `renderData()` 返回的引用依赖 Qt Scene Graph 的 `synchronize()` 屏障实现
  跨线程安全: GUI 线程在 synchronize() 期间被阻塞，render() 期间不修改
  render data。如数据更新频率增加，应考虑 shared_mutex 或 double-buffering

### 6. Synchronize 数据分离
- **问题**: 渲染线程 `render()` 直接访问 GUI 线程单例 (隐式竞态风险)
- **方案**: 新增 `SceneFrameState` 结构，在 `synchronize()` 中一次性捕获所有状态
- `GLViewportRender::synchronize()` 构建 `SceneFrameState` (renderData, camera, matrices, xRay, displayMode)
- `RenderSceneImpl::synchronize()` 缓存 state，更新 GPU 缓冲，重建 PickResolver
- `render()` / `processHover()` / `processPicking()` 使用缓存的 `m_frameState`，不访问任何单例
- **收益**: 明确的线程边界，消除渲染线程对 GUI 单例的运行时依赖

### 7. PickResolver 提取
- **问题**: `RenderSceneImpl` 混合 GL 编排和 pick 数据解析两类职责
- **方案**: 提取 `PickResolver` 类 — 纯数据逻辑，无 GL 依赖
- `PickResolver::rebuild()`: 从 DrawRangeEx 和 PickResolutionData 构建查找表
- `PickResolver::resolve()`: 优先级排序 + partUid/wireUid 层级解析
- `RenderSceneImpl`: 仅负责 GL 操作 (渲染到 FBO、读取像素) + 委托 PickResolver
- **收益**: pick 逻辑可独立单元测试，RenderSceneImpl 职责更清晰

### 8. 数据职责分离
- `RenderData` 只包含渲染所需的核心数据 (语义树、GPU 缓冲、BBox、dirty 标记)
- `PickResolutionData` (嵌套于 `RenderData.m_pickData`) 包含 pick 辅助查找表
  (Edge→Wire, Wire→Edge, Wire→Face, MeshLine→Nodes)
- `RenderVertex.m_pickId` 和 `DrawRangeEx` 的 pick 相关字段保留在原结构中，
  因为它们同时服务于渲染高亮 (MeshPass shader) 和 pick 解析

### 9. 脏标记延迟更新
- `RenderData.m_geometryDirty` / `m_meshDirty`
- 仅在数据变化时重新上传 GPU 缓冲
- 拾取时通过掩码选择性渲染, 避免冗余绘制

---

## 七、移植检查清单

### Module 0 检查清单
- [ ] `RenderEntityType` 枚举及掩码运算正确
- [ ] `PickId` 编码/解码往返测试通过
- [ ] `RenderVertex` 结构体大小 == 48 字节
- [ ] `ColorMap` 颜色索引循环正确
- [ ] `RenderData` 的 clear/clearGeometry/clearMesh 工作正常
- [ ] `PickResolutionData` 的 clear/clearGeometry/clearMesh 正确清理对应域数据

### Module 1 检查清单
- [ ] `ShaderProgram::compile()` 编译/链接成功, 错误有日志
- [ ] `GpuBuffer::initialize()` 创建 VAO/VBO/IBO
- [ ] `GpuBuffer::upload()` 顶点属性指针正确 (尤其 pickId 用 glVertexAttribIPointer)
- [ ] `CameraState::viewMatrix()` 和 `projectionMatrix()` 矩阵正确
- [ ] `SceneFrameState` 在 synchronize() 中正确构建并传递
- [ ] `RenderSceneImpl::synchronize()` 缓存 state 并更新 GPU 缓冲
- [ ] `RenderSceneImpl::render()` 使用缓存的 `m_frameState` (无单例访问)
- [ ] `PickResolver` 可独立构造和测试 (无 GL 依赖)
- [ ] `PickResolver::resolve()` 优先级排序正确
- [ ] `PickResolver::rebuild()` 构建正确的查找表
- [ ] `RenderSceneImpl` 可通过工厂创建
- [ ] `RenderSceneController` 信号订阅和通知工作正常
- [ ] `RenderSelectManager` 线程安全 (多线程访问无崩溃)
- [ ] 启动后视口显示背景色 (无几何体)

### Module 2 检查清单
- [ ] `GeometryRenderBuilder::build()` 生成正确的三角化数据
- [ ] Face 三角形有正确法线
- [ ] 每个 Part 颜色来自 ColorMap 调色板
- [ ] Edge/Vertex 使用固定颜色
- [ ] `DrawRangeEx` 包含正确的 partUid 和 wireUid
- [ ] `GeometryPass::render()` 表面有光照效果
- [ ] `glPolygonOffset` 消除 Z-fighting
- [ ] X-ray 模式面半透明 (premultiplied alpha)

### Module 3 检查清单
- [ ] `MeshRenderBuilder::build()` 按布局生成 [surface|wireframe|nodes]
- [ ] 各元素类型 (Tri/Quad/Tet/Hex/Prism/Pyramid) 三角化正确
- [ ] 显示模式切换 (Wireframe+Points → Surface+Points → 全部)
- [ ] 网格线和几何边颜色可区分
- [ ] 着色器端 PickId 比较逻辑正确

### Module 4 检查清单
- [ ] `PickFbo` 创建 RG32UI + Depth24 FBO, completeness 检查通过
- [ ] `PickFbo::readPickId()` 和 `readPickRegion()` 读回正确
- [ ] `PickPass::renderToFbo()` 按 mask 选择性渲染
- [ ] 点大小 12px / 线宽 3.0 便于拾取
- [ ] `PickResolver::resolve()` 优先级排序: Vertex > Edge > Face > Part
- [ ] `PickResolver` Wire 模式共享边消歧正确 (用 Face 上下文)
- [ ] `PickResolver` Part 模式子实体→Part 反向查找正确
- [ ] `processHover()` / `processPicking()` 使用缓存的 `m_frameState` 矩阵
- [ ] 拾取后重新绑定视口 FBO (`framebufferObject()->bind()`)

### Module 5 检查清单
- [ ] `synchronize()` 构建 SceneFrameState 并传递到 RenderSceneImpl
- [ ] `synchronize()` 中所有 controller 访问集中完成
- [ ] `render()` 中无 `RenderSceneController::instance()` 调用
- [ ] `render()` 调用简化为 `m_renderScene->render()` (无参数)
- [ ] `processHover()` 调用简化为 `m_renderScene->processHover(px, py)` (无矩阵)
- [ ] 轨迹球旋转/平移/缩放手感正常
- [ ] 悬停拾取每帧执行 (当 pickEnabled 时)
- [ ] 点击选中/取消选中
- [ ] HiDPI 缩放 (devicePixelRatio) 正确
- [ ] 拾取 FBO 使用后重新绑定视口 FBO
