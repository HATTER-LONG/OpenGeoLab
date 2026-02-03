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

### 3.4 query_entity
Query detailed information for one or more geometric entities.

#### 3.4.1 Single Entity Query
Request:
```json
{
  "action": "query_entity",
  "entity_id": 12345,
  "_meta": { "silent": true }
}
```

Response:
```json
{
  "success": true,
  "action": "query_entity",
  "entity": {
    "id": 12345,
    "uid": 1,
    "type": "Face",
    "name": "Face-1",
    "parent_ids": [10001],
    "child_ids": [],
    "owning_part": 10000,
    "bounding_box": {
      "min": {"x": 0.0, "y": 0.0, "z": 0.0},
      "max": {"x": 10.0, "y": 20.0, "z": 30.0}
    },
    "center": {"x": 5.0, "y": 10.0, "z": 15.0},
    "size": {"x": 10.0, "y": 20.0, "z": 30.0}
  }
}
```

#### 3.4.2 Batch Query
Request:
```json
{
  "action": "query_entity",
  "entity_ids": [12345, 12346, 12347],
  "_meta": { "silent": true }
}
```

Response:
```json
{
  "success": true,
  "action": "query_entity",
  "entities": [
    { "id": 12345, "uid": 1, "type": "Face", ... },
    { "id": 12346, "uid": 2, "type": "Edge", ... },
    { "id": 12347, "uid": 3, "type": "Vertex", ... }
  ]
}
```

### 3.5 highlight
Manage geometric entity highlight state for visual preview/selection.

#### 3.5.1 HighlightState Enum
- `"none"`: No highlight
- `"preview"`: Preview state (typically cyan)
- `"selected"`: Selected state (typically orange)

#### 3.5.2 Set Highlight
Request:
```json
{
  "action": "highlight",
  "operation": "set",
  "entity_id": 12345,
  "state": "preview",
  "_meta": { "silent": true }
}
```

Response:
```json
{
  "success": true,
  "action": "highlight",
  "operation": "set",
  "entity_id": 12345,
  "state": "preview"
}
```

#### 3.5.3 Clear Highlight (Single)
Request:
```json
{
  "action": "highlight",
  "operation": "clear",
  "entity_id": 12345,
  "_meta": { "silent": true }
}
```

#### 3.5.4 Clear All Highlights
Request:
```json
{
  "action": "highlight",
  "operation": "clear_all",
  "_meta": { "silent": true }
}
```

#### 3.5.5 Get Highlight State
Request:
```json
{
  "action": "highlight",
  "operation": "get",
  "entity_id": 12345,
  "_meta": { "silent": true }
}
```

Response:
```json
{
  "success": true,
  "action": "highlight",
  "operation": "get",
  "entity_id": 12345,
  "state": "selected"
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
