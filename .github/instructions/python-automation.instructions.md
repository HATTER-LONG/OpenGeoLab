---
description: 'Python automation and pybind11 binding rules for OpenGeoLab high-level workflows'
applyTo: '**/*.py,python/python_wrapper/**/*.{h,hpp,c,cc,cpp,cxx}'
---

# OpenGeoLab Python Automation and Bindings

## Role of Python

- Python is the automation and orchestration layer for high-level CAE workflows.
- Expose stable, task-oriented operations such as geometry import, cleanup, meshing, quality checks, and camera / scene actions.
- Avoid exposing raw low-level kernel internals unless there is a clear scripting need.

## Binding Design

- Keep the pybind11 layer thin.
- Keep shared request-routing logic in the bridge library under `python/python_wrapper`, then expose it from the `opengeolab` pybind11 module.
- Convert between Python-friendly data and C++ domain types close to the binding boundary.
- Validate input early and report errors with messages that a script author can act on.
- Preserve deterministic behavior suitable for script replay and LLM-generated automation.

## API Shape

- Prefer command or service style entry points over large object graphs with unclear ownership.
- Use names that align with module naming conventions such as `geometry.import`, `mesh.generateSurface`, or `scene.selectFace`.
- Design APIs so an LLM can compose them into a valid workflow without needing private internal knowledge.

## Safety and Scriptability

- Do not assume Python callers understand OCC or Gmsh preconditions.
- Document required parameters, units, defaults, and failure modes.
- Avoid hidden global state when a request payload or explicit session object would be clearer.
- Prefer examples that reflect real engineering tasks instead of toy math snippets.
- If an operation is script-recordable, keep the generated call shape stable and readable.
- Generated Python should be suitable for replay in batch workflows.