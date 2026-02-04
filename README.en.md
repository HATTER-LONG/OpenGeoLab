# OpenGeoLab

OpenGeoLab is a Qt Quick (QML) + OpenGL prototype for CAD-like geometry visualization and editing:
- Import BREP / STEP (STP) model files
- Manage topology and geometry via OpenCASCADE (OCC)
- Render with OpenGL and provide basic viewport controls (orbit/pan/zoom, view presets, fit)
- Entity picking (vertex/edge/face/solid/part) with snap-to-entity and type filtering
- Planned: meshing and AI-assisted mesh quality diagnostics & repair

## Main Features

### Geometry Display and Interaction
- STEP/BREP model import and display
- Basic geometry creation (box, cylinder, sphere, torus, etc.)
- Viewport controls: rotate (Ctrl+LMB), pan (Shift+LMB/MMB), zoom (RMB/Ctrl+scroll)
- View presets: front/back/left/right/top/bottom, fit-all

### Entity Picking
- Pick vertices, edges, faces, solids, and parts
- Snap-to-entity: automatically find the nearest entity around click position
- Type filtering: restrict picking to specific entity types
- Selection management: multi-select, remove, clear operations

### Entity Query
- Query entity details (type, name, parent part, bounding box, etc.)
- Single or batch entity query support

## Repository Layout
- `include/`: public headers / cross-module interfaces (modules should depend on each other via `include/` only)
- `src/`: implementation
  - `src/app/`: application entry, QML singletons (BackendService), OpenGL viewport (GLViewport)
  - `src/geometry/`: geometry document/entities, OCC build and render-data extraction
  - `src/io/`: model import services (STEP/BREP)
  - `src/render/`: render data types, SceneRenderer, RenderSceneController
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

## Next Steps (high-level)
- Transform/edit operations (trim/offset, etc.)
- Meshing: surface tessellation, quality metrics, smoothing/repair
- Rendering: consistent lighting (potential PBR/IBL), selection outline, debugging overlays
- IO: richer import/export metadata and error reporting
- AI: mesh diagnosis and guided repair workflows
