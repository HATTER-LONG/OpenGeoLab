"""Tests for Action Debugger execution and progress state handling."""
from __future__ import annotations

import json
import sys
import time
import unittest
from unittest.mock import patch

from PySide6.QtCore import QCoreApplication

from ai_chat_plugin.debugger_backend import DebuggerBackend


_app = QCoreApplication.instance() or QCoreApplication(sys.argv)


def _make_backend() -> DebuggerBackend:
    with patch("ai_chat_plugin.scene_tools.list_modules", return_value=[]):
        return DebuggerBackend()


def _process_events_until(predicate, timeout: float = 2.0) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        _app.processEvents()
        if predicate():
            return True
        time.sleep(0.005)
    return predicate()


class DebuggerBackendProgressTest(unittest.TestCase):
    def test_invalid_request_json_enters_failed_state(self) -> None:
        backend = _make_backend()
        backend._current_action = "list_shapes"
        backend._request_json = "{invalid"

        backend.execute()

        self.assertEqual(backend.executionState, "failed")
        self.assertFalse(backend.isExecuting)
        self.assertEqual(backend.progress, 0.0)
        self.assertEqual(backend.progressMessage, "Invalid request JSON")
        self.assertIn("Invalid request JSON", backend.responseJson)

    def test_progress_is_clamped_and_stale_updates_are_ignored(self) -> None:
        backend = _make_backend()
        backend._execution_id = 2
        backend._set_execution_state("running")

        backend._onProgress(1, 0.75, "stale")
        self.assertEqual(backend.progress, 0.0)

        backend._onProgress(2, 1.5, "Finishing")
        self.assertEqual(backend.progress, 1.0)
        self.assertEqual(backend.progressMessage, "Finishing")

    def test_failed_result_is_not_reported_as_complete(self) -> None:
        backend = _make_backend()
        backend._execution_id = 3
        backend._set_execution_state("running")
        backend._onProgress(3, 0.4, "Working")

        result = json.dumps({"ok": False, "summary": "Operation failed"})
        backend._onExecuteDone(3, result, False)

        self.assertEqual(backend.executionState, "failed")
        self.assertFalse(backend.isExecuting)
        self.assertEqual(backend.progress, 0.4)
        self.assertEqual(backend.progressMessage, "Operation failed")

    def test_successful_result_reaches_complete_state(self) -> None:
        backend = _make_backend()
        backend._execution_id = 4
        backend._set_execution_state("running")

        backend._onExecuteDone(4, json.dumps({"ok": True}), True)

        self.assertEqual(backend.executionState, "succeeded")
        self.assertEqual(backend.progress, 1.0)
        self.assertEqual(backend.progressMessage, "Done")

    def test_worker_progress_callback_returns_true(self) -> None:
        backend = _make_backend()
        backend._current_module = "geometry"
        backend._current_action = "list_shapes"
        backend._request_json = json.dumps(
            {"module": "geometry", "action": "list_shapes", "param": {}}
        )
        callback_results: list[bool] = []

        def execute_action(module, action, params, progress_callback):
            callback_results.append(progress_callback(0.25, "Listing shapes"))
            return {"ok": True, "shapes": []}

        with patch(
            "ai_chat_plugin.debugger_backend.scene_tools.execute_action",
            side_effect=execute_action,
        ):
            backend.execute()
            completed = _process_events_until(
                lambda: backend.executionState == "succeeded"
            )

        self.assertTrue(completed)
        self.assertEqual(callback_results, [True])
        self.assertEqual(backend.progress, 1.0)

    def test_clear_does_not_mutate_running_execution(self) -> None:
        backend = _make_backend()
        backend._execution_id = 5
        backend._set_execution_state("running")
        backend._progress = 0.5
        backend._progress_message = "Working"

        backend.clear()

        self.assertEqual(backend.executionState, "running")
        self.assertEqual(backend.progress, 0.5)
        self.assertEqual(backend.progressMessage, "Working")


if __name__ == "__main__":
    unittest.main()
