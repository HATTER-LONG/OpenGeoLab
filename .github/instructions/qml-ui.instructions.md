---
description: 'Qt6 QML UI rules for OpenGeoLab, with thin presentation layers over C++ services'
applyTo: 'apps/**/*.qml,**/qmldir'
---

# OpenGeoLab QML UI Development

## Presentation Boundary

- Keep QML focused on presentation, layout, interaction wiring, and lightweight state.
- Move heavy geometry, mesh, scene, and command logic into C++ services or controllers.
- Do not implement CAD or meshing algorithms in JavaScript inside QML.

## Shell Architecture

- Treat QML as the top layer over app-local controllers and service boundaries.
- Keep `Main.qml` focused on top-level state assembly, global wiring, and workbench layout.
- Store action metadata, page summaries, and request specs in `ActionRegistry.qml`.
- Store ribbon tab / group / action presentation structure in `RibbonConfig.qml`.
- Shared floating workflow surfaces should build on `FeaturePageBase.qml`; specialized workflows should compose it rather than cloning their own shell.
- Complex form validation, derived metrics, and request construction may live in sidecar JS helpers such as `GeometryCreatePageLogic.js`, but domain execution still belongs in C++ services.

## Request Assembly and Controller Integration

- QML business requests should be assembled as `{ module, action, param }` JSON and submitted through the generic controller interfaces.
- Prefer `submitServiceRequest()` for user-triggered operations that need progress updates or should not block the UI thread.
- Reserve `runServiceRequest()` for short synchronous flows.
- Do not add business-specific controller slots when a request spec plus the generic request pipeline can express the same behavior.
- Selection-related interactions should map to the selection system, not directly mutate render objects.

## Activity Center and Feedback

- Treat the Activity Center as the shared feedback surface for UI actions, module logs, and Python automation.
- The Events tab should surface `OperationLogService` output from module/app loggers.
- The Command Line tab should echo the latest JSON request, the latest public response, and Python command output.
- Keep runtime log emission level controls separate from in-panel visibility filtering.
- Show validation and advisory feedback close to the active form, not only inside logs.

## Structure, Layout, and Performance

- Keep components small and composable.
- Split large workbench surfaces into `theme`, `components`, and `sections`; keep deep business logic out of shell files.
- Extract repeated UI fragments into reusable components instead of duplicating blocks.
- Prefer explicit property bindings and signals over implicit side effects.
- Prefer `ColumnLayout` and `RowLayout` shells over deep absolute anchoring so header, sidebar, viewport, and output areas resize without overlap.
- Avoid expensive repeated bindings or large JavaScript loops in frequently updated visual paths.
- Keep model transformations and large data preparation outside QML.
- Be careful with object churn in dynamic views that may scale with scene complexity.

## Localization and UI Settings

- Wrap all user-facing QML strings in `qsTr()` from the start, including ribbon labels, menu actions, placeholder pages, status text, and panel titles.
- Treat English and Simplified Chinese as first-class UI languages. Runtime language switching should flow through an app-level controller that can trigger QML retranslation, rather than through ad hoc per-component string state.
- Keep theme and language controls together in the header pop menu workspace section. When both are present, place the language switch entry directly below the theme switch entry so appearance and localization settings stay discoverable.
- When introducing new QML or JS files with translatable strings, update the app CMake QML list, translation TS list, and matching `qmldir` entries explicitly so `lupdate` / `lrelease` continue to cover the full UI surface.
