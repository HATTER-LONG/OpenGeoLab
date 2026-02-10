# Render Architecture

## Overview

OpenGeoLab uses a **modular pass-based OpenGL rendering pipeline** with three layers:

1. **SceneRenderer (Facade)** — Public API maintaining backward compatibility
2. **RendererCore (Central Engine)** — Manages GL context, shaders, resources, and pass execution
3. **RenderPass Implementations** — Individual rendering stages

```
                         ┌──────────────────────┐
                         │   SceneRenderer       │  ← Facade (public API)
                         │   (scene_renderer.hpp)│
                         └─────────┬────────────┘
                                   │ delegates
                         ┌─────────▼────────────┐
                         │   RendererCore        │  ← GL resource manager
                         │   (renderer_core.hpp) │
                         └─────────┬────────────┘
                                   │ owns & executes
           ┌───────────────────────┼───────────────────────┐
           │                       │                       │
  ┌────────▼─────────┐  ┌─────────▼──────────┐  ┌────────▼─────────┐
  │  GeometryPass    │  │   MeshPass          │  │  HighlightPass   │
  │  (Faces/Edges/   │  │   (FEM elements/    │  │  (Stencil-based  │
  │   Vertices)      │  │    nodes)           │  │   outlines)      │
  └──────────────────┘  └────────────────────┘  └──────────────────┘
                                                        │ delegates
                                               ┌────────▼──────────┐
                                               │ OutlineHighlight   │
                                               │ (IHighlightStrategy│
                                               │  implementation)   │
                                               └───────────────────┘

  Separately owned by SceneRenderer (on-demand execution):
  ┌──────────────────┐
  │  PickingPass      │  ← Integer-encoded entity picking via FBO
  └──────────────────┘
```

## Pass Execution Order

Per frame, `RendererCore::render()` executes passes in registration order:

1. **GeometryPass** — Clear screen, render faces (Phong lighting), edges, vertices
2. **MeshPass** — Render FEM mesh element wireframes and node points
3. **HighlightPass** — Render selection/hover outlines via stencil buffer

**PickingPass** is called on-demand by `SceneRenderer::renderPicking()` (not every frame).

## Components

### SceneRenderer (`scene_renderer.hpp`)

Thin facade over RendererCore + registered passes. Maintains the original public API for backward compatibility with `GLViewportRenderer` in `opengl_viewport.cpp`.

**Responsibilities:**
- Initialize the render pipeline (register passes, create shaders)
- Forward API calls to the appropriate pass or RendererCore
- Own the PickingPass (on-demand, not in main pass list)

### RendererCore (`renderer_core.hpp`)

Central GPU resource manager and pass compositor.

**Responsibilities:**
- Initialize OpenGL and check GL 4.3+ compatibility
- Compile and cache shader programs (`"mesh"`, `"outline"`, `"pick"`, `"pick_line"`)
- Upload mesh data to GPU via `RenderBatch`
- Execute passes with proper context (matrices, viewport info)
- Cleanup GPU resources

### RenderPass (`render_pass.hpp`)

Abstract interface for pluggable rendering stages:

```cpp
class RenderPass {
    virtual const char* name() const = 0;
    virtual void initialize(QOpenGLFunctions& gl) = 0;
    virtual void resize(QOpenGLFunctions& gl, const QSize& size) = 0;
    virtual void execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) = 0;
    virtual void cleanup(QOpenGLFunctions& gl) = 0;
    bool isEnabled() const;
    void setEnabled(bool enabled);
};
```

**RenderPassContext** provides shared rendering state:
- View/projection/MVP matrices
- Viewport size and aspect ratio
- Camera position
- Pointer to RendererCore (for shader/batch access)

### GeometryPass (`geometry_pass.hpp`)

Renders geometry entities (faces, edges, vertices) with Phong lighting.

**Features:**
- Phong lighting model (ambient 0.3, specular 0.3)
- Per-entity hover and selection color overrides
- Polygon offset for face/edge depth ordering
- Per-part visibility filtering via `RenderSceneController`

**Rendering order:** Faces → Edges → Vertices

### MeshPass (`mesh_pass.hpp`)

Renders FEM mesh elements and nodes, separate from geometry to allow independent visibility control.

**Features:**
- Mesh elements rendered as wireframe lines
- Mesh nodes rendered as points
- Hover/selection highlighting
- Per-part mesh visibility filtering via `RenderSceneController`

### PickingPass (`picking_pass.hpp`)

Integer-encoded picking for precise entity identification.

