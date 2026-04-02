# Viewport Actions Design — Camera Control & Pick Area

## Problem

Camera state (`CameraState`) currently lives in the app layer (`GLViewport`), making it
inaccessible to the command system and Python scripting. Box-select can only be triggered
by QML mouse events. This prevents Python scripts from controlling camera views or
performing programmatic region picks.

## Approach

1. **Migrate CameraState to scene layer** — single source of truth, mutex-protected.
2. **New `ViewportState` class** in scene — holds camera, pending pick-area requests, and
   version tracking.
3. **New actions registered in SceneModule** — `fit_to_scene`, `set_view_preset`,
   `set_camera`, `pick_area`.
4. **Adapt app layer** — `GLViewport` and `GLViewportRenderer` read/write camera through
   `ViewportState` instead of local copy.

## 1. ViewportState (scene layer)

### 1.1 CameraState Migration

Move `CameraState` from `src/app/` to `src/libs/scene/`:

```
src/libs/scene/include/opengeolab/scene/camera_state.hpp
src/libs/scene/src/camera_state.cpp
```

The struct is unchanged — `position`, `target`, `up`, `nearPlane`, `farPlane`, plus
`viewMatrix()`, `projMatrix()`, `distance()`, `updateClipping()`, `fitToBoundingBox()`.

Dependencies: `glm`, `BoundingBox3D` (already in scene). Note: current `CameraState` in
app already includes `<opengeolab/scene/bounding_box3d.hpp>`, so moving it to scene
*eliminates* a cross-layer dependency. Export macro changes from `OPENGEOLAB_APP_EXPORT`
to `OPENGEOLAB_SCENE_EXPORT`.

### 1.2 ViewPreset Migration

Move `ViewPreset` enum from `TrackballController` to scene layer:

```cpp
// src/libs/scene/include/opengeolab/scene/view_preset.hpp
namespace OpenGeoLab::Scene {
enum class ViewPreset { Front, Back, Top, Bottom, Left, Right, Isometric };
}
```

### 1.3 PendingPickArea Struct

```cpp
// src/libs/scene/include/opengeolab/scene/viewport_state.hpp
namespace OpenGeoLab::Scene {

enum class PickAreaCoordType { Normalized, Pixel };

struct PendingPickArea {
    float x0{0.0F};
    float y0{0.0F};
    float x1{0.0F};
    float y1{0.0F};
    PickAreaCoordType coordType{PickAreaCoordType::Normalized};
    Core::PickAction action{Core::PickAction::Add};
};
```

### 1.4 ViewportState Class

```cpp
class OPENGEOLAB_SCENE_EXPORT ViewportState final {
public:
    ViewportState();
    ~ViewportState();

    // ── Camera ──
    [[nodiscard]] CameraState camera() const;       // lock → copy → unlock
    void setCamera(const CameraState& state);       // lock → set → unlock, bump version
    [[nodiscard]] uint64_t cameraVersion() const noexcept;

    // ── Camera presets ──
    void fitToBounds(const BoundingBox3D& bounds);
    void setViewPreset(ViewPreset preset);

    // ── Pick area (async, consumed by renderer) ──
    void requestPickArea(const PendingPickArea& request);
    [[nodiscard]] std::optional<PendingPickArea> consumePickArea();

    // ── Signals (emitted outside lock) ──
    Kangaroo::Util::Signal<> cameraChanged;
    Kangaroo::Util::Signal<> pickAreaRequested;

private:
    mutable std::mutex m_mutex;
    CameraState m_camera;
    std::atomic<uint64_t> m_cameraVersion{0};
    std::optional<PendingPickArea> m_pendingPickArea;
};
```

**Thread safety**: All accessors take `m_mutex`. Signals emitted after unlock to avoid
holding lock during callback dispatch.

**fitToBounds()**: Delegates to `CameraState::fitToBoundingBox()`. Emits `cameraChanged`.

**setViewPreset()**: Computes camera position from preset direction and current distance
(logic extracted from `TrackballController::setViewPreset()`). Emits `cameraChanged`.

### 1.5 SceneGraph Integration

`SceneGraph` adds:

```cpp
[[nodiscard]] ViewportState& viewportState();
[[nodiscard]] const ViewportState& viewportState() const;
```

Member: `ViewportState m_viewportState;`

## 2. Camera Actions

All registered in `SceneModule`, using existing action pattern.

