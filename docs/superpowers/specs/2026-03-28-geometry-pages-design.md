# Geometry Creation Pages — Design Specification

## Problem

Geometry creation (Box, Cylinder, Sphere, Torus) currently fires directly from `Main.qml::openActionPage` with hardcoded default parameters. Users cannot preview or adjust parameters before execution.

## Solution

Create 4 floating parameter pages using the existing `FunctionPageBase + MainPages` routing framework, with 3 shared input components following the OGL sister repo's layout pattern, adapted to the current theme system.

## Architecture

```
src/app/resource/qml/
├── MainPages.qml                    (modify: register 4 pages in componentMap)
├── components/
│   ├── FunctionPageBase.qml         (existing, no changes)
│   ├── DimensionInput.qml           (NEW: shared number input)
│   ├── ParamField.qml               (NEW: shared text input)
│   ├── CoordinateField.qml          (NEW: shared XYZ input)
│   └── pages/
│       ├── CreateBoxPage.qml        (NEW)
│       ├── CreateCylinderPage.qml   (NEW)
│       ├── CreateSpherePage.qml     (NEW)
│       └── CreateTorusPage.qml      (NEW)
├── Main.qml                         (modify: remove direct geometry dispatch)
```

## Shared Components

### DimensionInput.qml

Single-line number input with a color-coded axis badge.

```
┌──────────────────────────┐
│ [W] │ 10.000             │
└──────────────────────────┘
```

**Properties:**
- `label: string` — badge letter (e.g. "W", "H", "R")
- `value: real` — bound numeric value
- `decimals: int` — decimal places (default 3)
- `accentColor: color` — badge tint (e.g. red for X/W, green for Y/H, blue for Z/D)
- `tooltipText: string` — optional tooltip on badge hover

**Signal:** `valueEdited(real newVal)`

**Internals:** Rectangle with RowLayout containing a tinted badge Rectangle + TextField. Uses DoubleValidator with bottom 0.001. On editingFinished, clamps and emits. Uses `MainPages.theme` for styling.

### ParamField.qml

Labeled text input for string parameters (e.g. object name).

```
Name Label
┌──────────────────────────┐
│ placeholder text...      │
└──────────────────────────┘
```

**Properties:**
- `label: string` — label above input
- `placeholder: string` — placeholder text
- `value: string` — current text

**Signal:** `valueEdited(string newValue)`

**Internals:** Column with label Text + bordered Rectangle containing TextField. Uses `MainPages.theme`.

### CoordinateField.qml

3D coordinate input row with label. Composes 3 DimensionInput instances.

```
Origin Point
┌─[X] 0.000─┐ ┌─[Y] 0.000─┐ ┌─[Z] 0.000─┐
```

**Properties:**
- `label: string` — section label
- `coordX, coordY, coordZ: real`
- `decimals: int` (default 3)

**Signal:** `coordinateChanged(real x, real y, real z)`

**Internals:** Column with label Text + RowLayout of 3 DimensionInput (X=red, Y=green, Z=blue). DimensionInput instances use `onValueEdited` to update properties and emit `coordinateChanged`.

## Geometry Pages

All 4 pages extend `FunctionPageBase` and share this structure:

```
┌─────────────────────────────────┐
│ [icon]  Create Box         [✕]  │  ← FunctionPageBase titlebar
├─────────────────────────────────┤
│  Box Name                       │  ← ParamField
│  ┌─────────────────────────┐    │
│  │ Auto-generated if empty │    │
│  └─────────────────────────┘    │
│                                 │
│  Origin Point                   │  ← CoordinateField
│  [X] 0.000  [Y] 0.000  [Z]0.0 │
│                                 │
│  Dimensions                     │  ← Section label
│  [W] 10.000 [H] 10.000 [D]10. │  ← DimensionInput row
│                                 │
│  ┌─ Volume: 1000.000 ────────┐  │  ← Info section
│  └───────────────────────────┘  │
├─────────────────────────────────┤
│      [Execute]    [Cancel]      │  ← FunctionPageBase buttons
└─────────────────────────────────┘
```

### CreateBoxPage.qml

| Property     | Type   | Default   | Control        |
|-------------|--------|-----------|----------------|
| actionId    | string | "addBox"  | (FunctionPageBase) |
| boxName     | string | ""        | ParamField     |
| originX/Y/Z| real   | 0.0     | CoordinateField|
| dimW        | real   | 10.0    | DimensionInput W (red) |
| dimH        | real   | 10.0    | DimensionInput H (green) |
| dimD        | real   | 10.0    | DimensionInput D (blue) |

**Info:** Volume = W × H × D

**getParameters():**
```json
{
  "module": "geometry",
  "action": "create_box",
  "param": {
    "name": "Box_...",
    "x": 0, "y": 0, "z": 0,
    "width": 10, "height": 10, "depth": 10
  }
}
```

