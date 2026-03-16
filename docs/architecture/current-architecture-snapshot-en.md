# OpenGeoLab Architecture Snapshot and Next-Step Plan

## 1. Current Stable Control Paths

After this commit, the repository is no longer best described as a placeholder slice with scattered entry points. It now has a shared request-and-action backbone:

- QML UI -> `ActionRegistry.qml` / `RibbonConfig.qml` / `FeaturePageBase.qml` -> `OpenGeoLabController.runServiceRequest()` or `submitServiceRequest()` -> `CommandRecorder` / `CommandService` -> `ComponentRequestDispatcher` -> `<module>` service -> `<Action>` factory -> domain action
- Embedded Python -> `opengeolab_app.process(request)` -> `OpenGeoLabController` -> the same command / dispatcher / module-action pipeline
- External Python -> `opengeolab.process(request)` or `OpenGeoLabPythonBridge.process(request)` -> `CommandService` -> the same module-action pipeline
- Runtime logs -> `ModuleLogger` / `AppLogger` -> `QmlSpdlogSink` -> `OperationLogService` / `OperationLogModel` -> the Activity Center Events tab
- Request and script exchange -> controller `lastRequest` / `lastResponse` / `lastPythonOutput` -> the Activity Center Command Line tab

The value of the current state is not full CAD capability. The key gain is that QML, embedded Python, external Python, progress feedback, and logging now share one consistent request boundary.

## 2. Current Protocol and Module Responsibilities

### 2.1 Request contract

The repository now uses a stable JSON envelope as the public service boundary:

```json
{
  "module": "geometry",
  "action": "createBox",
  "param": {
    "modelName": "Box_001",
    "origin": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "dimensions": { "x": 120.0, "y": 80.0, "z": 60.0 }
  }
}
```

`libs/core` now formalizes that shape through `ServiceRequest` and `ServiceResponse`. `param` must be a JSON object, and the dispatcher, controller, and Python bridge all validate the same contract at the boundary.

### 2.2 Module responsibilities

| Module | Responsibility |
| --- | --- |
| `apps/OpenGeoLabApp` | Assembles the QML shell, generic app controller, embedded Python runtime, Activity Center, and runtime language switching. |
| `libs/core` | Defines `ServiceRequest`, `ServiceResponse`, `ProgressCallback`, module dispatching, and shared module logging helpers. |
| `libs/command` | Owns module registration bootstrap, request execution, record / replay, and Python script export. |
| `libs/geometry` | Exposes the `geometry` service and registers `createBox`, `createCylinder`, `createSphere`, `createTorus`, and `inspectModel` through action factories. |
| `libs/scene` | Exposes the `scene` service and returns stable `SceneGraph` data through `buildScene`. |
| `libs/render` | Exposes the `render` service and returns stable `RenderFrame` data through `buildFrame`. |
| `libs/selection` | Exposes the `selection` service and resolves `pickEntity` and `boxSelect` by chaining geometry -> scene -> render -> selection evaluation. |
| `python/python_wrapper` | Exposes the external `opengeolab` pybind module while reusing the shared command service instead of defining a separate business protocol. |

## 3. Current Design Conclusions

### 3.1 The controller is now a generic protocol adapter

`OpenGeoLabController` is no longer the place to accumulate domain-specific slots. Its stable role is now:

- QML assembles `{module, action, param}`
- the controller validates, executes, records, and reports progress
- domain semantics live inside module services and concrete actions

That means new user-facing capability should usually start with a new action and request spec, not with another controller method such as `createBox()` or `pickEntity()`.

### 3.2 Module services have converged on an action-factory pattern

`geometry`, `scene`, `render`, and `selection` now share the same service structure:

- each service validates `module` and `action`
- `ComponentRegistration.cpp` registers both the module service and the concrete action factories
- each concrete action owns parameter normalization, progress reporting, response construction, and module-local logging

The important architectural result is not one specific selection workflow. It is that domain capability is now consistently exposed through action components across modules.

