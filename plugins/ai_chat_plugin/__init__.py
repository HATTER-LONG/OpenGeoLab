"""AI Chat Plugin — Chat with AI powered by GitHub Copilot SDK.

Segment 1 provides an Action Debugger window for browsing and executing
OpenGeoLab commands. Full chat functionality is added in Segment 2.
"""
from __future__ import annotations

_active_windows: list = []


def describe_plugin() -> dict:
    """Return plugin metadata for the runtime discovery system."""
    return {
        "name": "AI Chat",
        "description": "Chat with AI powered by GitHub Copilot SDK.",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show the AI Chat plugin window (Action Debugger in Segment 1)."""
    from PySide6.QtCore import Qt
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {
            "ok": False,
            "message": "No QApplication instance.",
        }

    from ai_chat_plugin.action_debugger import ActionDebuggerWindow

    window = ActionDebuggerWindow(embedded=True)
    window.setAttribute(Qt.WA_DeleteOnClose)
    window.destroyed.connect(lambda: _active_windows.remove(window))
    _active_windows.append(window)
    window.show()
    return {"ok": True, "message": "AI Chat (Action Debugger) launched."}
