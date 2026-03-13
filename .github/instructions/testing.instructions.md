---
description: 'Testing guidance for OpenGeoLab C++ modules, integration points, and automation workflows'
applyTo: 'libs/**/tests/**/*,**/*test*.{c,cc,cpp,cxx,py}'
---

# OpenGeoLab Testing

## Test Focus

- Test behavior that matters to engineering workflows: import, cleanup, meshing, selection, command replay, and data conversion.
- Prefer deterministic tests with small, representative fixtures.
- Keep tests independent from local machine state when possible.

## Coverage Priorities

- Geometry operations: valid and invalid topology handling, cleanup effects, and result consistency.
- Mesh operations: parameter validation, mesh generation success criteria, and quality metric behavior.
- Scene and selection: object identity, visibility, and selection result mapping.
- Command system: execute, undo, redo, and replay semantics.
- Python automation: high-level API call success, argument validation, and error propagation.

## Design

- Prefer focused unit tests for domain logic and targeted integration tests for service boundaries.
- Use realistic but minimal CAD / mesh fixtures.
- Avoid tests that depend on incidental log text unless logging is the behavior under test.
- When reproducing a bug, add a regression test that would have failed before the fix.

## Build Integration

- Respect the existing `ENABLE_TEST` switch.
- Place unit and smoke tests next to the owning library in `libs/<module>/tests/`.
- Do not force tests to require the full application startup path unless the scenario genuinely needs it.

## Validation

- A passing test should provide confidence that the user-facing workflow or module contract still holds.
- If a change cannot be tested automatically, explain the gap and describe the manual verification path.