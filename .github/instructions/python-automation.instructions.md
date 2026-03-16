---
description: 'Python automation and pybind11 binding rules for OpenGeoLab high-level workflows'
applyTo: '**/*.py,python/python_wrapper/**/*.{h,hpp,c,cc,cpp,cxx}'
---

# OpenGeoLab Python Automation and Bindings

## Role of Python

- Python is the automation and orchestration layer for high-level CAE workflows.
- Expose stable, task-oriented operations such as geometry import, cleanup, meshing, quality checks, and camera / scene actions.
- Avoid exposing raw low-level kernel internals unless there is a clear scripting need.

## Request Surface

- External Python uses `opengeolab.process(request)` or `OpenGeoLabPythonBridge.process(request)` with the canonical `{ module, action, param }` request envelope.
- Embedded Python inside the app should keep the same request shape through `opengeolab_app.process(request)`, even if the runtime return type differs from the external pybind layer.
- `module` and `action` are required. `param` may be omitted or null at the call site, but it must normalize to a JSON object before entering the shared command path.
- Prefer examples that show concrete request objects, not dotted pseudo-APIs that do not exist in code.

## Binding Design

- Keep the pybind11 layer thin.
- Keep shared request-routing logic in the bridge library under `python/python_wrapper`, then expose it from the `opengeolab` pybind11 module.
- Route Python calls into the shared `CommandService` / request parser instead of adding module-specific shortcuts that bypass the canonical request contract.
- Convert between Python-friendly data and C++ domain types close to the binding boundary.
- Validate input early and report errors with messages that a script author can act on.
- Preserve deterministic behavior suitable for script replay and LLM-generated automation.

## API Shape

- Prefer a generic `process(request)` boundary plus thin helpers that compose the same request envelope over large object graphs with unclear ownership.
- When convenience helpers are introduced, they should still call the shared request path and remain readable when exported to Python scripts.
- Use canonical module/action names in examples and docs instead of implying unsupported dotted-call APIs.
- Design APIs so an LLM can compose them into a valid workflow without needing private internal knowledge.

## Safety and Scriptability

- Do not assume Python callers understand OCC or Gmsh preconditions.
- Document required request fields, units, defaults, and failure modes.
- Avoid hidden global state when a request payload or explicit session object would be clearer.
- Prefer examples that reflect real engineering tasks instead of toy math snippets.
- If an operation is script-recordable, keep the generated call shape stable and readable.
- Generated Python should be suitable for replay in batch workflows, typically by reusing the same request envelope through `bridge.process(request)` or `opengeolab.process(request)`.