### 2.1 `scene.fit_to_scene`

```json
{
  "module": "scene",
  "action": "fit_to_scene",
  "param": {}
}
```

**Implementation**: Calls `SceneGraph::sceneBounds()` to compute AABB of all visible nodes,
then `viewportState().fitToBounds(bounds)`.

**Response**: `{"ok": true}` or error if scene is empty.

### 2.2 `scene.set_view_preset`

```json
{
  "module": "scene",
  "action": "set_view_preset",
  "param": { "preset": "Front" }
}
```

**Accepted presets**: `Front`, `Back`, `Top`, `Bottom`, `Left`, `Right`, `Isometric`.

**Implementation**: Calls `viewportState().setViewPreset(parsed_preset)`.

**Response**: `{"ok": true}` or error for invalid preset name.

### 2.3 `scene.set_camera`

```json
{
  "module": "scene",
  "action": "set_camera",
  "param": {
    "position": [0.0, 0.0, 50.0],
    "target":   [0.0, 0.0, 0.0],
    "up":       [0.0, 1.0, 0.0]
  }
}
```

All three arrays are required (3 floats each).

**Implementation**: Constructs `CameraState`, calls `updateClipping()`, then
`viewportState().setCamera(state)`.

**Response**: `{"ok": true}`.

### 2.4 `scene.pick_area` (async)

```json
{
  "module": "scene",
  "action": "pick_area",
  "param": {
    "x0": 0.2, "y0": 0.2,
    "x1": 0.8, "y1": 0.8,
    "coordType": "normalized",
    "pickAction": "Add"
  }
}
```

**Parameters**:
- `x0, y0, x1, y1`: Rectangle corners.
- `coordType`: `"normalized"` (0.0–1.0, default) or `"pixel"` (Qt item-space pixels).
  - Normalized: (0,0) = top-left, (1,1) = bottom-right.
  - Pixel: matches QML MouseArea coordinate space.
- `pickAction`: `"Add"` (default) or `"Remove"`.

**Implementation**: Stores `PendingPickArea` in `viewportState()`, emits
`pickAreaRequested` → `dataChanged` → viewport `update()` → next render frame processes
the pick.

**Response**: `{"ok": true, "async": true}` — selection results will be in `SelectionState`
after the next render frame. Query with `scene.query_selection`.

**Coordinate conversion** (done by renderer during pick dispatch):
- Normalized → pixel: `x_px = x_norm × viewportWidth`, `y_px = y_norm × viewportHeight`
- Pixel → framebuffer pixel: `x_fb = x_px × devicePixelRatio`
- Normalized → framebuffer pixel: `x_fb = x_norm × viewportWidth × devicePixelRatio`

## 3. App Layer Adaptation

### 3.1 GLViewport Changes

**Remove**: Local `CameraState m_camera` member.

**Camera access**: Read/write through `sceneGraph()->viewportState()`:

```cpp
// Interactive camera manipulation (mouse drag)
auto state = sceneGraph()->viewportState().camera();
m_trackball.update(x, y, state);
sceneGraph()->viewportState().setCamera(state);
```

**TrackballController**: API unchanged (still takes `CameraState&`). Only calling
convention changes to read-modify-writeback pattern.

**fitToScene() / setViewPreset()**: Delegate to `viewportState()` methods instead of
calling `m_trackball` directly.

### 3.2 GLViewportRenderer::synchronize() Changes

**Camera source**: Read from `scene->viewportState().camera()` instead of
`viewport->cameraState()`.

**Pick area consumption**: Check `viewportState().consumePickArea()`:

```cpp
if (auto pickArea = scene->viewportState().consumePickArea()) {
    // Convert coordinates based on coordType
    // Store as pending pick for render() dispatch
}
```

### 3.3 GLViewportRenderer::render() Changes

Process pending pick area using existing `dispatchBoxSelectResults()` logic
(or extracted shared helper). Coordinate conversion:

```cpp
float fx0 = pickArea.x0, fy0 = pickArea.y0;
float fx1 = pickArea.x1, fy1 = pickArea.y1;

if (pickArea.coordType == PickAreaCoordType::Normalized) {
    fx0 *= m_frameState.viewportWidth;
    fy0 *= m_frameState.viewportHeight;
    fx1 *= m_frameState.viewportWidth;
    fy1 *= m_frameState.viewportHeight;
} else {
    // Pixel → framebuffer
    fx0 *= m_frameState.devicePixelRatio;
    fy0 *= m_frameState.devicePixelRatio;
    fx1 *= m_frameState.devicePixelRatio;
    fy1 *= m_frameState.devicePixelRatio;
}

auto results = m_pipeline.pickRect(
    static_cast<int>(fx0), static_cast<int>(fy0),
    static_cast<int>(fx1), static_cast<int>(fy1),
    pickMask());
```

