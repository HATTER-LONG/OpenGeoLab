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
    """Invoke geometry bounding_box through the full C++ module pipeline."""
    import opengeolab_pywrapper  # type: ignore

    request = json.dumps({
        "module": "geometry",
        "action": "bounding_box",
        "payload": {"pointCount": point_count, "seed": seed},
    })
    return json.loads(opengeolab_pywrapper.process(request))


def _set_points(points: list[dict]) -> dict:
    """Store explicit points in the C++ PointStore via geometry set_points."""
    import opengeolab_pywrapper  # type: ignore

    request = json.dumps({
        "module": "geometry",
        "action": "set_points",
        "payload": {"points": points},
    })
    return json.loads(opengeolab_pywrapper.process(request))


def _get_stored_bbox() -> dict:
    """Read back the bounding box of stored points via geometry get_stored_bbox."""
    import opengeolab_pywrapper  # type: ignore

    request = json.dumps({
        "module": "geometry",
        "action": "get_stored_bbox",
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
    dialog.setMinimumWidth(560)

    root_layout = QtWidgets.QVBoxLayout(dialog)

    # --- Info banner ---
    info_label = QtWidgets.QLabel(
        "PySide6 plugin UI hook reached the running Qt application."
    )
    root_layout.addWidget(info_label)

    # --- Random BBox section ---
    rand_group = QtWidgets.QGroupBox("Random Point Cloud BBox")
    rand_layout = QtWidgets.QVBoxLayout(rand_group)

    rand_param_layout = QtWidgets.QHBoxLayout()
    rand_param_layout.addWidget(QtWidgets.QLabel("Point Count:"))
    point_count_spin = QtWidgets.QSpinBox()
    point_count_spin.setRange(1, 100_000_000)
    point_count_spin.setValue(1_000_000)
    rand_param_layout.addWidget(point_count_spin)
    rand_param_layout.addWidget(QtWidgets.QLabel("Seed:"))
    seed_spin = QtWidgets.QSpinBox()
    seed_spin.setRange(0, 999_999)
    seed_spin.setValue(42)
    rand_param_layout.addWidget(seed_spin)
    rand_layout.addLayout(rand_param_layout)

    btn_random_bbox = QtWidgets.QPushButton("Compute Random BBox")
    rand_layout.addWidget(btn_random_bbox)
    root_layout.addWidget(rand_group)

    # --- Set / Get stored points section ---
    store_group = QtWidgets.QGroupBox("Set & Get Stored Points (write to main process)")
    store_layout = QtWidgets.QVBoxLayout(store_group)

    points_edit = QtWidgets.QPlainTextEdit()
    points_edit.setMaximumHeight(80)
    points_edit.setPlainText(
        '[{"x":1,"y":2,"z":3},{"x":-5,"y":10,"z":0},{"x":100,"y":-50,"z":25}]'
    )
    store_layout.addWidget(QtWidgets.QLabel("Points JSON (array of {x, y, z}):"))
    store_layout.addWidget(points_edit)

    store_btn_layout = QtWidgets.QHBoxLayout()
    btn_set_points = QtWidgets.QPushButton("Set Points → C++")
    btn_get_stored = QtWidgets.QPushButton("Get Stored BBox ← C++")
    store_btn_layout.addWidget(btn_set_points)
    store_btn_layout.addWidget(btn_get_stored)
    store_layout.addLayout(store_btn_layout)
    root_layout.addWidget(store_group)

    # --- Result display ---
    result_text = QtWidgets.QTextEdit()
    result_text.setReadOnly(True)
    result_text.setMinimumHeight(200)
    result_text.setPlaceholderText("Results will appear here.")
    root_layout.addWidget(result_text)

    # --- Close ---
    close_btn = QtWidgets.QPushButton("Close")
    close_btn.clicked.connect(dialog.accept)
    root_layout.addWidget(close_btn)

    # --- Signal handlers ---
    def _display(response: dict) -> None:
        result_text.setPlainText(json.dumps(response, indent=2))

    def on_random_bbox() -> None:
        try:
            _display(_call_geometry(point_count_spin.value(), seed_spin.value()))
        except Exception as exc:
            result_text.setPlainText(f"Error: {exc}")

    def on_set_points() -> None:
        try:
            points = json.loads(points_edit.toPlainText())
            _display(_set_points(points))
        except Exception as exc:
            result_text.setPlainText(f"Error: {exc}")

    def on_get_stored() -> None:
        try:
            _display(_get_stored_bbox())
        except Exception as exc:
            result_text.setPlainText(f"Error: {exc}")

    btn_random_bbox.clicked.connect(on_random_bbox)
    btn_set_points.clicked.connect(on_set_points)
    btn_get_stored.clicked.connect(on_get_stored)

    dialog.exec()

    return {
        "ok": True,
        "message": "PySide6 geometry demo dialog was shown and dismissed by user.",
    }