### 3.3 The QML shell now has a registry + feature-page structure

`Main.qml` is now much closer to a state-assembly layer than a business-logic container:

- `ActionRegistry.qml` stores action metadata, summaries, workflow kinds, and request specs
- `RibbonConfig.qml` stores the tab / group / action presentation model
- `FeaturePageBase.qml` provides a shared floating workflow surface
- `GeometryCreateFeaturePage.qml` plus `GeometryCreatePageLogic.js` handle validation, derived metrics, and request assembly for complex forms

That establishes a clear direction for future QML work: extend the registry, config, base page, and sidecar logic instead of reintroducing long inline flows inside `Main.qml`.

### 3.4 The Activity Center is now the shared observability surface

This commit also unified request feedback, Python output, and module logs into one UI surface:

- the Events tab shows runtime logs captured by `OperationLogService`
- the Command Line tab shows the latest JSON request / response exchange and Python output
- runtime log emission level and panel-level visibility filtering are intentionally separate
- `submitServiceRequest()` now supports asynchronous execution with shared progress feedback

The Activity Center is therefore no longer decorative chrome. It is the current runtime observability surface for UI, automation, and module execution.

### 3.5 Logging, localization, and Python automation now share the same baseline rules

- `ModuleLogger.hpp` provides shared logger creation plus `OGL_LOG_*` macros that preserve level, module name, source file, line, and thread id
- `HeaderMenuPanel.qml` and `UiSettingsController` colocate theme and language switching in one workspace settings surface
- new QML strings are wrapped in `qsTr()`
- embedded and external Python both converge on `process(request)`, even though one returns Python objects and the other currently returns formatted JSON text

Together these changes show that scriptability, localization, and runtime visibility are now treated as first-class architectural concerns rather than add-ons.

## 4. Current Gaps and Boundaries

The architecture is far clearer than before, but there are still important boundaries:

- `scene`, `render`, and `selection` still return stable placeholder data flows rather than real viewport hosting, GPU lifecycle management, or picking algorithms
- mesh and AI ribbon actions are still mostly UI metadata and are not yet wired into matching module-action services
- command history supports record / replay / export, but not full undo / redo yet
- the synchronous `runServiceRequest()` path still executes on the UI thread, so longer operations should prefer `submitServiceRequest()`
- the public pybind layer is still a generic bridge, not yet a typed high-level Python workflow API

## 5. Current Development Guidance

Compared with the older “UI -> Python bridge -> libs” description, the current direction is much closer to the intended product architecture because:

- QML, embedded Python, and external Python all use the same request envelope
- domain services now expose engineering action names instead of placeholder-only semantics
- the command recorder has become the shared replay and export seam
- the Activity Center can surface both runtime logs and request exchange
- the QML shell is converging on reusable registry / config / page / overlay composition

The main discipline to preserve from here is: define the module action and structured payload first, then expose adapters in QML, Python, and Activity surfaces.

## 6. Recommended Next Steps

### Phase A: replace placeholder actions with real domain backends

Keep the `module + action + param` protocol stable while gradually replacing placeholder geometry, scene, render, and selection behavior with OCC, Gmsh, real rendering, and real picking implementations.

### Phase B: expand module-action coverage

Bring mesh, visibility, camera, selection-set mutation, and other user-visible operations into the same action-factory and command-recording architecture instead of leaving them as UI-only placeholders.

### Phase C: move command history from replay-only to reversible contracts

Build undo and redo on top of the existing record / replay / export boundary.

### Phase D: continue consolidating the QML shell

Keep evolving around `ActionRegistry`, `RibbonConfig`, `FeaturePageBase`, and `OperationLogPanel` so that `Main.qml` does not accumulate business logic again.

### Phase E: keep localization, logging, and packaging aligned

Whenever new QML pages, icons, script surfaces, or module loggers are added, update `qmldir`, the app CMake QML lists, translation TS lists, and logger sink wiring together.
