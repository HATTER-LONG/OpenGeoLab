---
description: 'Documentation and Doxygen rules for OpenGeoLab interfaces, constraints, and design rationale'
applyTo: '**/*.md,libs/**/*.{h,hpp,c,cc,cpp,cxx},apps/**/*.{h,hpp,c,cc,cpp,cxx},python/python_wrapper/**/*.{h,hpp,c,cc,cpp,cxx}'
---

# OpenGeoLab Documentation

These instructions apply to Markdown documentation and code comments.

## General Guidelines

- Document interfaces, constraints, invariants, side effects, and design rationale.
- Do not comment obvious code behavior.
- Prefer concise, high-signal explanations over long narrative blocks.

## Doxygen Rules

- Follow the repository guidance in `doxygen_comment_style.md`.
- Public headers should include `@file` and `@brief`.
- Public or cross-module classes should have a short class-level `@brief` comment.
- Public functions should document non-obvious parameters, return conditions, notes, and warnings.
- Use `///<` for enum member comments when needed.
- If a module exports symbols across shared-library boundaries, keep the public export header part of the documented interface.

## Commenting Strategy

- Explain why a non-obvious approach exists, especially around geometry healing, meshing strategy, scene synchronization, and deploy behavior.
- Add implementation comments only where a reader would otherwise miss a constraint or tradeoff.
- Avoid duplicating names or types already obvious from the signature.

## Markdown Documents

- Keep repository docs actionable.
- Use tables or short lists when describing module responsibilities, dependency rules, or workflow steps.
- When documenting architecture, distinguish current implementation from target architecture if the repository is still evolving.

## Examples

- Examples should reflect OpenGeoLab workflows such as geometry import, mesh generation, command replay, or Python automation.
- Favor realistic snippets over generic placeholder code.