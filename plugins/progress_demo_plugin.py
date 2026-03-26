"""Progress Demo Plugin — demonstrates progress_callback support."""
from __future__ import annotations

import time


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Progress Demo",
        "description": "Simulates a long-running task with progress updates.",
        "hasUI": False,
    }


def execute(param: dict, progress_callback=None) -> dict:
    """Run a configurable number of steps, reporting progress."""
    steps = int(param.get("steps", 5))
    delay = float(param.get("delay", 0.5))

    for i in range(1, steps + 1):
        time.sleep(delay)
        progress = i / steps
        message = f"Step {i}/{steps}"
        if progress_callback is not None:
            progress_callback(progress, message)

    return {
        "ok": True,
        "message": f"Completed {steps} steps.",
    }
