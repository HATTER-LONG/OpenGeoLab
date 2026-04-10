# 3D Model & Camera State Export for LLM Understanding

## Problem

The AI chat plugin can access the 3D scene through `capture_viewport`, but the
returned metadata is limited to bounding-box-level information.  This prevents
the LLM from:

1. **Understanding model topology** — it cannot identify faces, edges, holes,
   or geometric features, so it cannot give meaningful modeling advice.
2. **Controlling the camera accurately** — it must guess raw position/target/up
   vectors, producing random or useless viewpoints.
3. **Selecting entities precisely** — without knowing which face or edge
   corresponds to a feature, it cannot call `scene.select` with the right IDs.

## Proposed Approach

Combine **new C++ Actions** for geometry queries and semantic camera control
with an **enhanced `capture_viewport`** that optionally includes topology data.
All new actions follow the existing `module.action` JSON protocol and are
discovered automatically by the AI chat's schema system — no plugin changes
needed.

## Architecture

```
geometry module (depends on ShapeStore + OCC BRep)
├── DescribeTopologyAction   — shape topology overview
├── QueryEntityInfoAction    — single face/edge/vertex detail

scene module (depends on ViewportState + ShapeStore)
├── LookAtEntityAction       — point camera at an entity
├── BestViewForEntityAction  — auto-compute optimal view angle + zoom
│
├── CaptureViewportAction    — [enhanced] new includeTopology parameter
```

Data flow:

```
LLM → tool_call → pywrapper.process() → CommandDispatcher
  → GeometryModule → ShapeStore → OCC BRep API  (topology extraction)
  → SceneModule    → ViewportState → CameraState (camera computation)
```

## Geometry Topology Query Actions

### `geometry.describe_topology`

Returns a structured overview of a shape's topology: face/edge/vertex counts
and a per-entity summary with type, coordinates, and dimensions.

**Request:**

```json
{
  "module": "geometry",
  "action": "describe_topology",
  "param": { "shapeId": 1 }
}
```

**Response:**

```json
{
  "ok": true,
  "shapeId": 1,
  "shapeName": "Box_1",
  "boundingBox": { "min": [0, 0, 0], "max": [10, 10, 10] },
  "counts": { "faces": 7, "edges": 18, "vertices": 16 },
  "faces": [
    {
      "id": 0,
      "surfaceType": "plane",
      "center": [5, 5, 0],
      "normal": [0, 0, -1],
      "area": 100.0
    },
    {
      "id": 1,
      "surfaceType": "plane",
      "center": [5, 5, 10],
      "normal": [0, 0, 1],
      "area": 87.4
    },
    {
      "id": 2,
      "surfaceType": "cylinder",
      "center": [5, 5, 10],
      "axis": [0, 0, 1],
      "radius": 2.0,
      "area": 12.6
    }
  ],
  "edges": [
    {
      "id": 0,
      "curveType": "line",
      "start": [0, 0, 0],
      "end": [10, 0, 0],
      "length": 10.0
    },
    {
      "id": 5,
      "curveType": "circle",
      "center": [5, 5, 10],
      "radius": 2.0,
      "length": 12.57
    }
  ]
}
```

**Surface types** (from OCC `GeomAbs_SurfaceType`): `plane`, `cylinder`,
`cone`, `sphere`, `torus`, `bspline`, `other`.

**Curve types** (from OCC `GeomAbs_CurveType`): `line`, `circle`, `ellipse`,
`parabola`, `hyperbola`, `bspline`, `other`.

### `geometry.query_entity_info`

Returns detailed information about a single face, edge, or vertex.

**Request:**

```json
{
  "module": "geometry",
  "action": "query_entity_info",
  "param": {
    "shapeId": 1,
    "entityType": "face",
    "localId": 2
  }
}
```

**Response (face example):**

```json
{
  "ok": true,
  "shapeId": 1,
  "entityType": "face",
  "localId": 2,
  "surfaceType": "cylinder",
  "center": [5, 5, 10],
  "axis": [0, 0, 1],
  "radius": 2.0,
  "area": 12.6,
  "boundingBox": { "min": [3, 3, 0], "max": [7, 7, 10] },
  "adjacentEdges": [4, 5, 6, 7],
  "adjacentFaces": [0, 1]
}
```

**Response (edge example):**

```json
{
  "ok": true,
  "shapeId": 1,
  "entityType": "edge",
  "localId": 5,
  "curveType": "circle",
  "center": [5, 5, 10],
  "radius": 2.0,
  "length": 12.57,
  "start": [7, 5, 10],
  "end": [7, 5, 10],
  "adjacentFaces": [1, 2]
}
```

**Response (vertex example):**

```json
{
  "ok": true,
  "shapeId": 1,
  "entityType": "vertex",
  "localId": 0,
  "position": [0, 0, 0],
  "adjacentEdges": [0, 3, 8]
}
```

## Semantic Camera Commands

### `scene.look_at_entity`

Points the camera at an entity, keeping the current viewing distance.

**Request:**

```json
{
  "module": "scene",
  "action": "look_at_entity",
  "param": {
    "shapeId": 1,
    "entityType": "face",
    "localId": 2
  }
}
```

**Camera computation:**

| Entity type | Target           | Direction                              |
|-------------|------------------|----------------------------------------|
| Face        | Face center      | Face outward normal                    |
| Edge        | Edge midpoint    | Average normal of adjacent faces       |
| Vertex      | Vertex position  | Average normal of adjacent faces       |

The camera position is placed along the computed direction at the current
viewing distance from the target.  The up vector is chosen as the world axis
(Z or Y) most orthogonal to the viewing direction.

**Response:**

