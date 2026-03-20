from __future__ import annotations

import json


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


def _call_geometry(point_count: int = 1_000_000, seed: int = 42) -> dict:
    """Invoke geometry.bounding_box through the full C++ command pipeline."""
    import opengeolab_pywrapper  # type: ignore

    request = json.dumps({
        "action": "geometry.bounding_box",
        "payload": {"pointCount": point_count, "seed": seed},
    })
    return json.loads(opengeolab_pywrapper.process(request))


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

    dialog = QtWidgets.QDialog()
    dialog.setWindowTitle("OpenGeoLab Demo Plugin")
    dialog.setMinimumWidth(520)

    root_layout = QtWidgets.QVBoxLayout(dialog)

    # --- Info banner ---
    info_label = QtWidgets.QLabel(
        "PySide6 plugin UI hook reached the running Qt application."
    )
    root_layout.addWidget(info_label)

    # --- Geometry pipeline test ---
    geo_group = QtWidgets.QGroupBox("Geometry Pipeline Test")
    geo_layout = QtWidgets.QVBoxLayout(geo_group)

    # Parameter inputs
    param_layout = QtWidgets.QHBoxLayout()
    param_layout.addWidget(QtWidgets.QLabel("Point Count:"))
    point_count_spin = QtWidgets.QSpinBox()
    point_count_spin.setRange(1, 100_000_000)
    point_count_spin.setValue(1_000_000)
    param_layout.addWidget(point_count_spin)

    param_layout.addWidget(QtWidgets.QLabel("Seed:"))
    seed_spin = QtWidgets.QSpinBox()
    seed_spin.setRange(0, 999_999)
    seed_spin.setValue(42)
    param_layout.addWidget(seed_spin)
    geo_layout.addLayout(param_layout)

    # Action buttons
    btn_layout = QtWidgets.QHBoxLayout()
    btn_default = QtWidgets.QPushButton("Get BBox (Default)")
    btn_custom = QtWidgets.QPushButton("Get BBox (Custom)")
    btn_layout.addWidget(btn_default)
    btn_layout.addWidget(btn_custom)
    geo_layout.addLayout(btn_layout)

    # Result display
    result_text = QtWidgets.QTextEdit()
    result_text.setReadOnly(True)
    result_text.setMinimumHeight(220)
    result_text.setPlaceholderText("Bounding box results will appear here.")
    geo_layout.addWidget(result_text)

    root_layout.addWidget(geo_group)

    # --- Close ---
    close_btn = QtWidgets.QPushButton("Close")
    close_btn.clicked.connect(dialog.accept)
    root_layout.addWidget(close_btn)

    # --- Signal handlers ---
    def _run_and_display(point_count: int, seed: int) -> None:
        try:
            response = _call_geometry(point_count, seed)
            result_text.setPlainText(json.dumps(response, indent=2))
        except Exception as exc:
            result_text.setPlainText(f"Error: {exc}")

    btn_default.clicked.connect(lambda: _run_and_display(1_000_000, 42))
    btn_custom.clicked.connect(
        lambda: _run_and_display(point_count_spin.value(), seed_spin.value())
    )

    dialog.exec()

    return {
        "ok": True,
        "message": "PySide6 geometry demo dialog was shown and dismissed by user.",
    }
