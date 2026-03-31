"""Hello Plugin — minimal script plugin example."""
from __future__ import annotations


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Hello Plugin",
        "description": "Returns a greeting message.",
        "hasUI": False,
    }


def execute(param: dict, progress_callback=None) -> dict:
    """Greet the caller."""
    name = param.get("name", "World")
    return {"ok": True, "message": f"Hello, {name}!"}
