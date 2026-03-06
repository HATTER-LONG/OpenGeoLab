# OpenGeoLab Architecture Notes

## Goal

OpenGeoLab currently combines three prototype tracks: CAD-style geometry handling, mesh generation, and interactive visualization. The app module ties QML, backend services, and render control together.

## Module Responsibilities

- app: QML bridge, async service execution, viewport objects, and user interaction plumbing.
- geometry: authoritative geometry document, entity hierarchy, OCC shape ingestion, and geometry render-data generation.
- mesh: authoritative mesh document, node/element storage, relation maps, and mesh render-data generation.
- render: render snapshots, scene control, GPU passes, picking, and highlight behavior.
- io: model import services.

## Boundary Rules

- Cross-module dependencies should flow through public headers in include/.
- geometry and mesh produce render snapshots; render consumes them and manages GPU lifetime.
- app should coordinate through services and controllers instead of mutating render internals directly.

## Improvements Completed In This Round

- Added document-level reader/writer locking in GeometryDocumentImpl and MeshDocumentImpl.
- Switched RenderSceneController to shared immutable snapshot publication.
- Fixed BackendService failure signal argument ordering and cancellation behavior.
- Upgraded the test target to C++20 and added mesh document / render-data coverage.

## Recommended Next Steps

- Reduce the dependence on global singleton macros such as GeoDocumentInstance and MeshDocumentInstance.
- Move the QML-to-backend boundary away from raw JSON strings toward stronger typed request objects.
- Split RenderSceneController responsibilities into smaller units for camera state, render-data refresh, and visibility state.
- Add broader tests for geometry indexing, pick resolution, and render batch generation.

## Suggested Evolution Order

1. Introduce explicit document contexts and gradually weaken singleton ownership.
2. Define structured request payloads and unified error codes for backend services.
3. Add finer-grained render-data invalidation to reduce full snapshot rebuilds.
4. Strengthen mesh-to-geometry provenance tracking for future editing and partial rebuild workflows.