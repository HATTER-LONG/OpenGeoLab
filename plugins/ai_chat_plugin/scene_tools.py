"""Cached hierarchical schema from opengeolab_pywrapper.describe().

Provides four layers of progressive discovery:
  Layer 0 — list_modules():                module names + descriptions
  Layer 1 — describe_module(name):         action names + descriptions
  Layer 2 — describe_action(mod, action):  full param/return schema
  Layer 3 — execute_action(mod, act, p):   execute via pywrapper.process()

In standalone mode (pywrapper unavailable), discovery functions return
empty results and execute_action returns an error dict.
"""
from __future__ import annotations

import json

try:
    import opengeolab_pywrapper as _wrapper

    HOSTED_MODE = True
except ImportError:
    _wrapper = None  # type: ignore[assignment]
    HOSTED_MODE = False

_cached_schema: dict | None = None


def _ensure_schema() -> dict:
    """Fetch and cache the full describe() schema on first call."""
    global _cached_schema
    if _cached_schema is None and HOSTED_MODE:
        _cached_schema = json.loads(_wrapper.describe())
    return _cached_schema or {}


def invalidate_cache() -> None:
    """Force re-fetch of the schema on next access."""
    global _cached_schema
    _cached_schema = None


def list_modules() -> list[dict]:
    """Layer 0: module names and descriptions."""
    schema = _ensure_schema()
    return [
        {"name": m["name"], "description": m.get("description", "")}
        for m in schema.get("modules", [])
    ]


def describe_module(module_name: str) -> dict | None:
    """Layer 1: action names and one-line descriptions for a module."""
    schema = _ensure_schema()
    for m in schema.get("modules", []):
        if m["name"] == module_name:
            return {
                "module": m["name"],
                "description": m.get("description", ""),
                "actions": [
                    {"name": a["name"], "description": a.get("description", "")}
                    for a in m.get("actions", [])
                ],
            }
    return None


def describe_action(module_name: str, action_name: str) -> dict | None:
    """Layer 2: full param/return schema for one action."""
    schema = _ensure_schema()
    for m in schema.get("modules", []):
        if m["name"] == module_name:
            for a in m.get("actions", []):
                if a["name"] == action_name:
                    return a
    return None


def execute_action(
    module: str, action: str, params: dict, progress_callback=None
) -> dict:
    """Layer 3: execute an action via opengeolab_pywrapper.process()."""
    if not HOSTED_MODE:
        return {"ok": False, "error": "Not in hosted mode — pywrapper unavailable"}
    request = json.dumps({"module": module, "action": action, "param": params})
    return json.loads(_wrapper.process(request, progress_callback))
