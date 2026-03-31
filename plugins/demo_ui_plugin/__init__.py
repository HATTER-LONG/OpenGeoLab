"""Demo UI Plugin — PySide6 window with Create Box and styled progress."""
from __future__ import annotations

import json
import threading

_active_windows: list = []


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Demo UI",
        "description": "Opens a styled PySide6 window with geometry tools.",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show a non-modal PySide6 window with a Create Box button and progress bar."""
    from PySide6.QtCore import QMetaObject, Qt, Q_ARG, Slot
    from PySide6.QtWidgets import (
        QApplication,
        QFrame,
        QHBoxLayout,
        QLabel,
        QProgressBar,
        QPushButton,
        QVBoxLayout,
        QWidget,
    )

    from demo_ui_plugin.plugin_theme import build_stylesheet, current_theme

    application = QApplication.instance()
    if application is None:
        return {
            "ok": False,
            "message": "No QApplication instance.",
        }

    theme = current_theme()

    class DemoWindow(QWidget):
        """Styled tool window for geometry operations."""

        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("Geometry Tool")
            self.setFixedSize(360, 260)
            self.setAttribute(Qt.WA_DeleteOnClose)  # type: ignore[arg-type]
            self.setStyleSheet(build_stylesheet(theme))

            root = QVBoxLayout(self)
            root.setContentsMargins(16, 16, 16, 16)
            root.setSpacing(12)

            # Title
            title = QLabel("Geometry Tool")
            title.setProperty("role", "title")
            root.addWidget(title)

            # Card frame
            card = QFrame()
            card.setProperty("role", "card")
            card_layout = QVBoxLayout(card)
            card_layout.setContentsMargins(12, 12, 12, 12)
            card_layout.setSpacing(8)

            self._status_label = QLabel("Ready")
            self._status_label.setProperty("role", "secondary")
            card_layout.addWidget(self._status_label)

            self._progress_bar = QProgressBar()
            self._progress_bar.setRange(0, 100)
            self._progress_bar.setValue(0)
            self._progress_bar.setTextVisible(False)
            self._progress_bar.setFixedHeight(4)
            self._progress_bar.setVisible(False)
            card_layout.addWidget(self._progress_bar)

            self._percent_label = QLabel("")
            self._percent_label.setProperty("role", "secondary")
            self._percent_label.setVisible(False)
            card_layout.addWidget(self._percent_label)

            root.addWidget(card)

            # Buttons
            btn_row = QHBoxLayout()
            btn_row.setSpacing(8)

            self._create_btn = QPushButton("Create Box")
            self._create_btn.clicked.connect(self._on_create_box)
            btn_row.addWidget(self._create_btn)

            close_btn = QPushButton("Close")
            close_btn.setProperty("role", "secondary")
            close_btn.clicked.connect(self.close)
            btn_row.addWidget(close_btn)

            root.addStretch()
            root.addLayout(btn_row)

        def _on_create_box(self) -> None:
            """Dispatch create_box to the C++ geometry module on a worker thread."""
            self._create_btn.setEnabled(False)
            self._status_label.setText("Creating box…")
            self._progress_bar.setValue(0)
            self._progress_bar.setVisible(True)
            self._percent_label.setText("0%")
            self._percent_label.setVisible(True)

            def progress_callback(progress: float, message: str) -> None:
                pct = int(progress * 100)
                QMetaObject.invokeMethod(
                    self,
                    "_update_progress",
                    Qt.QueuedConnection,
                    Q_ARG(int, pct),
                    Q_ARG(str, message),
                )

            def run() -> None:
                try:
                    import opengeolab_pywrapper as wrapper

                    request = json.dumps(
                        {
                            "module": "geometry",
                            "action": "create_box",
                            "mute": True,
                            "param": {
                                "center": [0, 0, 0],
                                "size": [2, 2, 2],
                                "vertexCount": 20,
                            },
                        }
                    )
                    response_str = wrapper.process(request, progress_callback)
                    response = json.loads(response_str)
                    if response.get("ok"):
                        result = response.get("result", {})
                        label = result.get("label", "Box")
                        msg = f"✓ Created: {label}"
                    else:
                        msg = f"✗ Error: {response.get('error', 'unknown')}"
                except Exception as exc:
                    msg = f"✗ Error: {exc}"

                QMetaObject.invokeMethod(
                    self,
                    "_on_finished",
                    Qt.QueuedConnection,
                    Q_ARG(str, msg),
                )

            threading.Thread(target=run, daemon=True).start()

        @Slot(int, str)
        def _update_progress(self, percent: int, message: str) -> None:
            self._progress_bar.setValue(percent)
            self._percent_label.setText(f"{percent}%")
            self._status_label.setText(message if message else "Processing…")

        @Slot(str)
        def _on_finished(self, message: str) -> None:
            self._status_label.setText(message)
            self._progress_bar.setValue(100)
            self._percent_label.setText("100%")
            self._create_btn.setEnabled(True)

    window = DemoWindow()
    window.destroyed.connect(lambda: _active_windows.remove(window))
    _active_windows.append(window)
    window.show()
    return {"ok": True, "message": "Geometry Tool launched."}