### 3.4 App → Scene Dependency

The `app` target already links `opengeolab_scene`. `CameraState` moving to scene does
not introduce new dependency cycles.

`TrackballController` remains in app; it includes `<opengeolab/scene/camera_state.hpp>`
and `<opengeolab/scene/view_preset.hpp>` (both now in scene).

## 4. SceneModule Signal Connections

`SceneModule` constructor adds connections for `ViewportState` signals → `dataChanged`:

```cpp
auto& vps = m_sceneGraph.viewportState();
m_graphConnections.push_back(vps.cameraChanged.connect(
    [this]() { dataChanged.emit(Core::ModuleDataEvent::ItemModified); }));
m_graphConnections.push_back(vps.pickAreaRequested.connect(
    [this]() { dataChanged.emit(Core::ModuleDataEvent::ItemModified); }));
```

This ensures command-driven camera changes and pick-area requests trigger viewport
re-render through the existing `ModuleDataNotifier → viewport->update()` chain.

## 5. Python Demo Plugin Update

Update `plugins/selection_demo_plugin/__init__.py` to use new viewport actions:

```python
def on_create_and_fit():
    result = process(json.dumps({
        "module": "geometry", "action": "create_box",
        "param": {"name": "DemoBox", ...}
    }))
    if result.get("ok"):
        process('{"module":"scene","action":"fit_to_scene"}')
        process('{"module":"scene","action":"set_view_preset","param":{"preset":"Isometric"}}')

def on_pick_area():
    process(json.dumps({
        "module": "scene", "action": "pick_area",
        "param": {
            "x0": 0.3, "y0": 0.3, "x1": 0.7, "y1": 0.7,
            "coordType": "normalized", "pickAction": "Add"
        }
    }))
    QTimer.singleShot(200, on_query_selection)

def on_query_selection():
    result = process('{"module":"scene","action":"query_selection"}')
    # Display selected entities in plugin UI
```

## 6. Files Changed Summary

### New files (scene library)
- `include/opengeolab/scene/camera_state.hpp` — migrated from app
- `src/camera_state.cpp` — migrated from app
- `include/opengeolab/scene/view_preset.hpp` — extracted from TrackballController
- `include/opengeolab/scene/viewport_state.hpp`
- `src/viewport_state.cpp`
- `include/opengeolab/scene/fit_to_scene_action.hpp`
- `src/fit_to_scene_action.cpp`
- `include/opengeolab/scene/set_view_preset_action.hpp`
- `src/set_view_preset_action.cpp`
- `include/opengeolab/scene/set_camera_action.hpp`
- `src/set_camera_action.cpp`
- `include/opengeolab/scene/pick_area_action.hpp`
- `src/pick_area_action.cpp`

### Deleted files (app)
- `include/opengeolab/app/camera_state.hpp`
- `src/camera_state.cpp`

### Modified files
- `scene/CMakeLists.txt` — add new sources
- `scene/scene_graph.hpp` — add `ViewportState` member + accessor
- `scene/scene_graph.cpp` — add `viewportState()` implementation
- `scene/scene_module.cpp` — register 4 new actions + connect ViewportState signals
- `app/CMakeLists.txt` — remove camera_state sources
- `app/gl_viewport.hpp` — remove `CameraState m_camera`, adapt methods
- `app/gl_viewport.cpp` — read/write camera via `viewportState()`
- `app/gl_viewport_renderer.cpp` — read camera from scene, consume pick area
- `app/trackball_controller.hpp` — change include path, use scene::ViewPreset
- `app/trackball_controller.cpp` — change include path
- `plugins/selection_demo_plugin/__init__.py` — add camera + pick area usage

## 7. Testing Strategy

- **Unit tests**: ViewportState thread safety (concurrent read/write), coordinate conversion
- **Existing tests**: All 28 tests must continue passing (CameraState API unchanged)
- **Integration**: Python demo plugin exercises full flow
  (create → fit → preset → pick_area → query_selection)
