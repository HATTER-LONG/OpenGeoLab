# OpenGeoLab

OpenGeoLab is a desktop prototype for CAD/CAE workflows. It combines Qt Quick (QML) for UI, OpenGL for viewport rendering, and OpenCASCADE for topology and geometry management, with an architecture that is being extended toward meshing, editing, and AI-assisted analysis.

Chinese documentation is available in README.md. JSON protocol details are documented in docs/json_protocols.en.md.

## Features

- Import BREP and STEP(STP) models.
- Manage vertices, edges, wires, faces, solids, and parts through OCC-based documents.
- Render geometry and mesh overlays with interactive viewport controls.
- Support picking-driven querying, highlighting, and selection.
- Keep extension points for trim/offset, mesh quality inspection, and AI-assisted repair workflows.

## Repository Layout

- include/: public headers and cross-module contracts.
- src/app/: application entry, QML bridge, async backend services, viewport objects.
- src/geometry/: geometry documents, entity relationships, OCC shape building, geometry render-data builders.
- src/mesh/: mesh documents, node/element storage, relation maps, mesh render-data builders.
- src/render/: render snapshots, scene controller, GPU passes, picking and highlight logic.
- src/io/: STEP / BREP import services.
- resources/qml/: QML pages and reusable UI components.
- test/: unit tests and test-specific CMake configuration.

See docs/architecture.en.md for a concise architecture summary.

## Dependencies

- CMake 3.14+
- Qt 6.8+: Core, Gui, Qml, Quick, OpenGL
- OpenCASCADE
- gmsh
- Ninja + MSVC, or another Windows-capable C++20 toolchain
- Optional: HDF5 / HighFive

The project uses CPM to fetch several third-party packages, including Kangaroo, nlohmann/json, Catch2, fmt, and spdlog.

## Build

Windows + Ninja example:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target opengeolab
```

The application binary is generated at build/bin/OpenGeoLabApp.exe.

To enable tests:

```powershell
cmake -S . -B build -G Ninja -DENABLE_TEST=ON
cmake --build build --target OpenGeoLabTests
ctest --test-dir build --output-on-failure
```

## QML ↔ C++ Protocol

- Entry point: BackendService.request(module, JSON.stringify(params))
- Dispatch: each module service routes by params.action inside processRequest
- Success signal: operationFinished(moduleName, actionName, result)
- Error signal: operationFailed(moduleName, actionName, error)

See docs/json_protocols.en.md for protocol details.

## Recent Engineering Improvements

- Added document-level reader/writer locking in geometry and mesh documents.
- Reworked RenderSceneController to publish immutable shared render-data snapshots.
- Fixed BackendService error signal argument ordering and cancellation semantics.
- Upgraded the test target to C++20 and added mesh document / mesh render-data tests.

## Development Notes

- Use include/ headers as the module boundary; avoid cross-module dependencies on src internals.
- Prefer explicit progress reporting and cancellation support for long-running operations.
- Follow the Doxygen style described in doxygen_comment_style.md.
