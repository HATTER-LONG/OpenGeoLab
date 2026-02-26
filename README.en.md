# OpenGeoLab

OpenGeoLab is a Qt Quick (QML) + OpenGL prototype for CAD-like geometry visualization and editing:
- Import BREP / STEP (STP) model files
- Manage topology and geometry via OpenCASCADE (OCC)
- Render with orthographic projection, opaque by default (premultiplied alpha ensures full opacity during Qt Quick scene-graph compositing), and X-ray mode (semi-transparent surfaces to reveal occluded edges, applies to both geometry and mesh; when off, polygon offset ensures back-face wireframes are properly occluded), with basic viewport controls (orbit/pan/zoom, view presets, fit)
- Multi-level entity picking (Vertex/Edge/Face/Wire/Part) with automatic child-to-parent resolution for Wire and Part modes; highlighting accurately matches the current pick type (e.g. Edge mode only highlights the selected edge; Wire mode does not incorrectly highlight faces); Wire highlight includes all constituent edges (including shared edges), displayed as a complete closed loop
- Mesh entity picking (Node/Line/Element) with hover and selection highlighting via GPU shader-based pickId comparison; hovered/selected mesh elements display highlight overlay even in wireframe-only mode (highlight-only surface pass discards non-highlighted fragments); MeshLine is stored as a proper MeshElement object using `generateMeshElementUID(Line)` for type-scoped unique IDs, automatically extracted and deduplicated from 2D/3D element edges during mesh generation
- Mesh supports bidirectional node-line-element relation queries, with query results including related entity references
- Mesh rendering supports three cycled display modes: wireframe+nodes, surface+points, and surface+points+wireframe (default: wireframe+nodes)
- Mesh surface color inherits from the parent Part color (darkened), clearly distinguished from geometry face colors
- Wire picking with shared edges automatically prioritizes the wire belonging to the face under the cursor (disambiguated via face context from the pick region)
- Planned: interactive editing (e.g., trim/offset), mesh quality diagnostics, and AI-assisted repair

## Repository Layout
- `include/`: public headers / cross-module interfaces (modules should depend on each other via `include/` only)
- `src/`: implementation
  - `src/app/`: application entry, QML singletons (BackendService), OpenGL viewport (GLViewport)
  - `src/geometry/`: geometry document/entities, OCC build and render-data extraction
    - Entity identity: internally uses `EntityKey = (EntityId + EntityUID + EntityType)` as a comparable/hashable handle
  - `src/mesh/`: mesh document/node/element management, FEM data storage
  - `src/io/`: model import services (STEP/BREP)
  - `src/render/`: render data types, SceneRenderer, RenderSceneController, GPU picking (PickPass)
- `resources/qml/`: QML UI
- `test/`: tests (disabled by default)

## Dependencies
- CMake >= 3.14
- Qt 6.8 (Core/Gui/Qml/Quick/OpenGL)
- OpenCASCADE (pre-installed; discovered by CMake via `find_package(OpenCASCADE REQUIRED)`)
- HDF5 (HighFive expects system HDF5)
- Ninja + MSVC (recommended on Windows)

Some dependencies are fetched via CPM (Kangaroo, nlohmann/json, HighFive, ...).

## Build (Windows example)

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is generated at: `build/bin/OpenGeoLabApp.exe`.

## JSON Protocols (QML ↔ C++)
- Entry point: `BackendService.request(moduleName, JSON.stringify(params))`
- Dispatch: `*Service::processRequest()` routes by `params.action`

Signals:
- `operationFinished(moduleName, actionName, result)`
- `operationFailed(moduleName, actionName, error)`

See: `docs/json_protocols.en.md`.

Recent additions:
- The Geo Query page calls `GeometryService` action `query_entity_info` to query detailed info for the currently selected entities (type+uid) and renders the results in a list.
- Mesh Query page supports Node/Line/Element pick queries via GPU PickPass with separate GL_POINTS/GL_LINES/GL_TRIANGLES draws to an offscreen FBO for pixel-level picking.
- X-ray mode now applies to both GeometryPass and MeshPass with premultiplied-alpha blended semi-transparent surfaces (`GL_ONE, GL_ONE_MINUS_SRC_ALPHA`) for correct Qt Quick compositing; when off, `GL_POLYGON_OFFSET_FILL` pushes surfaces back so visible-face wireframes pass the depth test while back-face wireframes are properly occluded.
- Full GL state reset before each frame (glColorMask/glDepthMask/glBlendFunc etc.) prevents Qt scene-graph leftover state from causing semi-transparency.
- Orthographic projection uses symmetric near/far clipping planes (negative near plane) to prevent model clipping on zoom.
- Wire highlight uses a wire-to-edges reverse map; hover/selection highlights all edges that compose the wire (including shared edges) as a complete closed loop.
- Mesh wireframe MeshLine refactored to proper MeshElement objects using `generateMeshElementUID(MeshElementType::Line)` for type-scoped unique IDs; `buildEdgeElements()` automatically extracts and deduplicates edges from 2D/3D elements, creating Line elements and building node-line-element relation maps.
- MeshLine picking now uses the Line element's UID directly as pickId, no longer using composite node-pair encoding; query results include `relatedElements`/`relatedLines` relation references.
- PickPass::renderToFbo refactored to use GeometryPickInput/MeshPickInput parameter structs, reducing parameter count.
- Mesh display mode cycles through three states: wireframe+nodes -> surface+points -> surface+points+wireframe (default: wireframe+nodes).
- Mesh surface color is now unified to the parent Part color (darkened 35%+offset), clearly distinguished from geometry face colors.
- Added mesh hover/selection highlighting via shader-based pickId uniform comparison.
- Wire picking with shared edges now uses face context disambiguation: when an edge belongs to multiple wires, the wire on the face under the cursor is preferred (pick region reads both Edge and Face, resolving Wire by Face ownership).
- X-ray toggle button now provides visual state indication (pressed/active state highlight).
- MeshLine queries now use `MeshDocument::findElementByRef()` directly, returning related elements (face/volume elements sharing the edge).

## Next Steps (high-level)
- Selection & picking (vertex/edge/face/part), highlight, and transform/edit operations (trim/offset, etc.)
- Meshing: surface tessellation, quality metrics, smoothing/repair
- Rendering: consistent lighting (potential PBR/IBL), selection outline, debugging overlays
- IO: richer import/export metadata and error reporting
- AI: mesh diagnosis and guided repair workflows
