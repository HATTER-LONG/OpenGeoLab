---
description: 'OpenGeoLab C++20 core, CAE domain modeling, and module boundary rules'
applyTo: 'libs/**/*.{h,hpp,c,cc,cpp,cxx},apps/**/*.{h,hpp,c,cc,cpp,cxx},python/python_wrapper/**/*.{h,hpp,c,cc,cpp,cxx}'
---

# OpenGeoLab C++ Development

Follow the repository-wide guidance from `.github/copilot-instructions.md` first.

## Core Rules

- Use C++20 features when they improve clarity or correctness, but avoid novelty-driven abstractions.
- Keep geometry, mesh, scene, render, selection, and command responsibilities separated.
- Prefer explicit request/response contracts and ownership boundaries over hidden side effects.
- Keep public APIs small, stable, and suitable for exposure to QML and Python when needed.

## Request and Action Protocol

- Service boundaries use the canonical `ServiceRequest { module, action, param }` and `ServiceResponse { success, module, action, message, payload }` contract.
- Validate `module`, `action`, and `param` close to the public entry point. `param` should be a JSON object, not an overloaded scalar or array payload.
- Each domain module should expose one service keyed by module name and route work through registered `[Domain]Action` factories. Add new user-visible capability as a concrete action, not as an ad hoc controller helper.
- Prefer engineering action names such as `createBox`, `inspectModel`, `buildScene`, `buildFrame`, `pickEntity`, and `boxSelect` over placeholder or UI-specific wording.
- When a request returns user-visible data, prefer structured payload keys such as `summary`, `model`, `sceneGraph`, `renderFrame`, or `selectionResult` over flattened text blobs. Include automation-friendly data such as `equivalentPython` when it materially helps script parity.

## Module Boundaries

- Core domain logic should not live in UI glue code.
- Renderer code must not directly operate on OpenCascade topology or Gmsh internals.
- Convert domain entities into render-oriented data structures before issuing draw calls.
- QML, embedded Python, and external Python entry points should all converge on the same generic request path instead of bypassing module services.
- Shared cross-module capabilities should be exposed through services or well-defined interfaces, not by reaching across unrelated modules.
- Avoid circular dependencies. If two modules start depending on each other, introduce an interface, data transfer object, or service boundary.

## Public Surface

- Public declarations belong in `libs/<module>/include/ogl/<module>/`.
- Module source files belong in `libs/<module>/src/`.
- Application-only controllers and startup glue belong in `apps/OpenGeoLabApp`.
- Do not expose third-party details in public headers unless the dependency is intentionally part of the API.
- Prefer forward declarations in headers when they reduce unnecessary coupling.
- If a module is intended to build as a shared library, public headers must include the module export header and annotate exported API types or functions.

## Modeling and Reliability

- Model CAE entities with names that reflect engineering semantics, not temporary implementation details.
- Distinguish clearly between topology, mesh, scene graph, render data, and command intent.
- Preserve IDs and object identity across operations that need selection, undo/redo, or script replay.
- Validation and precondition checks should happen close to public entry points.
- Fail fast on invalid state at service or command boundaries.
- Report recoverable runtime issues with actionable messages.
- Use `ModuleLogger.hpp` and the `OGL_LOG_*` macros for workflow visibility, dependency resolution, import/export steps, progress milestones, and failure diagnostics.
- Use `ProgressCallback` for intermediate progress reporting instead of controller-specific side channels when a service may run asynchronously.
- Do not silently swallow OCC, Gmsh, Qt, or filesystem failures.

## Performance and Style

- Avoid unnecessary copies of large geometry, mesh, and render buffers.
- Be explicit about expensive conversions between topology, mesh, and GPU-ready data.
- Keep hot-path allocation patterns simple and predictable.
- Optimize only after preserving correctness and maintainable structure.
- Follow `.clang-format` and `.clang-tidy` in the repository as the source of truth for C++ layout and naming.
- Format C++ code with the repository `.clang-format` style before finishing changes.
- Use `CamelCase` for namespaces, classes, structs, enums, and unions. Prefer nested namespaces such as `OGL::Geometry` and `OGL::Render`.
- Use `camelBack` for functions, methods, and non-constant variables.
- Use `m_` + `camelBack` only for private or protected class members.
- Keep struct fields and other public data-transfer members in plain `camelBack` without an `m_` prefix.
- Keep functions focused. Split long orchestration code into small helpers if it improves readability.
- Prefer descriptive names over abbreviations unless the term is standard in CAD / CAE.

## Testing

- New non-trivial domain logic should come with tests in the owning module's `tests/` directory.
- Prefer testing observable behavior over private implementation details.
- Test public action routing by dispatching canonical action names and asserting structured payload fields, not just success flags.
- Cover failure cases for request validation, progress reporting, geometry cleanup, mesh generation, selection, and command replay when those paths are touched.
