"""AI Chat Plugin — Chat with AI powered by GitHub Copilot SDK.

Segment 1 provides an Action Debugger window for browsing and executing
OpenGeoLab commands. Full chat functionality is added in Segment 2.
"""
from __future__ import annotations

_active_engines: list = []


def describe_plugin() -> dict:
    """Return plugin metadata for the runtime discovery system."""
    return {
        "name": "AI Chat",
        "description": "Chat with AI powered by GitHub Copilot SDK.",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show the AI Chat plugin window (Action Debugger in Segment 1)."""
    import sys
    from pathlib import Path

    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin._qml_setup import setup_engine

    backend = DebuggerBackend()
    engine = QQmlApplicationEngine()

    engine.rootContext().setContextProperty("backend", backend)

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.load(QUrl.fromLocalFile(str(qml_dir / "ActionDebuggerWindow.qml")))

    if not engine.rootObjects():
        return {"ok": False, "message": "QML failed to load."}

    setup_engine(engine, backend)

    # prevent GC
    engine._backend = backend
    _active_engines.append(engine)

    # Clean up when window closes.
    window = engine.rootObjects()[0]
    window.closing.connect(lambda: _active_engines.remove(engine))

    return {"ok": True, "message": "AI Chat (Action Debugger) launched."}
