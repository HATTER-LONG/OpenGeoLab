from __future__ import annotations

import json
from pathlib import Path

QML_DIALOG_FILENAME = "QmlDemoDialog.qml"


def describe_plugin() -> dict:
    try:
        import PySide6  # type: ignore
    except ModuleNotFoundError:
        pyside_available = False
        pyside_version = ""
    else:
        pyside_available = True
        pyside_version = getattr(PySide6, "__version__", "unknown")

    return {
        "name": "qml_demo_plugin",
        "namespace": "plugin.qml_demo",
        "actions": ["plugins.invoke_ui"],
        "ui": {
            "kind": "pyside6-qml",
            "entry": "launch_ui",
            "available": pyside_available,
            "version": pyside_version,
        },
        "summary": "Demonstrates QML-based plugin UI with PySide6 and QQmlApplicationEngine.",
    }


def _bridge_type():
    from PySide6.QtCore import QObject, Signal, Slot  # type: ignore

    class _Bridge(QObject):
        responseReady = Signal(str)

        @Slot(str)
        def process_request(self, request_json: str) -> None:
            """Handle a QML request and emit the JSON response string."""
            import opengeolab_pywrapper  # type: ignore

            try:
                result = opengeolab_pywrapper.process(request_json)
                self.responseReady.emit(result)
            except Exception as exc:
                error_response = json.dumps({"ok": False, "message": str(exc)})
                self.responseReady.emit(error_response)

    return _Bridge


def launch_ui() -> dict:
    try:
        from PySide6 import QtWidgets  # type: ignore
    except ModuleNotFoundError:
        return {
            "ok": False,
            "message": "PySide6 is not installed in the active Python environment.",
        }

    application = QtWidgets.QApplication.instance()
    if application is None:
        return {
            "ok": False,
            "message": "No QApplication instance is available for the PySide6 plugin dialog.",
        }

    try:
        from PySide6 import QtQuick  # noqa: F401  # type: ignore
    except ModuleNotFoundError:
        return {
            "ok": False,
            "message": "PySide6.QtQuick is not available in the active Python environment.",
        }

    engine = None
    try:
        from PySide6.QtCore import QEventLoop, QUrl  # type: ignore
        from PySide6.QtQml import QQmlApplicationEngine  # type: ignore

        bridge = _bridge_type()()
        engine = QQmlApplicationEngine()
        engine.rootContext().setContextProperty("bridge", bridge)

        qml_path = Path(__file__).resolve().parent / QML_DIALOG_FILENAME
        engine.load(QUrl.fromLocalFile(str(qml_path)))
        if not engine.rootObjects():
            return {
                "ok": False,
                "message": f"Failed to load QML demo dialog from '{qml_path}'.",
            }

        root_window = engine.rootObjects()[0]
        loop = QEventLoop()
        root_window.closing.connect(loop.quit)
        loop.exec()

        return {
            "ok": True,
            "message": "QML demo plugin dialog was shown and dismissed by user.",
        }
    except Exception as exc:
        return {"ok": False, "message": str(exc)}
    finally:
        if engine is not None:
            del engine
        if "bridge" in dir():
            del bridge
