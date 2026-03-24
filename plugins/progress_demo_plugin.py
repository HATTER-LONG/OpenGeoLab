"""Progress Demo Plugin — demonstrates progress_callback reporting."""
from __future__ import annotations

import time


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Progress Demo",
        "description": "Simulates a long task with progress reporting.",
        "hasUI": False,
    }


def execute(param: dict, progress_callback=None) -> dict:
    """Simulate a long-running operation with step-by-step progress.

    Parameters
    ----------
    param : dict
        Optional keys:
        - ``steps`` (int): Number of simulation steps (default 10).
        - ``delay`` (float): Seconds to sleep per step (default 0.2).
    progress_callback : callable or None
        If provided, called as ``progress_callback(progress, message)``
        where *progress* is a float in [0, 1] and *message* is a status string.
    """
    steps = int(param.get("steps", 10))
    delay = float(param.get("delay", 0.2))

    for i in range(1, steps + 1):
        time.sleep(delay)
        progress = i / steps
        message = f"Processing step {i}/{steps}"

        if progress_callback is not None:
            progress_callback(progress, message)

    return {
        "ok": True,
        "message": f"Completed {steps} steps.",
        "totalSteps": steps,
        "totalTime": f"{steps * delay:.1f}s",
    }
