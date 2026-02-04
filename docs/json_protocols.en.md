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
- `type`: `box|cylinder|sphere|cone|torus`
- `name` (optional)

Response convention:
- Success: `success: true` with `entity_id/entity_count/name/type`
- Failure: `success: false` with an `error` string (this action typically does not throw)

#### Box (nested format required):
```json
{
  "action": "create",
  "type": "box",
  "name": "Box-1",
  "dimensions": {"x": 10.0, "y": 20.0, "z": 30.0},
  "origin": {"x": 0.0, "y": 0.0, "z": 0.0}
}
```

#### Cylinder:
```json
{
  "action": "create",
  "type": "cylinder",
  "name": "Cylinder-1",
  "radius": 5.0,
  "height": 10.0,
  "x": 0.0, "y": 0.0, "z": 0.0
}
```

#### Sphere:
```json
{
  "action": "create",
  "type": "sphere",
  "name": "Sphere-1",
  "radius": 5.0,
  "x": 0.0, "y": 0.0, "z": 0.0
}
```

#### Torus:
```json
{
  "action": "create",
  "type": "torus",
  "name": "Torus-1",
  "majorRadius": 10.0,
  "minorRadius": 3.0,
  "x": 0.0, "y": 0.0, "z": 0.0
}
```

### 3.4 query_entity
Query entity information by ID.

#### Single entity:
```json
{ "action": "query_entity", "entity_id": 123 }
```

Response:
```json
{
  "success": true,
  "entity": {
    "id": 123,
    "uid": 45,
    "type": "Face",
    "name": "Face_45",
    "owning_part_id": 1,
    "owning_part_name": "Box-1",
    "part_color": "#3498DB",
    "bounding_box": { "min": [...], "max": [...] },
    "center": [5.0, 5.0, 5.0],
    "size": [10.0, 10.0, 10.0]
  }
}
```

#### Batch query:
```json
{ "action": "query_entity", "entity_ids": [123, 124, 125] }
```

---

## 4. Entity Picking

### 4.1 Overview
The GLViewport component supports picking geometry entities by clicking in the 3D viewport. The picking mechanism encodes entity UID and type into the OpenGL color buffer for precise entity selection.

### 4.2 QML Property Interface

GLViewport provides the following properties for controlling pick mode:

| Property | Type | Description |
|----------|------|-------------|
| `pickModeEnabled` | bool | Whether pick mode is enabled |
| `pickEntityType` | string | Entity type filter (Vertex/Edge/Face/Solid/Part) |

### 4.3 QML Signals

| Signal | Parameters | Description |
|--------|------------|-------------|
| `entityPicked` | (entityType: string, entityUid: int) | Emitted when an entity is picked |
| `pickCancelled` | none | Emitted when picking is cancelled (right-click) |

### 4.4 Picking Workflow

1. **Enable Pick Mode**
   - Set `viewport.pickModeEnabled = true`
   - Set `viewport.pickEntityType = "Face"` (optional, filters specific type)

2. **User Interaction**
   - Left-click: Search for matching entities around the click position (5 pixel snap radius)
   - Right-click: Cancel pick mode

3. **Receive Pick Results**
   ```qml
   Connections {
       target: viewport
       function onEntityPicked(entityType, entityUid) {
           console.log("Picked:", entityType, entityUid);
       }
       function onPickCancelled() {
           console.log("Pick cancelled");
       }
   }
   ```

### 4.5 Selector Component Integration

The Selector component (`resources/qml/util/Selector.qml`) provides a complete picking UI:

**Properties:**
| Property | Type | Description |
|----------|------|-------------|
| `visibleEntityTypes` | object | Controls which entity type buttons are displayed |
| `selectedEntities` | array | List of selected entities [{type, uid}, ...] |
| `pickModeActive` | bool | Whether currently in pick mode |
| `selectedType` | string | Currently selected entity type |

**Example: Show only Face type**
```qml
Selector {
    visibleEntityTypes: {
        "Vertex": false,
        "Edge": false,
        "Face": true,
        "Solid": false,
        "Part": false
    }
}
```

---

## 5. ReaderService

### 5.1 Load model
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
