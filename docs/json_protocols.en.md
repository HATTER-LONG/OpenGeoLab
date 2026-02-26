# OpenGeoLab JSON Protocols

This document describes the current JSON-based protocols between QML and C++ (via `BackendService.request()`).

## 1. Conventions

### 1.1 QML Entry Point
- API: `BackendService.request(moduleName, jsonString)`
- `moduleName`: service identifier (`RenderService` / `GeometryService` / `ReaderService` / `MeshService`)
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

#### Toggle X-ray mode
Enable/disable X-ray mode (semi-transparent surfaces to reveal occluded edges). Applies to both GeometryPass and MeshPass.

```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "toggle_xray": true },
  "_meta": { "silent": true }
}
```

#### Cycle mesh display mode
Cycle through: wireframe+nodes -> surface+points -> surface+points+wireframe -> wireframe+nodes... (default: wireframe+nodes)

```json
{
  "action": "ViewPortControl",
  "view_ctrl": { "cycle_mesh_display": true },
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
- `wire/solid/part` are exclusive (and also exclusive with `vertex/edge/face`).
- `wire` mode: clicking an edge auto-resolves to the parent Wire and highlights all edges in the Wire.
- `part` mode: clicking any sub-entity auto-resolves to the parent Part and highlights all entities under that Part.

```json
{
  "action": "SelectControl",
  "select_ctrl": { "types": ["vertex", "edge", "face"] },
  "_meta": { "silent": true }
}
```

Supported pick type strings:
- Geometry domain: `"vertex"`, `"edge"`, `"face"`, `"wire"`, `"solid"`, `"part"`
- Mesh domain: `"mesh_node"`, `"mesh_line"`, `"mesh_triangle"`, `"mesh_quad4"`, `"mesh_tetra4"`, `"mesh_hexa8"`, `"mesh_prism6"`, `"mesh_pyramid5"`

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

Note: `uid` is a numeric `EntityUID` (type-scoped id).

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


### 3.4 query_entity_info
- action: `"query_entity_info"`
- purpose: query detailed information for entities selected in the viewport (Geo Query page).

Request:
```json
{
  "action": "query_entity_info",
  "entities": [
    { "type": "Face", "uid": 123 },
    { "type": "Edge", "uid": 45 }
  ]
}
```

Response (example):
```json
{
  "success": true,
  "total": 2,
  "entities": [
    {
      "type": "Face",
      "uid": 123,
      "id": 1001,
      "name": "",
      "bounding_box": {
        "min": [0.0, 0.0, 0.0],
        "max": [10.0, 10.0, 10.0]
      },
      "related": {
        "parts": [{"id": 1, "uid": 1, "type": "Part"}],
        "solids": [{"id": 2, "uid": 1, "type": "Solid"}],
        "wires": [],
        "faces": [],
        "edges": [],
        "vertices": [],
        "shells": []
      }
    }
  ],
  "not_found": [
    { "type": "Edge", "uid": 45 }
  ]
}
```

Failure example:
```json
{ "success": false, "error": "Missing or invalid 'entities' array" }
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

---

## 5. MeshService

### 5.1 generate_mesh
- action: `"generate_mesh"`
- purpose: generate FEM mesh from selected geometry entities (Part/Solid/Face) using Gmsh backend.

Request:
```json
{
  "action": "generate_mesh",
  "entities": [
    { "type": "Part", "uid": 1 }
  ],
  "elementSize": 1.0,
  "meshDimension": 2,
  "elementType": "triangle"
}
```

Fields:
- `entities`: array of `{uid, type}` entity handles (supports Part/Solid/Face types)
- `elementSize`: target mesh element size (positive number, default `1.0`)
- `meshDimension`: mesh dimension, `2` for surface mesh, `3` for volume mesh (default `2`)
- `elementType`: element type, one of `"triangle"` / `"quad"` / `"auto"` (default `"triangle"`)

Response:
```json
{
  "success": true,
  "nodeCount": 256,
  "elementCount": 480
}
```

Failure example:
```json
{ "success": false, "error": "No valid entities provided for meshing" }
```

### 5.2 query_mesh_info
- action: `"query_mesh_info"`
- purpose: query detailed information for mesh nodes/elements by UID (Mesh Query page).

**MeshLine handling**: MeshLine is now a proper MeshElement object with UIDs generated by `generateMeshElementUID(MeshElementType::Line)` (type-scoped unique). Queries use `findElementByRef()` directly. Results include `relatedElements` (face/volume elements sharing the edge). Node results include `relatedLines` and `relatedElements`. Non-Line element results include `relatedLines` (Line elements forming the element's edges).

Request:
```json
{
  "action": "query_mesh_info",
  "entities": [
    { "type": "Node", "uid": 42 },
    { "type": "Triangle", "uid": 100 },
    { "type": "Line", "uid": 5 }
  ]
}
```

Response (example):
```json
{
  "success": true,
  "total": 3,
  "entities": [
    {
      "type": "Node",
      "uid": 42,
      "position": { "x": 1.0, "y": 2.0, "z": 3.0 },
      "relatedLines": [
        { "uid": 5, "type": "Line" },
        { "uid": 8, "type": "Line" }
      ],
      "relatedElements": [
        { "uid": 100, "type": "Triangle" }
      ]
    },
    {
      "type": "Triangle",
      "uid": 100,
      "elementId": 5,
      "nodeCount": 3,
      "nodeIds": [10, 11, 12],
      "nodes": [
        { "id": 10, "x": 0.0, "y": 0.0, "z": 0.0 },
        { "id": 11, "x": 1.0, "y": 0.0, "z": 0.0 },
        { "id": 12, "x": 0.5, "y": 1.0, "z": 0.0 }
      ],
      "relatedLines": [
        { "uid": 5, "type": "Line" },
        { "uid": 6, "type": "Line" },
        { "uid": 7, "type": "Line" }
      ]
    },
    {
      "type": "Line",
      "uid": 5,
      "elementId": 200,
      "nodeCount": 2,
      "nodeIds": [10, 11],
      "nodes": [
        { "id": 10, "x": 0.0, "y": 0.0, "z": 0.0 },
        { "id": 11, "x": 1.0, "y": 0.0, "z": 0.0 }
      ],
      "relatedElements": [
        { "uid": 100, "type": "Triangle" },
        { "uid": 101, "type": "Triangle" }
      ]
    }
  ],
  "not_found": []
}
```

Failure example:
```json
{ "success": false, "error": "Missing or invalid 'entities' array" }
```
