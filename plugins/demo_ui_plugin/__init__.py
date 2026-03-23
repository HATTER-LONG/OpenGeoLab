"""Demo UI Plugin — PySide6 non-modal window example."""
from __future__ import annotations

_active_windows: list = []


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Demo UI",
        "description": "Opens a PySide6 window (non-modal).",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show a non-modal PySide6 window. Returns immediately."""
    from PySide6.QtCore import Qt
    from PySide6.QtWidgets import QApplication, QLabel, QPushButton, QVBoxLayout, QWidget

    application = QApplication.instance()
    if application is None:
        return {
            "ok": False,
            "message": "No QApplication instance — PySide6 requires the C++ host app "
                       "(run with RelWithDebInfo, not Debug).",
        }

    window = QWidget()
    window.setWindowTitle("Demo UI Plugin")
    window.setMinimumSize(320, 200)
    window.setAttribute(Qt.WA_DeleteOnClose)  # type: ignore[arg-type]
    window.destroyed.connect(lambda: _active_windows.remove(window))
    _active_windows.append(window)

    layout = QVBoxLayout(window)
    layout.addWidget(QLabel("Hello from PySide6 Plugin!"))
    btn = QPushButton("Close")
    btn.clicked.connect(window.close)
    layout.addWidget(btn)

    window.show()
    return {"ok": True, "message": "Demo UI launched."}