**Method:**
- Renders to an FBO with `GL_R32UI` texture (unsigned integer per pixel)
- Each entity encoded as: `(EntityUID << 8) | EntityType` (24-bit UID + 8-bit type)
- Two shader variants: standard (faces/vertices) and geometry shader (thick-line edges)
- Async readback with PBO support

### HighlightPass (`highlight_pass.hpp`)

Delegates to a pluggable `IHighlightStrategy` for rendering selection/hover outlines.

**Current strategy: OutlineHighlight**
- Stencil-based solid outlines
- Pass 1: Draw entity at original size, write to stencil
- Pass 2: Draw scaled-from-centroid version where stencil != 1
- Separate colors for selection (blue) and hover (cyan)

## Data Flow

### CPU → GPU Pipeline

```
GeometryDocument (CPU)                    MeshDocument (CPU)
    │ getRenderData()                         │ getRenderData()
    ▼                                         ▼
DocumentRenderData ◄──── merge ────── DocumentRenderData
    │ (face/edge/vertex meshes)        (meshElement/meshNode meshes)
    │
    │ RendererCore::uploadMeshData()
    ▼
RenderBatch (GPU)
    ├── faceMeshBuffers      (VAO/VBO/EBO per face)
    ├── edgeMeshBuffers      (VAO/VBO/EBO per edge)
    ├── vertexMeshBuffers    (VAO/VBO per vertex)
    ├── meshElementMeshBuffers (VAO/VBO/EBO per mesh element)
    └── meshNodeMeshBuffers  (VAO/VBO per mesh node)
```

### Vertex Layout (RenderVertex)

| Location | Attribute | Type | Offset |
|----------|-----------|------|--------|
| 0 | Position | vec3 (float) | 0 |
| 1 | Normal | vec3 (float) | 12 |
| 2 | Color | vec4 (float) | 24 |

Total stride: 40 bytes per vertex.

### Entity Identification in RenderableBuffer

Each `RenderableBuffer` carries:
- `m_entityType` / `m_entityUid` — The entity this buffer represents
- `m_owningPartUid` — The Part this entity belongs to (for visibility/selection)
- `m_owningSolidUid` — The Solid this entity belongs to
- `m_owningWireUid` — The Wire(s) this entity belongs to
- `m_centroid` — Bounding box center (used for outline scaling)
- `m_hoverColor` / `m_selectedColor` — Override colors for highlighting

## Shader Programs

| Name | Vertex Shader | Fragment Shader | Purpose |
|------|--------------|-----------------|---------|
| `"mesh"` | Phong vertex (MVP, model, normal matrix, lighting) | Phong fragment (ambient + diffuse + specular) | Main geometry rendering |
| `"outline"` | Scale-from-centroid vertex | Solid color fragment | Stencil-based outlines |
| `"pick"` | MVP-only vertex | Uint32 ID fragment | Face/vertex picking |
| `"pick_line"` | MVP vertex + geometry shader (line thickening) | Uint32 ID fragment | Edge picking |

## Selection & Picking System

### SelectManager (`select_manager.hpp`)

Global singleton managing selection state:
- `PickTypes` bitmask: Vertex, Edge, Wire, Face, Solid, Part, MeshNode, MeshElement
- Exclusivity rules: Mesh types exclude geometry types; Part/Solid/Wire are mutually exclusive
- Thread-safe with mutex protection

### Pick Pipeline (in GLViewportRenderer)

1. On mouse move/click, `processPicking()` is called
2. `renderPicking()` renders all entities to pick FBO with encoded IDs
3. `readPickPixels()` reads a region around cursor
4. `findBestPickHit()` selects best hit by priority and distance
5. Result forwarded to `ViewportService` and `SelectManager`

## Visibility System

Per-part geometry and mesh visibility is controlled through:
- `RenderSceneController::setPartGeometryVisible(partUid, visible)`
- `RenderSceneController::setPartMeshVisible(partUid, visible)`

Both `GeometryPass` and `MeshPass` check visibility before rendering each buffer. The QML sidebar exposes toggle buttons ("G" and "M") per part via `ViewportService`.

## Threading Model

- **Main Thread:** Qt GUI, QML, viewport updates
- **OpenGL Thread:** GLViewportRenderer (created by QQuickFramebufferObject)
- **Worker Thread:** BackendService job execution (geometry/mesh operations)

Thread-safe components:
- EntityId/EntityUID generation (atomic counters)
- SelectManager (mutex-protected)
- RenderSceneController visibility state (mutex-protected)
- RenderData marked for refresh, consumed by render thread during `synchronize()`
