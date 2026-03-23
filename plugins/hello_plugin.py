"""Hello Plugin — pure function plugin example."""
from __future__ import annotations


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Hello Plugin",
        "description": "Returns a greeting message.",
        "hasUI": False,
    }


def execute(param: dict) -> dict:
    """Run the plugin logic and return a result dict."""
    name = param.get("name", "World")
    return {"ok": True, "message": f"Hello, {name}!"}
