# CLI Auto-Start HTTP Server + Display Mode Action Design

## Overview

Two independent features that share a common motivation: making OpenGeoLab
controllable from external tools (CLI, HTTP, Python scripts).

1. **`--start-http-server` CLI flag** — automatically launch the HTTP Server
   Plugin management panel and start the server on application startup.
2. **`set_display_mode` scene action** — expose `xRayMode` and
   `showTessellation` through the request/response protocol so they can be
   controlled via HTTP, Python, or any action dispatch path.

## Part A — `--start-http-server` CLI Flag

### Motivation

Every development session that uses the HTTP API requires the same manual
steps: open the plugin panel → click Start. A CLI flag eliminates this
friction.

### Design

**Entry point (`main.cpp`):**

- Add `QCommandLineParser` with a `--start-http-server` boolean option.
- If set, schedule a deferred `invoke_ui` request using
  `QTimer::singleShot(0, ...)` so it fires after the Qt event loop and QML
  engine are fully initialized.
- The request includes `"autoStart": true` in `param`.

**GIL safety:** The timer fires after the `pybind11::gil_scoped_release` on
the stack. `executeOnMainThread` → `EmbeddedPythonRuntime::process()`
acquires the GIL internally. PySide6 widget creation happens on the main
thread. No GIL issues.

**Runtime (`opengeolab_runtime.py`):**

- `_launch_plugin_ui` currently calls `mod.launch_ui()` with no arguments.
- Use `inspect.signature` to check whether `launch_ui` accepts a parameter.
  If yes, forward the full `param` dict. If no, call without arguments.
- This is backward-compatible with plugins that have zero-parameter
  `launch_ui()`.

**Plugin (`http_server_plugin/__init__.py`):**

- Change `launch_ui()` signature to accept an optional `param: dict | None`.
- After successfully creating the QML engine, check
  `param.get("autoStart", False)`. If true, call `backend.start()`.

### Behavior

```
opengeolab_app.exe --start-http-server
```

1. Application starts normally (OpenGL, QML, modules).
2. Once the event loop begins, the deferred timer fires.
3. `invoke_ui` creates the HTTP Server management panel (visible window).
4. `autoStart` triggers `backend.start()` — server listening on
   `127.0.0.1:8080`.
5. The management panel shows green status and logs incoming requests.

## Part B — `set_display_mode` Scene Action

### Motivation

`xRayMode` and `showTessellation` are currently app-layer properties on
`GLViewport`. They cannot be controlled via the action protocol (HTTP,
Python, etc.). Moving them to `ViewportState` (scene layer) and exposing a
`set_display_mode` action unifies control.

### State Ownership Change

**Before:** `GLViewport` owns `m_xRayMode` / `m_showTessellation`.
QML buttons toggle them. Renderer reads from GLViewport during
`synchronize()`.

**After:** `ViewportState` is the single source of truth (mutex-protected).
GLViewport delegates get/set to ViewportState and syncs back via a
`displayModeChanged` signal.

### ViewportState Changes

Add to `ViewportState`:

```cpp
[[nodiscard]] bool xRayMode() const;
void setXRayMode(bool enabled);
[[nodiscard]] bool showTessellation() const;
void setShowTessellation(bool enabled);

Kangaroo::Util::Signal<> displayModeChanged;
```

Private members: `bool m_xRayMode{false}`, `bool m_showTessellation{false}`.
Both getters/setters acquire `m_mutex`. Setters short-circuit on no-change
and emit `displayModeChanged` after releasing the lock.

### SetDisplayModeAction

```
Module: scene
Action: set_display_mode
```

**Params (all optional):**

| Name | Type | Description |
|------|------|-------------|
| `xRayMode` | boolean | Enable semi-transparent rendering |
| `showTessellation` | boolean | Overlay tessellation edges and vertices |

Omitted fields retain their current value.

**Returns:**

| Name | Type | Description |
|------|------|-------------|
| `ok` | boolean | Always true on success |
| `action` | string | `"set_display_mode"` |
| `xRayMode` | boolean | Current state after applying |
| `showTessellation` | boolean | Current state after applying |

**Example request:**

```json
{
  "module": "scene",
  "action": "set_display_mode",
  "param": { "xRayMode": true }
}
```

**Example response:**

```json
{
  "ok": true,
  "action": "set_display_mode",
  "xRayMode": true,
  "showTessellation": false
}
```

### GLViewport Wiring

1. **`setSceneGraph()`:** Connect `displayModeChanged` → `QMetaObject::invokeMethod`
   on the main thread to sync `m_xRayMode` / `m_showTessellation` and emit
   Q_PROPERTY change signals. Store connection as `ScopedConnection`.

2. **`setXRayMode()` / `setShowTessellation()`:** If `m_sceneGraph != nullptr`,
   also call `viewportState().setXRayMode(enabled)` (etc.) to propagate to
   the authoritative state.

3. **`GLViewportRenderer::synchronize()`:** Read from `ViewportState` when
   `sceneGraph` is available, fallback to `GLViewport` otherwise.

### Signal Chain (action-driven change)

```
SetDisplayModeAction::execute()
  → ViewportState::setXRayMode(true)
    → displayModeChanged.emit()
      → SceneModule → dataChanged(ViewportChanged)
        → ModuleDataNotifier → viewportRefreshNeeded (main thread)
          → viewport->update() → re-render
      → GLViewport callback (QMetaObject::invokeMethod, main thread)
        → m_xRayMode = true, Q_EMIT xRayModeChanged()
          → QML toolbar button updates
```

### Signal Chain (QML toggle)

```
QML ViewportToolbar button click
  → GLViewport::setXRayMode(true)
    → m_xRayMode = true
    → viewportState().setXRayMode(true) → displayModeChanged.emit()
    → Q_EMIT xRayModeChanged()
    → update()
```

The displayModeChanged callback sees no change (m_xRayMode already
matches) and does nothing. No feedback loop.

## Files Changed

| File | Part | Change |
|------|------|--------|
| `src/app/src/main.cpp` | A | QCommandLineParser + QTimer deferred action |
| `src/app/resource/python/opengeolab_runtime.py` | A | Forward param to launch_ui |
| `plugins/http_server_plugin/__init__.py` | A | Accept autoStart param |
| `src/libs/scene/include/opengeolab/scene/viewport_state.hpp` | B | Display mode API + signal |
| `src/libs/scene/src/viewport_state.cpp` | B | Implementation |
| `src/libs/scene/include/opengeolab/scene/set_display_mode_action.hpp` | B | Action header |
| `src/libs/scene/src/set_display_mode_action.cpp` | B | Action implementation |
| `src/libs/scene/src/scene_module.cpp` | B | Register action + connect signal |
| `src/libs/scene/CMakeLists.txt` | B | Add new files |
| `src/app/include/opengeolab/app/gl_viewport.hpp` | B | ScopedConnection member |
| `src/app/src/gl_viewport.cpp` | B | Delegate to ViewportState |
| `src/app/src/gl_viewport_renderer.cpp` | B | Read from ViewportState |
| `src/libs/scene/test/viewport_state_test.cpp` | B | Display mode tests |
| `src/libs/scene/test/viewport_actions_test.cpp` | B | SetDisplayModeAction tests |
