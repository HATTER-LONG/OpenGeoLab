# OpenGeoLab Architecture Snapshot and Next-Step Plan

## 1. Current Vertical Slice

The repository now has one stable placeholder slice that validates both interactive and scripted control paths:

- QML UI -> app controller -> command recorder -> command service -> libs
- Embedded Python runtime -> opengeolab_app API -> app controller -> command recorder -> command service -> libs
- External Python module -> opengeolab.OpenGeoLabPythonBridge -> command service -> libs

The value of the slice is architectural validation, not CAD capability. Geometry, scene, render, and selection are still placeholder implementations, but the user-visible control boundaries are now aligned with the intended product direction.

## 2. Current Module Responsibilities

| Module | Responsibility |
| --- | --- |
| apps/OpenGeoLabApp | Hosts the QML shell, the app controller, and the embedded Python runtime. |
| libs/core | Defines IService, ServiceResponse, and dispatcher-based service lookup. |
| libs/command | Owns command execution, recording, replay, and Python export helpers. |
| libs/geometry | Provides placeholder geometry model creation and service registration. |
| libs/scene | Builds stable placeholder scene-graph data from geometry results. |
| libs/render | Builds placeholder render-frame data without depending on kernel internals. |
| libs/selection | Resolves placeholder pick results from scene and render data. |
| python/python_wrapper | Exposes a thin Python-facing bridge that reuses the shared command service. |

## 3. Application and Automation Design

### 3.1 Command ownership

User-visible state changes now converge on the command layer. This is the correct seam for:

- replayable workflows
- future undo and redo support
- Python export
- later LLM-generated orchestration

The app controller no longer needs semantic ownership of python_wrapper. Python is an adapter over the same command path, not the runtime backbone of the UI.

### 3.2 Embedded Python

The application now embeds a Python interpreter and exposes a built-in module named opengeolab_app. Its purpose is to let a running application mutate state through stable high-level APIs instead of ad hoc UI hooks.

Current embedded API shape:

- opengeolab_app.run_command(module_name, params)
- opengeolab_app.replay_commands()
- opengeolab_app.clear_commands()
- opengeolab_app.get_state()

This keeps runtime scripting aligned with the same command and state model used by QML.

### 3.3 Recording granularity

Recorded scripts should capture engineering intent, not raw UI noise.

Preferred granularity:

- selection.pickEntity(...)
- scene.setVisibility(...)
- geometry.cleanup(...)
- mesh.generateSurface(...)
- camera.setPose(...)

Raw mouse motion, transient hover, or per-frame camera deltas should not become the primary replay artifact.

## 4. Current Gaps

The placeholder slice now proves the command and automation seams, but important product work remains:

- render still lacks a real OpenGL viewport host and GPU resource lifecycle
- scene is not yet the full source of truth for visibility, activation, and interaction state
- selection is still placeholder logic rather than ray cast, ID-buffer, or rectangle selection
- command has record and replay, but not undo or redo yet
- embedded Python currently executes on the app thread and should remain a high-level automation tool, not a long-running compute path

## 5. Review Notes and Best Practices

The current direction matches the repository goals better than the previous UI -> Python bridge -> libs path because:

- app-to-domain traffic now uses a command boundary
- Python automation reuses that boundary instead of bypassing it
- generated Python remains readable and replay-oriented
- QML stays presentation-focused and thin

The main discipline to preserve from here is: add new user-facing operations to command first, then expose them to QML and Python as adapters.

## 6. Recommended Next Steps

### Phase A: real viewport host

Introduce a true OpenGL-backed viewport layer so scene and selection can stop relying on placeholder frame data.

### Phase B: command semantics expansion

Add explicit command types for visibility, camera, selection-set mutation, geometry cleanup, and future mesh operations.

### Phase C: undo and redo

Move from replay-only history to full reversible command contracts.

### Phase D: real geometry and meshing backends

Replace placeholder geometry, scene, render, and selection implementations with OCC and Gmsh-backed data flows while keeping the same command and automation boundaries.