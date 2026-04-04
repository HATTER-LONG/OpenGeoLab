"""QObject controller bridging Python data layer to QML UI.

Exposes properties and invokable methods that the QML Action Debugger
binds to. Owns the SchemaTreeModel and ParamListModel instances.
"""
from __future__ import annotations

import json
import threading
from pathlib import Path

from PySide6.QtCore import (
    Property,
    QMetaObject,
    QObject,
    Qt,
    Q_ARG,
    Signal,
    Slot,
)

from ai_chat_plugin import scene_tools
from ai_chat_plugin.param_list_model import ParamListModel
from ai_chat_plugin.schema_tree_model import SchemaTreeModel


class DebuggerBackend(QObject):
    """Backend controller for the Action Debugger QML UI."""

    # ── Signals for property change notification ────────────────────────
    currentModuleChanged = Signal()
    currentActionChanged = Signal()
    detailJsonChanged = Signal()
    requestJsonChanged = Signal()
    responseJsonChanged = Signal()
    isExecutingChanged = Signal()
    progressChanged = Signal()
    progressMessageChanged = Signal()
    isDarkChanged = Signal()

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)

        self._current_module = ""
        self._current_action = ""
        self._detail_json = ""
        self._request_json = ""
        self._response_json = ""
        self._is_executing = False
        self._progress = 0.0
        self._progress_message = ""
        self._is_dark = True

        self._schema_tree_model = SchemaTreeModel(self)
        self._schema_tree_model.load_from_scene_tools()

        self._param_list_model = ParamListModel(self)
        self._param_list_model.dataChanged.connect(self._on_params_changed)

        # Resolve icon directory (works in both hosted and standalone).
        icons_dir = Path(__file__).resolve().parent.parent / "_shared" / "icons"
        self._icon_path = icons_dir.as_uri() if icons_dir.exists() else ""

    # ── Read-only model access (exposed as context properties) ──────────

    @Property(QObject, constant=True)
    def schemaTreeModel(self) -> SchemaTreeModel:
        return self._schema_tree_model

    @Property(QObject, constant=True)
    def paramListModel(self) -> ParamListModel:
        return self._param_list_model

    @Property(str, constant=True)
    def iconPath(self) -> str:
        return self._icon_path

    # ── Properties ──────────────────────────────────────────────────────

    def _get_current_module(self) -> str:
        return self._current_module

    currentModule = Property(str, _get_current_module, notify=currentModuleChanged)

    def _get_current_action(self) -> str:
        return self._current_action

    currentAction = Property(str, _get_current_action, notify=currentActionChanged)

    def _get_detail_json(self) -> str:
        return self._detail_json

    detailJson = Property(str, _get_detail_json, notify=detailJsonChanged)

    def _get_request_json(self) -> str:
        return self._request_json

    def _set_request_json(self, value: str) -> None:
        if self._request_json != value:
            self._request_json = value
            self.requestJsonChanged.emit()

    requestJson = Property(str, _get_request_json, _set_request_json, notify=requestJsonChanged)

    def _get_response_json(self) -> str:
        return self._response_json

    responseJson = Property(str, _get_response_json, notify=responseJsonChanged)

    def _get_is_executing(self) -> bool:
        return self._is_executing

    isExecuting = Property(bool, _get_is_executing, notify=isExecutingChanged)

    def _get_progress(self) -> float:
        return self._progress

    progress = Property(float, _get_progress, notify=progressChanged)

    def _get_progress_message(self) -> str:
        return self._progress_message

    progressMessage = Property(str, _get_progress_message, notify=progressMessageChanged)

    def _get_is_dark(self) -> bool:
        return self._is_dark

    isDark = Property(bool, _get_is_dark, notify=isDarkChanged)

    # ── Invokable methods ───────────────────────────────────────────────

    @Slot(str, str)
    def selectAction(self, module: str, action: str) -> None:
        """Update detail panel and param form for the selected action."""
        self._current_module = module
        self.currentModuleChanged.emit()
        self._current_action = action
        self.currentActionChanged.emit()

        if not action:
            # Module-level selection — show module summary.
            detail = scene_tools.describe_module(module)
            self._detail_json = json.dumps(detail, indent=2, ensure_ascii=False) if detail else ""
            self.detailJsonChanged.emit()
            self._param_list_model.load_from_schema({})
            self._request_json = ""
            self.requestJsonChanged.emit()
            return

        schema = scene_tools.describe_action(module, action)
        if schema:
            self._detail_json = json.dumps(schema, indent=2, ensure_ascii=False)
            self.detailJsonChanged.emit()
            self._param_list_model.load_from_schema(schema.get("params", {}))
            self._rebuild_request_json()
        else:
            self._detail_json = json.dumps(
                {"error": f"Action '{action}' not found in '{module}'"},
                indent=2,
            )
            self.detailJsonChanged.emit()
            self._param_list_model.load_from_schema({})
            self._request_json = ""
            self.requestJsonChanged.emit()

    @Slot()
    def execute(self) -> None:
        """Execute the current action using the request JSON."""
        if self._is_executing or not self._current_action:
            return

        text = self._request_json.strip()
        try:
            request = json.loads(text)
        except json.JSONDecodeError:
            self._response_json = json.dumps(
                {"ok": False, "error": "Invalid request JSON"}, indent=2
            )
            self.responseJsonChanged.emit()
            return

        module = request.get("module", self._current_module)
        action = request.get("action", self._current_action)
        params = request.get("param", {})

        self._is_executing = True
        self.isExecutingChanged.emit()
        self._progress = 0.0
        self.progressChanged.emit()
        self._progress_message = ""
        self.progressMessageChanged.emit()
        self._response_json = ""
        self.responseJsonChanged.emit()

        def progress_cb(pct: float, msg: str) -> None:
            QMetaObject.invokeMethod(
                self,
                "_onProgress",
                Qt.ConnectionType.QueuedConnection,
                Q_ARG(float, pct),
                Q_ARG(str, msg),
            )

        def run() -> None:
            try:
                result = scene_tools.execute_action(
                    module, action, params, progress_callback=progress_cb
                )
                out = json.dumps(result, indent=2, ensure_ascii=False)
            except Exception as exc:
                out = json.dumps({"ok": False, "error": str(exc)}, indent=2)
            QMetaObject.invokeMethod(
                self,
                "_onExecuteDone",
                Qt.ConnectionType.QueuedConnection,
                Q_ARG(str, out),
            )

        threading.Thread(target=run, daemon=True).start()

    @Slot()
    def clear(self) -> None:
        """Reset the param form and response."""
        self._param_list_model.clear()
        self._response_json = ""
        self.responseJsonChanged.emit()
        self._progress = 0.0
        self.progressChanged.emit()
        self._progress_message = ""
        self.progressMessageChanged.emit()
        if self._current_action:
            self._rebuild_request_json()

    @Slot()
    def toggleTheme(self) -> None:
        """Flip dark/light mode."""
        self._is_dark = not self._is_dark
        self.isDarkChanged.emit()

    # ── Internal slots (called from worker thread via QueuedConnection) ─

    @Slot(float, str)
    def _onProgress(self, pct: float, msg: str) -> None:
        self._progress = pct
        self.progressChanged.emit()
        self._progress_message = msg
        self.progressMessageChanged.emit()

    @Slot(str)
    def _onExecuteDone(self, result_json: str) -> None:
        self._response_json = result_json
        self.responseJsonChanged.emit()
        self._is_executing = False
        self.isExecutingChanged.emit()
        self._progress = 1.0
        self.progressChanged.emit()
        self._progress_message = "Done"
        self.progressMessageChanged.emit()

    # ── Private helpers ─────────────────────────────────────────────────

    def _on_params_changed(self) -> None:
        """Re-serialise requestJson when param model changes."""
        if self._current_action:
            self._rebuild_request_json()

    def _rebuild_request_json(self) -> None:
        """Build request JSON from current module/action and param values."""
        request = {
            "module": self._current_module,
            "action": self._current_action,
            "param": self._param_list_model.build_params(),
        }
        self._request_json = json.dumps(request, indent=2, ensure_ascii=False)
        self.requestJsonChanged.emit()
