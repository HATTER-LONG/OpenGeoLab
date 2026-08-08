"""AI Chat Plugin — Chat with AI powered by GitHub Copilot SDK.

Provides a tab-merged window with Chat (Copilot SDK conversation) and
Action Debugger (command browser/executor).
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
    """Show the AI Chat plugin window (Chat + Action Debugger tabs)."""
    from pathlib import Path

    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    from ai_chat_plugin.chat_config import ChatConfig
    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine

    backend = DebuggerBackend()
    config = ChatConfig()
    chat_backend = ChatBackend(config=config)
    engine = QQmlApplicationEngine()

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.setInitialProperties({
        "backend": backend,
        "chatBackend": chat_backend,
    })
    engine.load(QUrl.fromLocalFile(str(qml_dir / "PluginWindow.qml")))

    if not engine.rootObjects():
        return {"ok": False, "message": "QML failed to load."}

    setup_engine(engine, backend)

    engine._backend = backend
    engine._config = config
    engine._chat_backend = chat_backend
    _active_engines.append(engine)

    # Gracefully stop worker threads on app exit to avoid shutdown crashes
    application.aboutToQuit.connect(chat_backend.shutdown)

    return {"ok": True, "message": "AI Chat launched."}
