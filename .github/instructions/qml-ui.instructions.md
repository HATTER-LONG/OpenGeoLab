---
description: 'Qt6 QML UI rules for OpenGeoLab, with thin presentation layers over C++ services'
applyTo: 'apps/**/*.qml,**/qmldir'
---

# OpenGeoLab QML UI Development

## Presentation Boundary

- Keep QML focused on presentation, layout, interaction wiring, and lightweight state.
- Move heavy geometry, mesh, scene, and command logic into C++ services or controllers.
- Do not implement CAD or meshing algorithms in JavaScript inside QML.

## UI Architecture

- Treat QML as the top layer over interaction controllers, scene managers, and services.
- Keep QML files under `apps/OpenGeoLabApp/qml/` and pair them with app-local controllers rather than domain libraries.
- User intent should flow from UI events to controller / service boundaries, then into command execution and scene updates.
- Prefer explicit property bindings and signals over implicit side effects.

## Interaction Rules

- Selection-related interactions should map to the selection system, not directly manipulate render objects.
- Destructive or state-changing user operations should align with command-system concepts when the backend supports it.
- Keep camera and viewport interactions consistent with a 3D engineering workflow.

## Structure and Performance

- Keep components small and composable.
- Extract repeated UI fragments into reusable components instead of duplicating blocks.
- Use clear names for properties and signals that match engineering concepts visible to the user.
- Avoid expensive repeated bindings or large JavaScript loops in frequently updated visual paths.
- Keep model transformations and large data preparation outside QML.
- Be careful with object churn in dynamic views that may scale with scene complexity.

## Integration Expectations

- When a QML change depends on backend capabilities, align names and payloads with the C++ or Python-facing service contract.
- QML-facing APIs should expose high-level operations such as import, cleanup, mesh generation, selection, and camera control.