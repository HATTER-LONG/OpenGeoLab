from __future__ import annotations


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
        "name": "demo_plugin",
        "namespace": "plugin.demo",
        "actions": ["plugin.demo.echo", "plugins.invoke_ui"],
        "ui": {
            "kind": "pyside6-dialog",
            "entry": "launch_ui",
            "available": pyside_available,
            "version": pyside_version,
        },
        "summary": "Demonstrates Python plugin metadata and optional PySide6 UI activation.",
    }


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

    dialog = QtWidgets.QMessageBox()
    dialog.setWindowTitle("OpenGeoLab Demo Plugin")
    dialog.setText("PySide6 plugin UI hook reached the running Qt application.")
    dialog.setStandardButtons(QtWidgets.QMessageBox.StandardButton.Ok)
    dialog.exec()

    return {
        "ok": True,
        "message": "PySide6 demo dialog was shown and dismissed by user.",
    }
