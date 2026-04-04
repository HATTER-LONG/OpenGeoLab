# OpenGeoLab Command Protocol

## Request Format

All OpenGeoLab commands follow a JSON protocol:
- Request: `{module, action, param}` → Response: `{ok, result/errors}`

## Discovery Workflow

1. `list_modules()` → returns array of `{name, description}` for all modules
2. `describe_module(module_name)` → returns `{module, description, actions}` where each action has `{name, description}`
3. `execute_action(module, action, params)` → returns the action result with `{ok: true/false, ...}`

Always follow this order: discover modules → describe the relevant module → execute with correct params.

## Available Modules (discovered at runtime)

Use `list_modules` to discover.  Common modules include:
- **geometry**: Create shapes (create_box, create_cylinder, create_sphere, etc.)
- **mesh**: Generate meshes from geometry (mesh_shape, etc.)
- **scene**: Selection, camera, labels, hover
- **io**: Import/export files

## Parameter Constraints

- Parameter names and types must match the schema exactly (case-sensitive)
- `shapeId` values come from geometry creation results — never fabricate them
- Entity types: `GeoSolid`, `GeoFace`, `GeoEdge`, `GeoVertex`, `MeshNode`, `MeshEdge`, `MeshElement`
- Numeric parameters use standard JSON number format (no units)

## Error Handling

- If `ok` is `false`, the `errors` field contains an error description
- Common errors: missing required parameter, invalid shapeId, unknown action name
- On error, re-check the parameter schema with `describe_module` before retrying
