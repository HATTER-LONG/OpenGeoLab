---
description: 'Testing guidance for OpenGeoLab C++ modules, integration points, and automation workflows'
applyTo: 'libs/**/tests/**/*,**/*test*.{c,cc,cpp,cxx,py}'
---

# OpenGeoLab Testing

## Test Focus

- Test behavior that matters to engineering workflows: structured request routing, geometry creation and inspection, scene/render/selection data flow, command replay/export, and automation feedback.
- Prefer deterministic tests with small, representative fixtures.
- Keep tests independent from local machine state when possible.

## Coverage Priorities

- Module services: supported and unsupported action routing, request validation, and stable payload shape.
- Geometry operations: parameter normalization, result consistency, and progress callback behavior.
- Scene, render, and selection: scene graph / render frame / selection result mapping and object identity.
- Command system: execute, record, replay, export semantics, and response consistency. Do not assume undo / redo exists unless the change adds it.
- Python and app automation: high-level `process(request)` success, argument validation, async request completion, and error propagation.
- Activity feedback: operation log source metadata, progress state changes, and controller-visible summaries when those are part of the user-facing contract.

## Design

- Prefer focused unit tests for domain logic and targeted integration tests for service boundaries.
- Use realistic but minimal CAD / mesh fixtures.
- Avoid tests that depend on incidental log text unless logging or metadata is the behavior under test.
- When a feature exposes structured JSON payloads, assert the meaningful fields instead of exact pretty-printed text blobs.
- For progress-capable operations, capture intermediate progress values and messages rather than checking only final success.
- When reproducing a bug, add a regression test that would have failed before the fix.

## Build Integration

- Respect the existing `ENABLE_TEST` switch.
- Place unit tests next to the owning library in `libs/<module>/tests/`.
- Do not force tests to require the full application startup path unless the scenario genuinely needs it.

## Validation

- A passing test should provide confidence that the user-facing workflow or module contract still holds.
- If a change cannot be tested automatically, explain the gap and describe the manual verification path.
