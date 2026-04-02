"""Selection Demo Plugin — Create geometry, select entities, control camera, and pick areas.

Demonstrates programmatic interaction through the scene module's JSON actions:
- scene.select:           add entities to selection by {shapeId, type, localId}
- scene.deselect:         remove entities from selection
- scene.clear_selection:  clear all selections
- scene.query_selection:  list currently selected entities
- scene.set_pick_mode:    enable/disable interactive picking
- scene.fit_to_scene:     fit camera to show entire scene
- scene.set_view_preset:  apply a named camera preset (Front, Top, …)
- scene.set_camera:       set camera position / target / up explicitly
- scene.pick_area:        select entities inside a 2-D screen rectangle
"""
from __future__ import annotations

import json
import threading

_active_windows: list = []


def describe_plugin() -> dict:
    """Return plugin metadata."""
    return {
        "name": "Selection Demo",
        "description": "Create a box, select entities, control camera, and pick areas via Python.",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show a PySide6 window demonstrating programmatic entity selection."""
    from PySide6.QtCore import QMetaObject, Qt, QTimer, Q_ARG, Slot
    from PySide6.QtWidgets import (
        QApplication,
        QComboBox,
        QFrame,
        QHBoxLayout,
        QLabel,
        QPushButton,
        QVBoxLayout,
        QWidget,
    )

    # Import shared theme from sibling plugin.
    from demo_ui_plugin.plugin_theme import build_stylesheet, current_theme

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    theme = current_theme()

    combo_style = (
        f"QComboBox {{ background: {theme.surface_strong}; "
        f"border: 1px solid {theme.border}; border-radius: 4px; "
        f"padding: 4px 8px; color: {theme.text_primary}; }}"
    )

    class SelectionDemoWindow(QWidget):
        """Tool window for create-and-select workflow."""

        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("Selection Demo")
            self.setFixedSize(420, 500)
            self.setAttribute(Qt.WA_DeleteOnClose)
            self.setStyleSheet(build_stylesheet(theme))
            self._shape_id: int | None = None

            root = QVBoxLayout(self)
            root.setContentsMargins(16, 16, 16, 16)
            root.setSpacing(10)

            # Title
            title = QLabel("Selection Demo")
            title.setProperty("role", "title")
            root.addWidget(title)

            # Status card
            card = QFrame()
            card.setProperty("role", "card")
            card_layout = QVBoxLayout(card)
            card_layout.setContentsMargins(12, 12, 12, 12)
            card_layout.setSpacing(6)
            self._status = QLabel("Ready — create a box first.")
            self._status.setProperty("role", "secondary")
            self._status.setWordWrap(True)
            card_layout.addWidget(self._status)
            root.addWidget(card)

            # Entity type selector
            type_row = QHBoxLayout()
            type_row.setSpacing(8)
            type_label = QLabel("Entity type:")
            type_label.setProperty("role", "secondary")
            type_row.addWidget(type_label)
            self._type_combo = QComboBox()
            self._type_combo.addItems(["GeoSolid", "GeoFace", "GeoEdge", "GeoVertex"])
            self._type_combo.setStyleSheet(combo_style)
            type_row.addWidget(self._type_combo)
            type_row.addStretch()
            root.addLayout(type_row)

            # Selection buttons
            btn_row = QHBoxLayout()
            btn_row.setSpacing(8)

            self._create_btn = QPushButton("Create Box")
            self._create_btn.clicked.connect(self._on_create)
            btn_row.addWidget(self._create_btn)

            self._select_btn = QPushButton("Select")
            self._select_btn.setEnabled(False)
            self._select_btn.clicked.connect(self._on_select)
            btn_row.addWidget(self._select_btn)

            self._clear_btn = QPushButton("Clear")
            self._clear_btn.setProperty("role", "secondary")
            self._clear_btn.clicked.connect(self._on_clear)
            btn_row.addWidget(self._clear_btn)

            root.addLayout(btn_row)

            # ── Camera & Pick section ───────────────────────────────────
            cam_card = QFrame()
            cam_card.setProperty("role", "card")
            cam_layout = QVBoxLayout(cam_card)
            cam_layout.setContentsMargins(12, 12, 12, 12)
            cam_layout.setSpacing(8)

            cam_title = QLabel("Camera & Pick")
            cam_title.setProperty("role", "secondary")
            cam_layout.addWidget(cam_title)

            # View preset row
            view_row = QHBoxLayout()
            view_row.setSpacing(8)
            view_label = QLabel("View:")
            view_label.setProperty("role", "secondary")
            view_row.addWidget(view_label)
            self._view_combo = QComboBox()
            self._view_combo.addItems(
                ["Front", "Back", "Top", "Bottom", "Left", "Right", "Isometric"]
            )
            self._view_combo.setStyleSheet(combo_style)
            view_row.addWidget(self._view_combo)
            apply_btn = QPushButton("Apply")
            apply_btn.clicked.connect(self._on_apply_view)
            view_row.addWidget(apply_btn)
            view_row.addStretch()
            cam_layout.addLayout(view_row)

            # Action buttons row
            cam_btn_row = QHBoxLayout()
            cam_btn_row.setSpacing(8)

            fit_btn = QPushButton("Fit to Scene")
            fit_btn.clicked.connect(self._on_fit_to_scene)
            cam_btn_row.addWidget(fit_btn)

            set_cam_btn = QPushButton("Set Camera")
            set_cam_btn.clicked.connect(self._on_set_camera)
            cam_btn_row.addWidget(set_cam_btn)

            pick_btn = QPushButton("Pick Area (0.2-0.8)")
            pick_btn.clicked.connect(self._on_pick_area)
            cam_btn_row.addWidget(pick_btn)

            cam_layout.addLayout(cam_btn_row)

            # Camera/pick status label
            self._cam_status = QLabel("")
            self._cam_status.setProperty("role", "secondary")
            self._cam_status.setWordWrap(True)
            cam_layout.addWidget(self._cam_status)

            root.addWidget(cam_card)
            root.addStretch()

        # ── helpers ─────────────────────────────────────────────────────

        def _dispatch(self, request: dict) -> dict:
            """Synchronously dispatch a JSON request to the C++ backend."""
            import opengeolab_pywrapper as wrapper

            response_str = wrapper.process(json.dumps(request), None)
            return json.loads(response_str)

        def _dispatch_async(
            self,
            request: dict,
            on_done: str,
        ) -> None:
            """Dispatch on a worker thread, invoke on_done slot with result JSON."""

            def run() -> None:
                try:
                    result = self._dispatch(request)
                    msg = json.dumps(result)
                except Exception as exc:
                    msg = json.dumps({"ok": False, "error": str(exc)})
                QMetaObject.invokeMethod(
                    self, on_done, Qt.QueuedConnection, Q_ARG(str, msg)
                )

            threading.Thread(target=run, daemon=True).start()

        # ── Create Box ──────────────────────────────────────────────────

        def _on_create(self) -> None:
            self._create_btn.setEnabled(False)
            self._status.setText("Creating box…")
            self._dispatch_async(
                {
                    "module": "geometry",
                    "action": "create_box",
                    "mute": True,
                    "param": {
                        "center": [0, 0, 0],
                        "size": [2, 2, 2],
                        "vertexCount": 20,
                    },
                },
                "_on_create_done",
            )

        @Slot(str)
        def _on_create_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            self._create_btn.setEnabled(True)
            if result.get("ok"):
                self._shape_id = result.get("shapeId")
                topo = result.get("topology", {})
                self._status.setText(
                    f"✓ Box created (shapeId={self._shape_id})  "
                    f"S:{topo.get('solids',0)} F:{topo.get('faces',0)} "
                    f"E:{topo.get('edges',0)} V:{topo.get('vertices',0)}"
                )
                self._select_btn.setEnabled(True)
                # Auto fit-to-scene after creating geometry.
                self._dispatch_async(
                    {"module": "scene", "action": "fit_to_scene", "param": {}},
                    "_on_fit_done",
                )
            else:
                self._status.setText(f"✗ {result.get('error', 'unknown')}")

        # ── Select entity ───────────────────────────────────────────────

        def _on_select(self) -> None:
            if self._shape_id is None:
                return
            entity_type = self._type_combo.currentText()
            self._status.setText(f"Selecting {entity_type} on shape {self._shape_id}…")
            self._dispatch_async(
                {
                    "module": "scene",
                    "action": "select",
                    "param": {
                        "append": False,
                        "entities": [
                            {
                                "shapeId": self._shape_id,
                                "type": entity_type,
                                "localId": 1,
                            }
                        ],
                    },
                },
                "_on_select_done",
            )

        @Slot(str)
        def _on_select_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                count = result.get("selected", 0)
                self._status.setText(f"✓ Selected {count} entity.")
            else:
                self._status.setText(f"✗ {result.get('error', 'unknown')}")

        # ── Clear selection ─────────────────────────────────────────────

        def _on_clear(self) -> None:
            self._dispatch_async(
                {"module": "scene", "action": "clear_selection", "param": {}},
                "_on_clear_done",
            )

        @Slot(str)
        def _on_clear_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                self._status.setText("Selection cleared.")
            else:
                self._status.setText(f"✗ {result.get('error', 'unknown')}")

        # ── Fit to Scene ────────────────────────────────────────────────

        def _on_fit_to_scene(self) -> None:
            self._cam_status.setText("Fitting to scene…")
            self._dispatch_async(
                {"module": "scene", "action": "fit_to_scene", "param": {}},
                "_on_fit_done",
            )

        @Slot(str)
        def _on_fit_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                self._cam_status.setText("✓ Fit to scene applied.")
            else:
                self._cam_status.setText(f"✗ {result.get('error', 'unknown')}")

        # ── View Preset ─────────────────────────────────────────────────

        def _on_apply_view(self) -> None:
            preset = self._view_combo.currentText()
            self._cam_status.setText(f"Applying view: {preset}…")
            self._dispatch_async(
                {
                    "module": "scene",
                    "action": "set_view_preset",
                    "param": {"preset": preset},
                },
                "_on_view_done",
            )

        @Slot(str)
        def _on_view_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                self._cam_status.setText(f"✓ View preset applied.")
            else:
                self._cam_status.setText(f"✗ {result.get('error', 'unknown')}")

        # ── Set Camera ──────────────────────────────────────────────────

        def _on_set_camera(self) -> None:
            self._cam_status.setText("Setting camera…")
            self._dispatch_async(
                {
                    "module": "scene",
                    "action": "set_camera",
                    "param": {
                        "position": [5.0, 5.0, 5.0],
                        "target": [0.0, 0.0, 0.0],
                        "up": [0.0, 1.0, 0.0],
                    },
                },
                "_on_set_camera_done",
            )

        @Slot(str)
        def _on_set_camera_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                self._cam_status.setText("✓ Camera set to (5,5,5) → origin.")
            else:
                self._cam_status.setText(f"✗ {result.get('error', 'unknown')}")

        # ── Pick Area ───────────────────────────────────────────────────

        def _on_pick_area(self) -> None:
            self._cam_status.setText("Picking area (0.2–0.8)…")
            self._dispatch_async(
                {
                    "module": "scene",
                    "action": "pick_area",
                    "param": {
                        "x0": 0.2,
                        "y0": 0.2,
                        "x1": 0.8,
                        "y1": 0.8,
                        "coordType": "normalized",
                        "pickAction": "Add",
                    },
                },
                "_on_pick_area_done",
            )

        @Slot(str)
        def _on_pick_area_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                self._cam_status.setText("✓ Pick area sent. Querying selection…")
                QTimer.singleShot(500, self._query_selection_after_pick)
            else:
                self._cam_status.setText(f"✗ {result.get('error', 'unknown')}")

        def _query_selection_after_pick(self) -> None:
            """Query selection results after a short delay following pick_area."""
            self._dispatch_async(
                {"module": "scene", "action": "query_selection", "param": {}},
                "_on_query_selection_done",
            )

        @Slot(str)
        def _on_query_selection_done(self, result_json: str) -> None:
            result = json.loads(result_json)
            if result.get("ok"):
                entities = result.get("entities", [])
                self._cam_status.setText(
                    f"✓ Pick complete — {len(entities)} entity(ies) selected."
                )
            else:
                self._cam_status.setText(f"✗ {result.get('error', 'unknown')}")

    window = SelectionDemoWindow()
    window.destroyed.connect(lambda: _active_windows.remove(window))
    _active_windows.append(window)
    window.show()
    return {"ok": True, "message": "Selection Demo launched."}
