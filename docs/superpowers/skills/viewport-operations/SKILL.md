---
name: viewport-operations
description: Use when needing to interact with the 3D viewport — capturing screenshots, querying scene contents, manipulating camera, managing selections or labels on geometry
---

# Viewport Operations

## Overview

OpenGeoLab exposes **20 scene actions** through a unified JSON request/response protocol. All actions belong to the `scene` module and follow the same envelope:

```json
{"module": "scene", "action": "<action_name>", "param": { ... }}
```

**AI tool path:** Use `execute_action(module="scene", action="<name>", params={...})` from the Copilot tool.

**Discovery path:** Use `describe_action(module_name="scene", action_name="<name>")` to get full parameter/return schema for any action at runtime.

## When to Use

- Need to **see** what's in the viewport → `capture_viewport`
- Need to **query** scene contents → `list_nodes`, `query_selection`, `describe_labels`
- Need to **control** the camera → `set_camera`, `set_view_preset`, `fit_to_scene`
- Need to **select/deselect** geometry → `select`, `deselect`, `clear_selection`, `pick_area`
- Need to **label** entities for reference → `add_label`, `remove_label`, `describe_labels`
- Need to **show/hide** objects → `set_visibility`

## Entity Model

All entity references use a 3-tuple:

| Field | Type | Description |
|-------|------|-------------|
| `shapeId` | int | 0-based geometry source ID |
| `entityType` | string | `"GeoVertex"` \| `"GeoEdge"` \| `"GeoWire"` \| `"GeoFace"` \| `"GeoSolid"` |
| `localId` | int | 1-based topology-local ID within the shape |

## Quick Reference — All 20 Actions

### Capture & Query

| Action | Key Params | Returns |
|--------|-----------|---------|
| `capture_viewport` | `width?=1024`, `height?=768`, `captureImage?=true`, `includeMetadata?=true` | `metadata` + base64 `image` |
| `list_nodes` | ø | `nodes[]` with `sourceType`, `sourceId`, `name`, `visible` |
| `query_selection` | ø | `selections[]` (entity 3-tuples) |
| `describe_labels` | ø | `labels[]`, `colorLegend`, `totalLabels` |

### Camera

| Action | Key Params | Returns |
|--------|-----------|---------|
| `set_camera` | `position[3]`, `target[3]`, `up[3]` (all required) | `ok` |
| `set_view_preset` | `preset`: `"Front"` \| `"Back"` \| `"Top"` \| `"Bottom"` \| `"Left"` \| `"Right"` \| `"Isometric"` | `ok` |
| `fit_to_scene` | ø | `ok` |

### Selection

| Action | Key Params | Returns |
|--------|-----------|---------|
| `select` | `entities[]`, `append?=true` | `selected` count |
| `deselect` | `entities[]` | `ok` |
| `clear_selection` | ø | `ok` |
| `pick_area` | `x0`, `y0`, `x1`, `y1`, `coordType?="normalized"`, `pickAction?="Add"` | `async: true` (query later) |
| `set_pick_mode` | `pickMask?`, `enabled?` | `ok` |
| `set_hover` | `entity` (3-tuple or null) | `ok` |

### Labels

| Action | Key Params | Returns |
|--------|-----------|---------|
| `add_label` | `shapeId`, `entityType`, `localId` | `text` (generated label) |
| `remove_label` | `shapeId`, `entityType`, `localId` | `ok` |
| `clear_labels` | ø | `ok` |
| `set_labels_visible` | `visible` (bool) | `ok` |
| `set_auto_label` | `enabled` (bool) | `ok` |

### Scene Management

| Action | Key Params | Returns |
|--------|-----------|---------|
| `set_visibility` | `nodes[]` with visibility flags | `ok` |
| `new_model` | ø | `ok` (clears workspace) |

## Capture Viewport — Detailed

The most important action for AI "seeing" the viewport.

```json
{
  "module": "scene",
  "action": "capture_viewport",
  "param": {"width": 1024, "height": 768}
}
```

**Response structure:**

```json
{
  "ok": true,
  "action": "capture_viewport",
  "metadata": {
    "viewport": {"width": 1024, "height": 768},
    "camera": {
      "eye": [x, y, z],
      "target": [x, y, z],
      "up": [x, y, z]
    },
    "visibleShapes": [
      {
        "shapeId": 0,
        "name": "Box_1",
        "screenBBox": {"x": 120, "y": 80, "w": 300, "h": 250}
      }
    ],
    "selections": [
      {"shapeId": 0, "type": "GeoFace", "localId": 3}
    ],
    "labels": [
      {"text": "F:3", "entity": {"shapeId": 0, "type": "GeoFace", "localId": 3}}
    ],
    "hover": null
  },
  "image": "<base64-PNG>"
}
```

**Screen coordinate system:** Origin (0,0) at top-left; X right, Y down; pixel units relative to capture resolution.

**`screenBBox`** is `null` when shape is entirely off-screen.

**AI tool shortcut:** `capture_viewport(width=1024, height=768)` — image is automatically attached as a blob; only metadata JSON is returned to the model.

## Label Color Legend

| Entity Type | Prefix | Color | Hex |
|------------|--------|-------|-----|
| GeoVertex | V | Red | `#FF0000` |
| GeoEdge | E | Blue | `#0000FF` |
| GeoFace | F | Green | `#00FF00` |
| GeoSolid | S | Orange | `#FF8800` |

Label text format: `<prefix>:<localId>` (e.g., `F:3` = Face #3). Labels behind geometry appear semi-transparent (30% opacity).

## Common Workflows

### "What am I looking at?"

```
1. capture_viewport()           → get screenshot + metadata
2. Read visibleShapes[]         → identify shapes by name & screen position
3. Read selections[]            → see what's currently selected
```

### "Select a specific face and label it"

```
1. list_nodes()                 → find shapeId by name
2. select(entities=[{shapeId: 0, entityType: "GeoFace", localId: 3}], append: false)
3. add_label(shapeId: 0, entityType: "GeoFace", localId: 3)
4. capture_viewport()           → verify visually
```

### "Change viewpoint to see object from top"

```
1. set_view_preset(preset: "Top")
2. fit_to_scene()
3. capture_viewport()           → verify new view
```

### "Box-select entities in a region"

```
1. pick_area(x0: 0.2, y0: 0.3, x1: 0.8, y1: 0.7, coordType: "normalized")
2. query_selection()            → get resulting selection (after next render frame)
```

## Async Action Notes

- **`pick_area`** returns immediately with `async: true`. The actual selection result appears after the next render frame. Call `query_selection()` after a short delay to retrieve results.
- **`capture_viewport`** with `captureImage: true` has a 5-second timeout for image capture. If the render thread does not respond, `image` is `null` and `imageError` explains the failure. Metadata is always returned regardless.

## Discovery at Runtime

If you need the exact parameter schema for any action:

```
describe_action(module_name="scene", action_name="capture_viewport")
```

This returns the full `params` and `returns` definition from the C++ `describe()` method, which is always authoritative.