```json
{
  "ok": true,
  "camera": {
    "position": [5, 5, 25],
    "target": [5, 5, 10],
    "up": [0, 1, 0]
  }
}
```

### `scene.best_view_for_entity`

Computes and applies the optimal camera position to view an entity, including
automatic distance calculation to fit the entity in the viewport.

**Request:**

```json
{
  "module": "scene",
  "action": "best_view_for_entity",
  "param": {
    "shapeId": 1,
    "entityType": "face",
    "localId": 2,
    "padding": 1.5
  }
}
```

**Parameters:**

- `shapeId`, `entityType`, `localId` — identify the entity.
- `padding` (optional, default 1.5) — multiplier on the entity bounding box
  diagonal to determine viewing distance.  Larger values show more context.

**Camera computation:**

1. Compute the entity's local bounding box.
2. Determine viewing direction from entity normal/orientation (same logic as
   `look_at_entity`).
3. Calculate distance = `bbox_diagonal × padding` to ensure the entity fills
   the viewport comfortably.
4. Set target = entity center, position = target + direction × distance.
5. Choose up vector as the world axis most orthogonal to direction.

**Response:**

```json
{
  "ok": true,
  "camera": {
    "position": [5, 5, 18],
    "target": [5, 5, 10],
    "up": [0, 1, 0]
  },
  "entityBounds": {
    "min": [3, 3, 0],
    "max": [7, 7, 10]
  }
}
```

## Enhanced `capture_viewport`

Add `includeTopology` boolean parameter to the existing action (default
`false`).

**Request:**

```json
{
  "module": "scene",
  "action": "capture_viewport",
  "param": {
    "width": 1024,
    "height": 768,
    "captureImage": true,
    "includeMetadata": true,
    "includeTopology": true
  }
}
```

When `includeTopology` is `true`, each entry in `visibleShapes` includes a
`topology` field with the same structure as `describe_topology` (counts, faces
summary, edges summary).

```json
{
  "ok": true,
  "metadata": {
    "camera": { "eye": [0, 0, 50], "target": [5, 5, 5], "up": [0, 1, 0] },
    "visibleShapes": [
      {
        "shapeId": 1,
        "name": "Box_1",
        "worldBounds": { "min": [0,0,0], "max": [10,10,10] },
        "screenBBox": { "x": 120, "y": 80, "w": 400, "h": 350 },
        "topology": {
          "counts": { "faces": 7, "edges": 18, "vertices": 16 },
          "faces": [ ... ],
          "edges": [ ... ]
        }
      }
    ],
    "selections": [ ... ],
    "labels": [ ... ]
  },
  "image": "<base64 PNG>"
}
```

## Implementation Notes

### OCC Topology Extraction

Use the following OCC APIs for topology analysis:

- `TopExp_Explorer` to iterate faces, edges, vertices of a `TopoDS_Shape`.
- `BRep_Tool::Surface(face)` → `GeomAdaptor_Surface` to determine surface
  type, center, normal, axis, radius.
- `BRep_Tool::Curve(edge)` → `GeomAdaptor_Curve` for curve type, start/end
  points, center, radius.
- `BRep_Tool::Pnt(vertex)` for vertex coordinates.
- `GProp_GProps` + `BRepGProp::SurfaceProperties` for face area.
- `GProp_GProps` + `BRepGProp::LinearProperties` for edge length.
- `BRepBndLib::Add` for entity bounding boxes.

### Camera Computation

- Face normal: `BRepGProp_Face::Normal()` at the parametric center.
- Edge midpoint: evaluate curve at `(first + last) / 2`.
- Adjacent faces for an edge: `TopExp::MapShapesAndAncestors` with
  `TopAbs_EDGE` → `TopAbs_FACE`.
- Up vector selection: from `{(0,0,1), (0,1,0), (1,0,0)}`, pick the one with
  the smallest dot product against the viewing direction.

### Thread Safety

- Geometry queries read from `ShapeStore` under its existing mutex.
- Camera commands write to `ViewportState`, which is already thread-safe.
- `CaptureViewportAction` topology enhancement reuses the same `ShapeStore`
  read path.

### File Layout

```
src/libs/geometry/
├── include/opengeolab/geometry/
│   ├── describe_topology_action.hpp
│   ├── query_entity_info_action.hpp
│   └── topology_utils.hpp          (shared OCC extraction helpers)
├── src/
│   ├── describe_topology_action.cpp
│   ├── query_entity_info_action.cpp
│   └── topology_utils.cpp
├── test/
│   ├── describe_topology_action_test.cpp
│   └── query_entity_info_action_test.cpp

src/libs/scene/
├── include/opengeolab/scene/
│   ├── look_at_entity_action.hpp
│   └── best_view_for_entity_action.hpp
├── src/
│   ├── look_at_entity_action.cpp
│   ├── best_view_for_entity_action.cpp
│   └── capture_viewport_action.cpp  (enhanced)
├── test/
│   ├── look_at_entity_action_test.cpp
│   └── best_view_for_entity_action_test.cpp
```

### Registration

- `DescribeTopologyAction` and `QueryEntityInfoAction` registered in
  `GeometryModule::GeometryModule()`.
- `LookAtEntityAction` and `BestViewForEntityAction` registered in
  `SceneModule::SceneModule()`.

## Scope Boundaries

**In scope:**

- Basic topology: face/edge/vertex type, coordinates, dimensions, adjacency.
- Semantic camera: look-at, best-view, with auto-computed parameters.
- Enhanced capture: optional topology in viewport metadata.

**Out of scope (future iterations):**

- Feature recognition (holes, fillets, chamfers as named features).
- Automatic scene context injection into AI chat (always-on topology).
- Spatial relationship descriptions ("face A is above face B").
- Annotated screenshots with overlaid entity labels.
