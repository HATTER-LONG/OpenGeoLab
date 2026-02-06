# OpenGeoLab JSON Protocols

This document describes the current JSON-based protocols between QML and C++ (via `BackendService.request()`).

## 1. Conventions

### 1.1 QML Entry Point
- API: `BackendService.request(moduleName, jsonString)`
- `moduleName`: service identifier (`RenderService` / `GeometryService` / `ReaderService`)
- `jsonString`: JSON string (recommended to always pass an object)

### 1.2 `action`
- Most services use `params.action` to route requests.
- `BackendService` extracts `actionName` from `params.action` and returns it via:
  - `operationFinished(moduleName, actionName, result)`
  - `operationFailed(moduleName, actionName, error)`

### 1.3 `_meta`
- `_meta.silent: true|false`
  - `true`: synchronous execution (no progress overlay; suitable for frequent UI actions)
  - `false` (default): async worker thread with progress/cancel support

---

## 2. RenderService

### 2.1 ViewPortControl
- action: `"ViewPortControl"`

#### Refresh
```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "refresh": true },
  "_meta": { "silent": true }
}
```

#### Fit to scene
```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "fit": true },
  "_meta": { "silent": true }
}
```

#### View preset
`view_ctrl.view` enum:
- 0: Front
- 1: Back
- 2: Left
- 3: Right
- 4: Top
- 5: Bottom

```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "view": 0 },
  "_meta": { "silent": true }
}
```

### 2.2 SelectControl
- action: `"SelectControl"`
- purpose: control viewport picking mode (enable + pick types) and query/clear current selections.

#### Enable/disable picking
```json
{
  "action": "SelectControl",
  "select_ctrl": { "enabled": true },
  "_meta": { "silent": true }
}
```

#### Set pick types
Notes:
- `vertex/edge/face` can be combined.
- `solid/part` are exclusive (and also exclusive with `vertex/edge/face`).

```json
{
  "action": "SelectControl",
  "select_ctrl": { "types": ["vertex", "edge", "face"] },
  "_meta": { "silent": true }
}
```

#### Get selections
```json
{
  "action": "SelectControl",
  "select_ctrl": { "get": true },
  "_meta": { "silent": true }
}
```

Example response:
```json
{
  "status": "success",
  "action": "SelectControl",
  "pick_enabled": true,
  "pick_types": 7,
  "selections": [
    { "type": "Face", "uid": 123 }
  ]
}
```

#### Clear selections
```json
{
  "action": "SelectControl",
  "select_ctrl": { "clear": true },
  "_meta": { "silent": true }
}
```

---

## 3. GeometryService

### 3.1 new_model
```json
{ "action": "new_model" }
```

### 3.2 get_part_list
```json
{ "action": "get_part_list", "_meta": { "silent": true } }
```

### 3.3 create
Common fields:
- `type`: `box|cylinder|sphere|cone`
- `name` (optional)

Response convention:
- Success: `success: true` with `entity_id/entity_count/name/type`
- Failure: `success: false` with an `error` string (this action typically does not throw)

Box (nested format required):
```json
{
  "action": "create",
  "type": "box",
  "name": "Box-1",
  "dimensions": {"x": 10.0, "y": 20.0, "z": 30.0},
  "origin": {"x": 0.0, "y": 0.0, "z": 0.0}
}
```

---

## 4. ReaderService

### 4.1 Load model
- action: `"load_model"`
- fields:
  - `file_path`: string

Request:
```json
{ "action": "load_model", "file_path": "C:/models/demo.step" }
```

Success response:
```json
{
  "success": true,
  "action": "load_model",
  "file_path": "C:/models/demo.step",
  "reader": "StepReader",
  "entity_count": 123
}
```

Failure:
- thrown as an exception and reported via `operationFailed`.

Note: ReaderService validates `action == "load_model"`; other actions are rejected.
