# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务
1. 修改 geometry/mesh 的 id 与 uid 使用 uint64_t（使用其中的 56 位，另外 8 位为 type），全局修改当前已有接口；
2. 设计一套 render data，支持 geometry 和 mesh 的渲染数据，包含顶点位置、颜色、法线等信息，并且要包含 connectivity 支持后续 pick 扩展，例如鼠标选取 part 类型，可以从像素中拿到相关 edge 或者 face 反推出对应 part。
3. 完善 geometry / mesh document 构建 render data 接口，可以获取render 数据并且支持增量更新，例如当一个 face 的颜色发生改变时，只需要更新对应 face 的 render data 而不需要重新构建整个 document 的 render data。
4. 检查 mesh document 构建数据是否合理尤其  gmshTypeToMeshElementType 接口，确保所有的 gmsh 元素类型都能正确转换为 mesh document 中的元素类型。
5. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
6. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
7. 保证最终代码可以编译通过，并正确执行。

整体数据设计如下：
                    ┌──────────────────────────┐
                    │        OCC Geometry      │
                    │   TopoDS_Shape / Face    │
                    └────────────┬─────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────┐
                    │     GeometryAdapter      │
                    │  Triangulate / Extract   │
                    └────────────┬─────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────┐
                    │   GeoRenderData (CPU)    │
                    │ vertices / indices       │
                    │ + EntityKey              │
                    └────────────┬─────────────┘
                                 │
                                 │
                                 │
                    ┌──────────────────────────┐
                    │        Gmsh Mesh         │
                    │ nodes / elements         │
                    └────────────┬─────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────┐
                    │       MeshAdapter        │
                    │  Surface extract         │
                    └────────────┬─────────────┘
                                 │
                                 ▼
                    ┌──────────────────────────┐
                    │   MeshRenderData (CPU)   │
                    │ vertices / indices       │
                    │ + EntityKey              │
                    └────────────┬─────────────┘
                                 │
                                 ▼
          ┌──────────────────────────────────────────┐
          │         RenderBucketBuilder               │
          │  merge by primitive type + material      │
          │  assign global renderId                  │
          └────────────┬─────────────────────────────┘
                       ▼
          ┌──────────────────────────────────────────┐
          │              RenderBucket                │
          │ big VBO + EBO + SubMeshRange            │
          │ idToEntity mapping                      │
          └────────────┬─────────────────────────────┘
                       ▼
          ┌──────────────────────────────────────────┐
          │               RenderChunk                │
          │ split by element count / AABB            │
          └────────────┬─────────────────────────────┘
                       ▼
          ┌──────────────────────────────────────────┐
          │               RendererCore               │
          │  MainPass / PickPass / Culling / BVH    │
          └────────────┬─────────────────────────────┘
                       ▼
                         OpenGL