**Icon:** `cubeOutline`

### CreateCylinderPage.qml

| Property     | Type   | Default       | Control        |
|-------------|--------|---------------|----------------|
| actionId    | string | "addCylinder" | (FunctionPageBase) |
| cylinderName| string | ""            | ParamField     |
| centerX/Y/Z| real   | 0.0     | CoordinateField|
| radius      | real   | 5.0     | DimensionInput R (red) |
| cylHeight   | real   | 10.0    | DimensionInput H (blue) |

**Info:** Volume = π × R² × H, Surface Area = 2πR(R + H)

**getParameters():**
```json
{
  "module": "geometry",
  "action": "create_cylinder",
  "param": {
    "name": "Cylinder_...",
    "x": 0, "y": 0, "z": 0,
    "radius": 5, "height": 10
  }
}
```

**Icon:** `cylinder`

### CreateSpherePage.qml

| Property     | Type   | Default      | Control        |
|-------------|--------|--------------|----------------|
| actionId    | string | "addSphere"  | (FunctionPageBase) |
| sphereName  | string | ""           | ParamField     |
| centerX/Y/Z| real   | 0.0     | CoordinateField|
| radius      | real   | 5.0     | DimensionInput R (red) |

**Info:** Volume = 4/3 π R³, Surface Area = 4πR², Diameter = 2R

**getParameters():**
```json
{
  "module": "geometry",
  "action": "create_sphere",
  "param": {
    "name": "Sphere_...",
    "x": 0, "y": 0, "z": 0,
    "radius": 5
  }
}
```

**Icon:** `sphere`

### CreateTorusPage.qml

| Property     | Type   | Default     | Control        |
|-------------|--------|-------------|----------------|
| actionId    | string | "addTorus"  | (FunctionPageBase) |
| torusName   | string | ""          | ParamField     |
| centerX/Y/Z| real   | 0.0     | CoordinateField|
| majorRadius | real   | 10.0    | DimensionInput R1 (red) |
| minorRadius | real   | 3.0     | DimensionInput R2 (green) |

**Info:** Volume = 2π²Rr², Surface Area = 4π²Rr, Outer Diameter = 2(R+r)

**Validation:** Warning box when minorRadius ≥ majorRadius (orange tint, using theme.accentC).

**getParameters():**
```json
{
  "module": "geometry",
  "action": "create_torus",
  "param": {
    "name": "Torus_...",
    "x": 0, "y": 0, "z": 0,
    "majorRadius": 10, "minorRadius": 3
  }
}
```

**Icon:** `torus`

## MainPages.componentMap Registration

```javascript
readonly property var componentMap: ({
    "addBox":      { path: "components/pages/CreateBoxPage.qml" },
    "addCylinder": { path: "components/pages/CreateCylinderPage.qml" },
    "addSphere":   { path: "components/pages/CreateSpherePage.qml" },
    "addTorus":    { path: "components/pages/CreateTorusPage.qml" }
})
```

## Main.qml Changes

Remove the `geometryActions` object and its `if (actionKey in geometryActions)` block from `openActionPage()`. These actions now route through `MainPages.hasPage()` → `MainPages.handleAction()`.

## Theme Usage

All new components access theme via `MainPages.theme` (consistent with FunctionPageBase). No `pragma ComponentBehavior: Bound` on pages or shared input components — they are dynamically created by MainPages.

Accent colors for axis badges:
- Red `"#E53935"` for X, W, R, R1
- Green `"#43A047"` for Y, H, R2
- Blue `"#1E88E5"` for Z, D, H (when alongside R)

Info section uses `MainPages.theme.surfaceMuted` background with `MainPages.theme.radiusSmall` radius.

## Files to Create

1. `src/app/resource/qml/components/DimensionInput.qml`
2. `src/app/resource/qml/components/ParamField.qml`
3. `src/app/resource/qml/components/CoordinateField.qml`
4. `src/app/resource/qml/components/pages/CreateBoxPage.qml`
5. `src/app/resource/qml/components/pages/CreateCylinderPage.qml`
6. `src/app/resource/qml/components/pages/CreateSpherePage.qml`
7. `src/app/resource/qml/components/pages/CreateTorusPage.qml`

## Files to Modify

1. `src/app/resource/qml/MainPages.qml` — populate componentMap
2. `src/app/resource/qml/Main.qml` — remove geometryActions block
3. `src/app/CMakeLists.txt` — add 7 new QML_FILES
4. `src/app/resource/translations/opengeolab_zh_CN.ts` — add translations

## Testing

- Build: `cmake --build build --config RelWithDebInfo --parallel 4`
- Test: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- Manual: click each geometry action in menu → floating page opens → parameter adjustment → execute sends correct JSON
