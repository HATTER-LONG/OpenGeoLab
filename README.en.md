# OpenGeoLab

OpenGeoLab is a Qt Quick (QML) + OpenGL prototype for CAD-like geometry visualization and editing:
- Import BREP / STEP (STP) model files
- Manage topology and geometry via OpenCASCADE (OCC)
- Render with OpenGL and provide basic viewport controls (orbit/pan/zoom, view presets, fit)
- Planned: interactive editing (e.g., trim/offset), meshing, and AI-assisted mesh quality diagnostics & repair

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
- ~~Selection & picking (vertex/edge/face/part), highlight~~ ✅ (EntitySelector component & QueryPage implemented), and transform/edit operations (trim/offset, etc.)
- Meshing: surface tessellation, quality metrics, smoothing/repair
- Rendering: consistent lighting (potential PBR/IBL), ~~selection outline~~ ✅ (HighlightState-based rendering), debugging overlays
- IO: richer import/export metadata and error reporting
- AI: mesh diagnosis and guided repair workflows

## Features

### Geometry Selection & Query
- **Select Control** (EntitySelector.qml): Reusable entity picking component
  - Selection mode filtering: Vertex, Edge, Face, Solid, Part
  - Hover preview: Temporary highlight (cyan) on mouse hover
  - Click selection: Select entities with single click (orange highlight)
  - Box selection: Drag to select multiple entities at once
  - Right-click clear: Clear all selections
  - Multi-select and max selection limit support
- **Query Interface**: Query page in Geometry tab
  - Selection mode toggle: None / Vertex / Edge / Face / Solid / Part
  - Selected entity list: Display type and ID of selected entities
  - Entity detail query: Bounding box, center point, size, parent/child relationships
