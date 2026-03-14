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

## Workbench Layout Patterns

- Split large workbench surfaces into `theme`, `components`, and page-level `sections`; keep `Main.qml` focused on state assembly and signal wiring.
- Prefer `ColumnLayout` and `RowLayout` shells over deep absolute anchoring for ribbon-style workbenches so header, sidebar, viewport, and output areas resize without overlap.
- Avoid tall outer margins and oversized decorative chrome. Keep shell padding compact so engineering tools preserve usable viewport and panel space.
- Ribbon areas should show explicit grouping with titled clusters instead of a flat row of unrelated actions; if commands exceed width, allow horizontal scrolling instead of shrinking into visual noise.
- Put long-form editors, payload viewers, and automation consoles inside reusable card surfaces with consistent inner spacing, border strength, and monospace treatment.

## Visual System Guidance

- Use a deliberate engineering palette with one dominant action hue and a small number of support accents. Avoid mixing too many equally loud colors inside the same workbench surface.
- Keep icon language coherent. Reuse a single stroke/fill vocabulary across import, export, inspect, replay, viewport, and theme actions.
- Add motion to clarify hierarchy changes and affordance states: short hover/press scale transitions for controls, subtle panel entrance animations, and restrained ambient viewport motion.
- Decorative background shapes should support depth and atmosphere without consuming layout space or competing with dense CAE controls.
- Status overlays inside the viewport should stay compact and semi-transparent so they read as context, not as a second competing panel.

## Integration Expectations

- When a QML change depends on backend capabilities, align names and payloads with the C++ or Python-facing service contract.
- QML-facing APIs should expose high-level operations such as import, cleanup, mesh generation, selection, and camera control.