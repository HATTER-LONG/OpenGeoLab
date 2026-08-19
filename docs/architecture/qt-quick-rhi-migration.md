# Qt Quick RHI renderer

## 结论

OpenGeoLab 的视口适合采用 `QQuickRhiItem + QQuickRhiItemRenderer + QRhi`。
这三个类型是同一方案的不同层次，并不是互斥选项：Item 接入 QML，Renderer
承接 Qt Quick 渲染线程生命周期，QRhi 负责缓冲、纹理、管线和命令录制。

本实验分支直接替换 OpenGL，不保留双后端或兼容开关。Qt Quick 可按平台选择
D3D11/D3D12、Metal、Vulkan 或 OpenGL；OpenGeoLab 本身不再调用 OpenGL API。

## 方案比较

| 方案 | 适用范围 | 对 OpenGeoLab 的评价 |
| --- | --- | --- |
| `QSGGeometryNode` / `QSGGeometry` | 少量 2D/简单 3D 几何，交给 Scene Graph 排序和材质系统 | 不合适作为主渲染器。多 Pass、离屏拾取、截图、MSDF 图集和资源更新会被拆成大量 Node/Material，控制力不足 |
| `QQuickRhiItem` | 在 QML 中提供由 RHI 纹理承载的自定义内容 | 合适的视口外壳，但仍需要 Renderer 实现实际绘制 |
| `QQuickRhiItemRenderer + QRhi` | 自定义多 Pass、显式资源和命令管理 | 最合理。能保持现有 RenderPipeline 的集中控制，同时由 Qt 映射到不同图形 API |

`QSGGeometry` 仍可用于橡皮筋、辅助 HUD 等简单 Scene Graph 覆盖层，但不应承担
CAE 主场景渲染。

## 新结构

1. `RenderSceneSnapshot` 在 CPU 侧展平可见场景，完成索引重定位、DrawRange
   整理、实体查询和拾取所需的位置访问；不包含任何图形 API 对象。
2. `RhiViewport` 是 `QQuickRhiItem`，负责 QML 属性、输入和 GUI/渲染线程同步。
3. `RhiViewportRenderer` 是 `QQuickRhiItemRenderer`，只在 Qt Quick 渲染线程
   同步状态、录制 QRhi 命令和安排异步纹理回读。
4. `RenderPipeline` 直接持有 `QRhiBuffer`、`QRhiShaderResourceBindings` 和多组
   `QRhiGraphicsPipeline`；着色器由 `qt_add_shaders` 离线生成 `.qsb`。
5. Qt 6.9 的 `QQuickRhiItem` 是公开 API；QRhi 头仍来自 QtGui 的版本化 RHI
   目录，因此 render/app 私有链接 `Qt6::GuiPrivate`，构建产物需要与 Qt 次版本一致。

## 本分支已完成

- 移除 Qt OpenGL、glad 和 stb 依赖以及旧 OpenGL Pass/FBO/Shader/Buffer 源码。
- Qt Quick 不再强制 OpenGL backend 或创建 OpenGL surface format。
- Surface、x-ray、wireframe、point、selection 和 hover 使用 QRhi pipeline。
- 场景资源可在 RHI、RenderPass、MSAA 或场景容量变化时重建。
- 点击、悬停、圆形区域和框选保持原接口；当前实验实现使用 CPU 投影拾取，避免
  同步 GPU readback 阻塞渲染线程。
- 截图改为对 `QQuickRhiItem` resolved color texture 做异步 QRhi readback。
- Render 模块的后端无关行为测试可独立运行。

## 尚未达到功能等价的部分

- 宽线暂时调用 QRhi line width，Metal/D3D/Vulkan 对宽线能力并不一致；正式方案
  应在顶点着色器中把线段展开为屏幕空间三角形。
- 点仍依赖 point topology/point size；正式方案应改为实例化 billboard quad。
- Tessellation overlay 已从三角索引生成三条边并重绘三角顶点；颜色和像素级效果
  仍需跨后端截图校准。
- MSDF 标签的 CPU 锚点、堆叠和字体度量仍保留，但 QRhi 图集纹理与 glyph quad
  pass 尚未接回。
- CPU 拾取尚未按深度判定最前命中，矩形框选目前采用外接圆近似；需要补精确
  矩形相交和深度排序，或实现 RHI integer selection target。
- 需要在 Windows D3D、macOS Metal、Linux Vulkan/OpenGL 上做图像和设备丢失测试。

这些缺口意味着该分支现在是可编译的 RHI 原型，而不是可直接合并到主分支的
功能等价实现。
