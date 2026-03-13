# OpenGeoLab Architecture Snapshot and Next-Step Plan

## 1. Current Project State

The repository has already been reorganized into a modular scaffold:

- apps/OpenGeoLabApp
- libs/core
- libs/geometry
- libs/mesh
- libs/scene
- libs/render
- libs/selection
- libs/command
- python/python_wrapper

The first fully wired vertical slice is now:

QML UI -> app controller -> Python bridge -> Kangaroo ComponentFactory -> geometry lib

Its main value is not geometry capability yet. Its value is that the cross-layer request path, module boundary rules, and scriptable entrypoint are now stable for every future module.

## 2. Current Module Responsibilities

### 2.1 apps/OpenGeoLabApp

- Hosts the QML UI and app-level controller
- Accepts module + JSON requests from QML
- Does not depend on geometry-specific service interfaces

### 2.2 libs/core

- Defines IService and ServiceResponse
- Provides ComponentRequestDispatcher
- Resolves services through Kangaroo ComponentFactory by module name

### 2.3 libs/geometry

- Currently provides PlaceholderGeometryModel
- Registers the geometry service through registerGeometryComponents
- Will later be replaced by OCC-backed GeometryModel, import, topology, and cleanup capabilities

### 2.4 python/python_wrapper

- Provides OpenGeoLabPythonBridge
- Is shared by the app layer and the pybind11 module
- Centralizes the high-level call(module, params) contract

## 3. Architecture Assessment for QML + OpenGL + Mouse Interaction + Python Script Recording

Conclusion: the current direction is sound, but scene, render, selection, and command still need real implementations before the stack can support orbit, picking, box selection, and script recording.

### 3.1 Why the current direction is correct

- The UI is not directly coupled to the geometry kernel.
- The Python bridge already acts as a high-level automation entrypoint, which is the right place for script replay and LLM-generated workflows.
- Kangaroo ComponentFactory provides a consistent module-string routing model across geometry, mesh, scene, selection, and command.
- The render layer is explicitly kept separate from geometry and mesh kernels, which is the correct prerequisite for OpenGL view ownership and picking.

### 3.2 What is still missing

- The render module does not yet own a real OpenGL viewport host or RenderData cache layer.
- The scene module is not yet the single source of truth for visible objects and selection state.
- The selection module does not yet isolate ray picking, ID-buffer picking, and rectangle selection from raw UI events.
- The command module does not yet own user-visible operations, so undo/redo and Python recording are not available yet.

### 3.3 Recommended interaction layering

Recommended split of responsibilities:

1. QML layer
   - Layout, tool state, and event forwarding only
   - No geometry, picking, or rendering algorithms

2. app / InteractionController
   - Normalizes mouse press, move, release, and wheel events
   - Converts them into semantic requests such as orbit camera, pick face, or box select

3. scene module
   - Owns visible nodes, visibility state, active object state, and selection set
   - Acts as the semantic layer shared by rendering and selection

4. render module
   - Consumes RenderData exported from scene
   - Owns GPU buffers, camera state, render passes, and highlight rendering
   - Must not directly consume OCC or Gmsh entities

5. selection module
   - Implements ray casting, GPU picking, and rectangle selection
   - Returns entity IDs or scene node IDs
   - Does not directly mutate the UI

6. command module
   - Owns user-visible operations as commands
   - Records high-level actions for undo/redo and Python script generation

### 3.4 Key principle for Python script recording

Script recording should never store raw mouse motion as the main artifact.

Correct granularity:

- Camera orbit -> set_camera_pose
- Face picking -> select_face or select_entities
- Box selection -> box_select(screen_rect, filter)
- Geometry cleanup -> geometry.removeSmallFaces(params)
- Mesh generation -> mesh.generateSurface(params)

In other words, mouse input should trigger commands, and commands should generate Python operations. The script layer must capture intent, not incidental UI coordinates.

## 4. Current CMake Assessment

The current CMake layout is significantly better than the original monolithic setup:

- The root CMake file owns dependencies and global options
- Each module owns its own CMakeLists.txt
- The geometry smoke test now lives in libs/geometry/tests
- First-party libraries can switch between static and shared builds through OPENGEOLAB_BUILD_SHARED_LIBS
- core and geometry now use generated export headers for Windows/MSVC shared-library compatibility
- Basic install/export rules are now present for future reusable-library packaging

Rules that should remain in place:

- Every public library module must own an export header
- Unit tests must stay inside their owning module
- Only future cross-module integration scenarios should use a top-level tests directory

## 5. Recommended Next Implementation Order

### Phase A: add compilable placeholder implementations for scene, render, and selection

Goal: establish the minimal 3D interaction data flow.

### Phase B: introduce a real OpenGL viewport host

Goal: make QML capable of driving orbit, pan, zoom, and redraw.

### Phase C: let the command system own user-visible operations

Goal: create a single path for undo/redo and Python script recording.

### Phase D: replace the geometry placeholder with a real OCC-backed model path

Goal: move from a demonstration slice to an actual editable geometry module.