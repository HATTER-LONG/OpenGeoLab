---
description: 'CMake, dependency wiring, Qt QML packaging, and install rules for OpenGeoLab'
applyTo: '**/CMakeLists.txt,cmake/**/*.cmake,cmake/**/*.cmake.in'
---

# OpenGeoLab CMake Development

These instructions apply to build-system changes in OpenGeoLab.

## General Guidelines

- Preserve a clear separation between project configuration, dependency resolution, target definition, and install / deploy logic.
- Prefer small, explicit changes over large CMake rewrites.
- Match the existing formatting style and command layout used in the repository.

## Dependency Management

- Use the existing CPM-based helpers and package resolution flow before introducing a new dependency pattern.
- Keep third-party version declarations centralized when possible.
- Distinguish between CPM-fetched dependencies and pre-installed dependencies such as OpenCASCADE, Qt, and Gmsh.
- When adding a dependency, document whether it is required at configure time, build time, runtime, or install time.

## Target Design

- Prefer target-based CMake APIs.
- Keep include directories, link libraries, compile features, and definitions attached to the owning target.
- Do not leak private implementation dependencies through public interfaces unless intended.
- Avoid global flags when target-specific settings are sufficient.
- First-party libraries should be able to switch between static and shared builds through the OpenGeoLab library-type option.
- When a library may be built as shared on Windows, use generated export headers instead of ad hoc `__declspec` blocks.

## Qt and QML

- Use Qt target APIs consistently for executable and QML module setup.
- Keep QML file lists explicit enough to make packaging and deploy behavior obvious.
- When changing QML modules, also review install rules and deploy script behavior.

## Install and Runtime Packaging

- Ensure new runtime dependencies are covered by install rules on supported platforms.
- Changes to QML resources, plugins, or shared libraries must consider deployed layout, not only local builds.
- Windows-specific runtime packaging changes should be reviewed with dependency copy behavior in mind.

## Tests

- Gate tests behind the existing `ENABLE_TEST` option unless there is a strong reason not to.
- Keep test registration local to each module's `tests/` subtree.
- New tests should not force unrelated dependencies into the main application target.

## Validation

- After modifying CMake, verify configure and build still work.
- Prefer fixing the actual dependency or target relationship instead of layering more conditional branches on top.