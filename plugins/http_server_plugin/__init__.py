"""HTTP Server Plugin — local REST API for OpenGeoLab actions.

Provides a management panel to start/stop an HTTP server that forwards
JSON requests to ``opengeolab_runtime.process()``.
"""
from __future__ import annotations

_active_engines: list = []


def describe_plugin() -> dict:
    """Return plugin metadata for the runtime discovery system."""
    return {
        "name": "HTTP Server",
        "description": "Local REST API for external action execution.",
        "hasUI": True,
    }


def launch_ui(param: dict | None = None) -> dict:
    """Show the HTTP Server management panel.

    Parameters
    ----------
    param:
        Optional configuration dict.  Recognised keys:

        * ``autoStart`` (*bool*) – when *True*, the HTTP server is started
          automatically after the panel is created (e.g. triggered by the
          ``--start-http-server`` CLI flag).
    """
    from pathlib import Path

    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtQuickControls2 import QQuickStyle
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    QQuickStyle.setStyle("Basic")

    from http_server_plugin.server_backend import ServerBackend

    backend = ServerBackend()
    engine = QQmlApplicationEngine()

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.setInitialProperties({"backend": backend})
    engine.load(QUrl.fromLocalFile(str(qml_dir / "ServerWindow.qml")))

    if not engine.rootObjects():
        return {"ok": False, "message": "QML failed to load."}

    # Prevent garbage collection of backend and engine.
    engine._backend = backend  # noqa: SLF001
    _active_engines.append(engine)

    # Stop server gracefully on app exit.
    application.aboutToQuit.connect(backend.stop)

    # Auto-start the server when requested (e.g. via --start-http-server).
    if bool((param or {}).get("autoStart", False)):
        backend.start()

    return {"ok": True, "message": "HTTP Server panel launched."}
