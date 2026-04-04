# Action Debugger QML Rewrite — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Replace the PySide6 widget-based Action Debugger UI with a QML interface backed by Python QObject controllers, producing visual parity with the main application.

**Architecture:** Python layer (`DebuggerBackend` + `ParamListModel`) exposes data and actions via `Q_PROPERTY` / `Q_INVOKABLE`. QML layer renders UI with `PluginTheme` singleton for theming. `JsonHighlighter` is attached to QML `TextArea` documents from Python after QML loads.

**Tech Stack:** PySide6 6.x (QML engine, QObject, QAbstractListModel), Qt Quick Controls, Python 3.11+

**Spec:** `docs/superpowers/specs/2026-04-04-action-debugger-qml-rewrite.md`

---

## File Map

| File | Action | Responsibility |
|------|--------|----------------|
| `plugins/ai_chat_plugin/qml/theme/PluginTheme.qml` | Create | Singleton color palette (dark/light) |
| `plugins/ai_chat_plugin/qml/theme/qmldir` | Create | Module registration for singleton |
| `plugins/ai_chat_plugin/param_list_model.py` | Create | `QAbstractListModel` for action params |
| `plugins/ai_chat_plugin/debugger_backend.py` | Create | `QObject` controller bridging Python↔QML |
| `plugins/ai_chat_plugin/qml/SchemaTreeView.qml` | Create | Left panel tree component |
| `plugins/ai_chat_plugin/qml/DetailPanel.qml` | Create | JSON schema display panel |
| `plugins/ai_chat_plugin/qml/ParamForm.qml` | Create | Dynamic param form with type delegates |
| `plugins/ai_chat_plugin/qml/RequestResponseView.qml` | Create | Request/response split editors |
| `plugins/ai_chat_plugin/qml/ActionDebuggerPage.qml` | Create | Root embeddable layout |
| `plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml` | Create | Standalone window wrapper |
| `plugins/ai_chat_plugin/__init__.py` | Modify | Update `launch_ui()` for QML engine |
| `plugins/ai_chat_plugin/__main__.py` | Modify | Update `main()` for QML engine |
| `plugins/ai_chat_plugin/action_debugger.py` | Delete | Replaced by `debugger_backend.py` + QML |
| `plugins/ai_chat_plugin/param_form_builder.py` | Delete | Replaced by `param_list_model.py` + QML |

---

## Pre-requisite: Commit Uncommitted Foundation Changes

Before starting the QML rewrite tasks, the agent must first commit the
uncommitted changes that the QML rewrite depends on. These are working changes
from the previous PySide6 iteration that provide needed infrastructure.

**Commit 1 — Icons and shared theme enhancement:**

```bash
git add plugins/_shared/icons/ plugins/_shared/plugin_theme.py
git commit -m "feat(plugins): add shared SVG icons and enhance plugin_theme

Add 21 SVG icons to plugins/_shared/icons/ for use by all plugins.
Enhance plugin_theme.py with comprehensive stylesheet covering
QTreeView, QTextEdit, QSplitter, QScrollBar, QCheckBox, QLineEdit,
QSpinBox and other widgets.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

**Commit 2 — json_highlighter set_theme + standalone pywrapper fix:**

```bash
git add plugins/ai_chat_plugin/json_highlighter.py plugins/ai_chat_plugin/__main__.py
git commit -m "fix(ai-chat): add set_theme to json_highlighter and fix standalone pywrapper loading

Add set_theme() method to JsonHighlighter for runtime theme switching.
Fix standalone pywrapper loading on Python 3.13/Windows: preload
opengeolab_command.dll via ctypes before importing pybind11 module.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

**Commit 3 — Main.qml aiChat handler:**

```bash
git add src/app/resource/qml/Main.qml
git commit -m "feat(app): add aiChat handler in Main.qml openActionPage

Route 'aiChat' page type to the AI Chat plugin launch via
RequestService.executeOnMainThread.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

**Do NOT commit** the uncommitted changes to `action_debugger.py` and
`param_form_builder.py` — those files will be deleted and replaced by the QML
rewrite. Discard their working-tree changes after the prerequisite commits:

```bash
git checkout -- plugins/ai_chat_plugin/action_debugger.py plugins/ai_chat_plugin/param_form_builder.py
```

**Do NOT commit** `docs/superpowers/specs/2026-04-03-ai-chat-plugin-design.md` — it is
a pre-existing modified file that must stay uncommitted.

---

### Task 1: PluginTheme QML Singleton

**Files:**
- Create: `plugins/ai_chat_plugin/qml/theme/PluginTheme.qml`
- Create: `plugins/ai_chat_plugin/qml/theme/qmldir`

- [ ] **Step 1: Create theme directory**

```bash
mkdir -p plugins/ai_chat_plugin/qml/theme
```

- [ ] **Step 2: Create PluginTheme.qml**

Create `plugins/ai_chat_plugin/qml/theme/PluginTheme.qml`:

```qml
pragma Singleton

import QtQuick

QtObject {
    id: root

    property bool darkMode: true

    // ── Backgrounds ────────────────────────────────────────────────────
    readonly property color bg: darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color surface: darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted: darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong: darkMode ? "#1d2630" : "#dfe9f4"

    // ── Text ───────────────────────────────────────────────────────────
    readonly property color textPrimary: darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary: darkMode ? "#a0acb9" : "#60748b"
    readonly property color textTertiary: darkMode ? "#7d8997" : "#8397ac"
    readonly property color textOnAccent: "#ffffff"

    // ── Accents ────────────────────────────────────────────────────────
    readonly property color accentA: darkMode ? "#5aa2ff" : "#1473e6"
    readonly property color accentB: darkMode ? "#85c0ff" : "#14ae8a"

    // ── Semantic ───────────────────────────────────────────────────────
    readonly property color success: darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color warning: darkMode ? "#ffd071" : "#d89209"
    readonly property color danger: darkMode ? "#ff8d7d" : "#d9534f"

    // ── Border ─────────────────────────────────────────────────────────
    readonly property color borderSubtle: darkMode ? "#27313c" : "#d6e0eb"

    // ── Spacing ────────────────────────────────────────────────────────
    readonly property int gapTight: 8
    readonly property int gap: 12
    readonly property int gapWide: 16

    // ── Radii ──────────────────────────────────────────────────────────
    readonly property int radiusSmall: 12
    readonly property int radiusMedium: 18

    // ── Typography ─────────────────────────────────────────────────────
    readonly property string monoFont: "Consolas"

    // ── Animation ──────────────────────────────────────────────────────
    readonly property int animFast: 120
    readonly property int animNormal: 140

    /// Return @p c with alpha replaced by @p a.
    function tint(c: color, a: real): color {
        return Qt.rgba(c.r, c.g, c.b, a);
    }
}
```

- [ ] **Step 3: Create qmldir**

Create `plugins/ai_chat_plugin/qml/theme/qmldir`:

```
singleton PluginTheme 1.0 PluginTheme.qml
```

- [ ] **Step 4: Commit**

```bash
git add plugins/ai_chat_plugin/qml/theme/PluginTheme.qml plugins/ai_chat_plugin/qml/theme/qmldir
git commit -m "feat(ai-chat): add PluginTheme QML singleton

Simplified subset of AppTheme.qml for plugin use. Provides
dark/light color palette, spacing, radii and tint helper.
Registered as a singleton via qmldir.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: ParamListModel

**Files:**
- Create: `plugins/ai_chat_plugin/param_list_model.py`

- [ ] **Step 1: Create param_list_model.py**

Create `plugins/ai_chat_plugin/param_list_model.py`:

```python
"""QAbstractListModel exposing action parameters for QML consumption.

Each row represents one parameter from an action's schema. QML delegates
render type-appropriate editors (TextField, CheckBox, etc.) based on the
``paramType`` role. Optional parameters have an ``enabled`` toggle.
"""
from __future__ import annotations

import json
from typing import Any

from PySide6.QtCore import (
    QAbstractListModel,
    QByteArray,
    QModelIndex,
    Qt,
    Slot,
)


class _Param:
    """Internal storage for a single parameter row."""

    __slots__ = ("name", "param_type", "required", "description", "value", "enabled")

    def __init__(
        self,
        name: str,
        param_type: str,
        required: bool,
        description: str,
    ) -> None:
        self.name = name
        self.param_type = param_type
        self.required = required
        self.description = description
        self.value: Any = _default_for_type(param_type)
        self.enabled: bool = required


def _default_for_type(param_type: str) -> Any:
    """Return a sensible default value for a parameter type."""
    if param_type == "boolean":
        return False
    if param_type in ("number", "integer"):
        return 0
    if param_type == "array":
        return "[]"
    if param_type == "object":
        return "{}"
    return ""


# Custom roles — values must not collide with Qt built-in roles.
NAME_ROLE = Qt.ItemDataRole.UserRole + 1
PARAM_TYPE_ROLE = Qt.ItemDataRole.UserRole + 2
REQUIRED_ROLE = Qt.ItemDataRole.UserRole + 3
DESCRIPTION_ROLE = Qt.ItemDataRole.UserRole + 4
VALUE_ROLE = Qt.ItemDataRole.UserRole + 5
ENABLED_ROLE = Qt.ItemDataRole.UserRole + 6


class ParamListModel(QAbstractListModel):
    """Flat list model of action parameters for QML Repeater/ListView."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._params: list[_Param] = []

    # ── QAbstractListModel overrides ────────────────────────────────────

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return 0 if parent.isValid() else len(self._params)

    def roleNames(self) -> dict[int, QByteArray]:
        return {
            NAME_ROLE: QByteArray(b"name"),
            PARAM_TYPE_ROLE: QByteArray(b"paramType"),
            REQUIRED_ROLE: QByteArray(b"required"),
            DESCRIPTION_ROLE: QByteArray(b"description"),
            VALUE_ROLE: QByteArray(b"value"),
            ENABLED_ROLE: QByteArray(b"enabled"),
        }

    def data(self, index: QModelIndex, role: int = Qt.ItemDataRole.DisplayRole) -> Any:
        if not index.isValid() or index.row() >= len(self._params):
            return None
        p = self._params[index.row()]
        if role == NAME_ROLE:
            return p.name
        if role == PARAM_TYPE_ROLE:
            return p.param_type
        if role == REQUIRED_ROLE:
            return p.required
        if role == DESCRIPTION_ROLE:
            return p.description
        if role == VALUE_ROLE:
            return p.value
        if role == ENABLED_ROLE:
            return p.enabled
        return None

    def setData(self, index: QModelIndex, value: Any, role: int = Qt.ItemDataRole.EditRole) -> bool:
        if not index.isValid() or index.row() >= len(self._params):
            return False
        p = self._params[index.row()]
        if role == VALUE_ROLE:
            p.value = value
            self.dataChanged.emit(index, index, [VALUE_ROLE])
            return True
        if role == ENABLED_ROLE:
            p.enabled = bool(value)
            self.dataChanged.emit(index, index, [ENABLED_ROLE])
            return True
        return False

    def flags(self, index: QModelIndex) -> Qt.ItemFlag:
        return Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsEditable

    # ── Public API ──────────────────────────────────────────────────────

    def load_from_schema(self, params_dict: dict[str, dict]) -> None:
        """Rebuild rows from an action's ``params`` schema dict.

        Args:
            params_dict: ``{"paramName": {"type": "string", "required": true,
                            "description": "..."}, ...}``
        """
        self.beginResetModel()
        self._params.clear()
        for name, meta in params_dict.items():
            self._params.append(
                _Param(
                    name=name,
                    param_type=meta.get("type", "string"),
                    required=meta.get("required", True),
                    description=meta.get("description", ""),
                )
            )
        self.endResetModel()

    def build_params(self) -> dict[str, Any]:
        """Collect enabled parameters into a dict for execute_action()."""
        result: dict[str, Any] = {}
        for p in self._params:
            if not p.enabled:
                continue
            value = p.value
            if p.param_type in ("number",):
                try:
                    value = float(value) if isinstance(value, str) else value
                except (ValueError, TypeError):
                    value = 0.0
            elif p.param_type in ("integer",):
                try:
                    value = int(value) if isinstance(value, str) else value
                except (ValueError, TypeError):
                    value = 0
            elif p.param_type in ("array", "object"):
                if isinstance(value, str):
                    try:
                        value = json.loads(value)
                    except json.JSONDecodeError:
                        pass
            result[p.name] = value
        return result

    def clear(self) -> None:
        """Reset all values to defaults and enable required-only."""
        for row, p in enumerate(self._params):
            p.value = _default_for_type(p.param_type)
            p.enabled = p.required
        if self._params:
            self.dataChanged.emit(
                self.index(0),
                self.index(len(self._params) - 1),
                [VALUE_ROLE, ENABLED_ROLE],
            )

    @Slot(int, "QVariant")
    def setValue(self, row: int, value: Any) -> None:
        """Slot for QML to update a parameter value by row index."""
        idx = self.index(row)
        self.setData(idx, value, VALUE_ROLE)

    @Slot(int, bool)
    def setEnabled(self, row: int, enabled: bool) -> None:
        """Slot for QML to toggle an optional parameter's enabled state."""
        idx = self.index(row)
        self.setData(idx, enabled, ENABLED_ROLE)
```

- [ ] **Step 2: Commit**

```bash
git add plugins/ai_chat_plugin/param_list_model.py
git commit -m "feat(ai-chat): add ParamListModel for QML param form

QAbstractListModel exposing action parameters with typed roles
(name, paramType, required, description, value, enabled).
Provides setValue/setEnabled slots for QML delegates and
build_params() to collect enabled values for execution.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: DebuggerBackend

**Files:**
- Create: `plugins/ai_chat_plugin/debugger_backend.py`

This is the central QObject bridging Python data/logic to the QML UI.

- [ ] **Step 1: Create debugger_backend.py**

Create `plugins/ai_chat_plugin/debugger_backend.py`:

```python
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
```

- [ ] **Step 2: Commit**

```bash
git add plugins/ai_chat_plugin/debugger_backend.py
git commit -m "feat(ai-chat): add DebuggerBackend QObject controller

Central bridge between Python data layer and QML UI. Exposes
schemaTreeModel, paramListModel, JSON properties, and invokable
methods (selectAction, execute, clear, toggleTheme). Execution
runs on worker thread with progress callbacks via QueuedConnection.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: SchemaTreeView.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/SchemaTreeView.qml`

The tree view uses `TreeView` (Qt 6.4+) to display the module→action hierarchy
from `SchemaTreeModel`. Since `SchemaTreeModel` is a `QStandardItemModel`,
its custom roles (`MODULE_NAME_ROLE = 0x101`, `ACTION_NAME_ROLE = 0x102`) are
accessed via `model.data(index, role)` calls from the delegate.

- [ ] **Step 1: Create SchemaTreeView.qml**

Create `plugins/ai_chat_plugin/qml/SchemaTreeView.qml`:

```qml
import QtQuick
import QtQuick.Controls
import theme

/**
 * Left-panel tree view displaying module → action hierarchy.
 * Emits actionSelected(moduleName, actionName) when the user
 * clicks an action item, or moduleSelected(moduleName) for modules.
 */
Item {
    id: root

    required property var model
    signal actionSelected(string moduleName, string actionName)
    signal moduleSelected(string moduleName)

    Rectangle {
        anchors.fill: parent
        color: PluginTheme.surface
        radius: PluginTheme.radiusSmall

        TreeView {
            id: treeView
            anchors.fill: parent
            anchors.margins: PluginTheme.gapTight
            model: root.model
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            selectionModel: ItemSelectionModel {
                model: treeView.model
            }

            delegate: TreeViewDelegate {
                id: treeDelegate

                required property int row
                required property int column
                required property int depth
                required property bool isTreeNode
                required property bool expanded
                required property bool hasChildren
                required property var index

                implicitHeight: 32
                implicitWidth: treeView.width
                leftPadding: depth * 20 + (isTreeNode && hasChildren ? 0 : 20)

                contentItem: RowLayout {
                    spacing: PluginTheme.gapTight

                    Label {
                        text: treeDelegate.model.display ?? ""
                        color: treeDelegate.current
                               ? PluginTheme.textOnAccent
                               : PluginTheme.textPrimary
                        font.pixelSize: 13
                        font.weight: treeDelegate.hasChildren
                                     ? Font.DemiBold : Font.Normal
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                background: Rectangle {
                    color: {
                        if (treeDelegate.current)
                            return PluginTheme.accentA;
                        if (treeDelegate.hovered)
                            return PluginTheme.tint(
                                PluginTheme.surfaceStrong,
                                PluginTheme.darkMode ? 0.6 : 0.8
                            );
                        return "transparent";
                    }
                    radius: PluginTheme.radiusSmall / 2

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }

                // Role IDs matching SchemaTreeModel (Qt.UserRole + 1, + 2).
                readonly property int moduleNameRole: 0x101
                readonly property int actionNameRole: 0x102

                onClicked: {
                    treeView.selectionModel.setCurrentIndex(
                        treeDelegate.index,
                        ItemSelectionModel.ClearAndSelect
                    );

                    let mod = treeDelegate.model.data(
                        treeDelegate.index, treeDelegate.moduleNameRole
                    );
                    let act = treeDelegate.model.data(
                        treeDelegate.index, treeDelegate.actionNameRole
                    );

                    if (act !== undefined && act !== null) {
                        root.actionSelected(mod, act);
                    } else if (mod !== undefined && mod !== null) {
                        root.moduleSelected(mod);
                    }
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("No modules loaded")
        color: PluginTheme.textTertiary
        font.pixelSize: 13
        visible: root.model ? root.model.rowCount() === 0 : true
    }
}
```

> **Note on custom role access:** `QStandardItemModel` exposes `model.data(index, role)`
> to QML delegates. The role numbers `0x101` (257 = `Qt.UserRole + 1`) and `0x102`
> (258 = `Qt.UserRole + 2`) must match the Python constants `MODULE_NAME_ROLE` and
> `ACTION_NAME_ROLE` in `schema_tree_model.py`.

- [ ] **Step 2: Commit**

```bash
git add plugins/ai_chat_plugin/qml/SchemaTreeView.qml
git commit -m "feat(ai-chat): add SchemaTreeView QML component

TreeView rendering module→action hierarchy from SchemaTreeModel.
Uses custom roles to extract module/action names and emits
actionSelected/moduleSelected signals for backend binding.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: DetailPanel.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/DetailPanel.qml`

- [ ] **Step 1: Create DetailPanel.qml**

Create `plugins/ai_chat_plugin/qml/DetailPanel.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Read-only panel displaying the action's JSON schema.
 * The Python backend attaches a JsonHighlighter to this
 * component's TextArea document via objectName lookup.
 */
Item {
    id: root

    property string json: ""
    property string placeholderText: qsTr("← Select an action from the tree.")

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label {
            text: qsTr("Schema")
            color: PluginTheme.textSecondary
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.leftMargin: PluginTheme.gapTight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall

            ScrollView {
                anchors.fill: parent
                anchors.margins: PluginTheme.gapTight

                TextArea {
                    objectName: "detailTextArea"
                    text: root.json || root.placeholderText
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    font.family: PluginTheme.monoFont
                    font.pixelSize: 13
                    color: root.json
                           ? PluginTheme.textPrimary
                           : PluginTheme.textTertiary
                    selectionColor: PluginTheme.accentA
                    selectedTextColor: PluginTheme.textOnAccent

                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add plugins/ai_chat_plugin/qml/DetailPanel.qml
git commit -m "feat(ai-chat): add DetailPanel QML component

Read-only panel displaying action schema JSON with
JsonHighlighter support via objectName lookup.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: ParamForm.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/ParamForm.qml`

This is the most complex QML file. It uses a `Repeater` with a `Loader` to
render type-appropriate delegates based on the `paramType` role from
`ParamListModel`.

- [ ] **Step 1: Create ParamForm.qml**

Create `plugins/ai_chat_plugin/qml/ParamForm.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Dynamic parameter form rendered from ParamListModel.
 * Uses a Repeater + Loader to pick type-appropriate delegates.
 */
Item {
    id: root

    required property var model
    required property bool isExecuting
    required property real progress

    signal executeClicked()
    signal clearClicked()
    signal valueChanged(string name, var value)

    ColumnLayout {
        anchors.fill: parent
        spacing: PluginTheme.gapTight

        Label {
            text: qsTr("Parameters")
            color: PluginTheme.textSecondary
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.leftMargin: PluginTheme.gapTight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall
            clip: true

            ScrollView {
                anchors.fill: parent
                anchors.margins: PluginTheme.gapTight
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: PluginTheme.gapTight

                    Repeater {
                        model: root.model

                        delegate: RowLayout {
                            id: paramRow
                            width: parent.width
                            spacing: PluginTheme.gapTight
                            opacity: model.enabled ? 1.0 : 0.4

                            Behavior on opacity {
                                NumberAnimation { duration: PluginTheme.animFast }
                            }

                            // Optional param checkbox
                            CheckBox {
                                visible: !model.required
                                checked: model.enabled
                                onToggled: {
                                    root.model.setEnabled(index, checked);
                                }
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                            }

                            // Spacer for required params (align with checkbox)
                            Item {
                                visible: model.required
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                            }

                            // Label
                            Label {
                                text: model.name + (model.required ? " *" : "")
                                color: PluginTheme.textPrimary
                                font.pixelSize: 13
                                Layout.preferredWidth: 120
                                Layout.alignment: Qt.AlignTop
                                elide: Text.ElideRight

                                ToolTip.visible: paramLabelMa.containsMouse
                                                 && model.description !== ""
                                ToolTip.text: model.description
                                ToolTip.delay: 500

                                MouseArea {
                                    id: paramLabelMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            // Value editor — type-dependent
                            Loader {
                                Layout.fillWidth: true
                                enabled: model.enabled

                                sourceComponent: {
                                    switch (model.paramType) {
                                    case "boolean":
                                        return boolDelegate;
                                    case "number":
                                    case "integer":
                                        return numberDelegate;
                                    case "array":
                                    case "object":
                                        return textAreaDelegate;
                                    default:
                                        return stringDelegate;
                                    }
                                }

                                onLoaded: {
                                    item.paramIndex = index;
                                    item.paramValue = model.value;
                                    item.paramName = model.name;
                                }
                            }
                        }
                    }

                    // Empty state
                    Label {
                        visible: root.model ? root.model.rowCount() === 0 : true
                        text: qsTr("No parameters for this action.")
                        color: PluginTheme.textTertiary
                        font.pixelSize: 13
                        topPadding: PluginTheme.gap
                    }
                }
            }
        }

        // ── Action buttons row ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: PluginTheme.gapTight

            Button {
                text: qsTr("▶ Execute")
                enabled: !root.isExecuting
                onClicked: root.executeClicked()

                contentItem: Label {
                    text: parent.text
                    color: PluginTheme.textOnAccent
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 100
                    color: parent.enabled
                           ? (parent.down ? Qt.darker(PluginTheme.accentA, 1.2)
                                          : PluginTheme.accentA)
                           : PluginTheme.surfaceStrong
                    radius: PluginTheme.radiusSmall / 2

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }

            Button {
                text: qsTr("Clear")
                onClicked: root.clearClicked()

                contentItem: Label {
                    text: parent.text
                    color: PluginTheme.textSecondary
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 72
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : PluginTheme.surfaceMuted
                    radius: PluginTheme.radiusSmall / 2
                    border.color: PluginTheme.borderSubtle
                    border.width: 1
                }
            }

            Item { Layout.fillWidth: true }

            ProgressBar {
                id: progressBar
                visible: root.isExecuting || root.progress >= 1.0
                value: root.progress
                Layout.preferredWidth: 120
                Layout.preferredHeight: 4
            }
        }
    }

    // ── Inline delegate components ─────────────────────────────────────

    Component {
        id: stringDelegate
        TextField {
            property int paramIndex: -1
            property var paramValue: ""
            property string paramName: ""

            text: paramValue ?? ""
            placeholderText: paramName
            color: PluginTheme.textPrimary
            font.pixelSize: 13
            font.family: PluginTheme.monoFont
            selectionColor: PluginTheme.accentA
            selectedTextColor: PluginTheme.textOnAccent

            background: Rectangle {
                implicitHeight: 30
                color: PluginTheme.surface
                radius: PluginTheme.radiusSmall / 2
                border.color: parent.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1
            }

            onTextEdited: root.model.setValue(paramIndex, text)
        }
    }

    Component {
        id: numberDelegate
        TextField {
            property int paramIndex: -1
            property var paramValue: 0
            property string paramName: ""

            text: String(paramValue ?? 0)
            placeholderText: paramName
            color: PluginTheme.textPrimary
            font.pixelSize: 13
            font.family: PluginTheme.monoFont
            validator: DoubleValidator {}
            selectionColor: PluginTheme.accentA
            selectedTextColor: PluginTheme.textOnAccent

            background: Rectangle {
                implicitHeight: 30
                color: PluginTheme.surface
                radius: PluginTheme.radiusSmall / 2
                border.color: parent.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1
            }

            onTextEdited: root.model.setValue(paramIndex, text)
        }
    }

    Component {
        id: boolDelegate
        CheckBox {
            property int paramIndex: -1
            property var paramValue: false
            property string paramName: ""

            checked: paramValue ?? false
            text: checked ? "true" : "false"

            onToggled: root.model.setValue(paramIndex, checked)
        }
    }

    Component {
        id: textAreaDelegate
        TextArea {
            property int paramIndex: -1
            property var paramValue: ""
            property string paramName: ""

            text: String(paramValue ?? "")
            placeholderText: paramName
            wrapMode: TextEdit.Wrap
            color: PluginTheme.textPrimary
            font.pixelSize: 13
            font.family: PluginTheme.monoFont
            selectionColor: PluginTheme.accentA
            selectedTextColor: PluginTheme.textOnAccent

            implicitHeight: Math.max(60, contentHeight + topPadding + bottomPadding)

            background: Rectangle {
                color: PluginTheme.surface
                radius: PluginTheme.radiusSmall / 2
                border.color: parent.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1
            }

            onTextChanged: {
                if (activeFocus) {
                    root.model.setValue(paramIndex, text);
                }
            }
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add plugins/ai_chat_plugin/qml/ParamForm.qml
git commit -m "feat(ai-chat): add ParamForm QML component

Dynamic parameter form using Repeater + Loader with type-aware
delegates (string, number, boolean, array/object). Optional
params have enable checkbox. Includes Execute/Clear buttons
and progress bar.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: RequestResponseView.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/RequestResponseView.qml`

- [ ] **Step 1: Create RequestResponseView.qml**

Create `plugins/ai_chat_plugin/qml/RequestResponseView.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Horizontal split view showing request JSON (editable, left)
 * and response JSON (read-only, right).
 *
 * Python JsonHighlighter is attached to TextArea documents
 * via objectName lookup after QML load.
 */
Item {
    id: root

    property string requestJson: ""
    property string responseJson: ""
    signal requestEdited(string text)

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // ── Request panel ──────────────────────────────────────────────
        Item {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                Label {
                    text: qsTr("Request")
                    color: PluginTheme.textSecondary
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    Layout.leftMargin: PluginTheme.gapTight
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: PluginTheme.surfaceMuted
                    radius: PluginTheme.radiusSmall

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: PluginTheme.gapTight

                        TextArea {
                            objectName: "requestTextArea"
                            text: root.requestJson
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            font.family: PluginTheme.monoFont
                            font.pixelSize: 13
                            color: PluginTheme.textPrimary
                            selectionColor: PluginTheme.accentA
                            selectedTextColor: PluginTheme.textOnAccent
                            placeholderText: '{"module": "…", "action": "…", "param": {}}'

                            background: Rectangle { color: "transparent" }

                            onTextChanged: {
                                if (activeFocus && text !== root.requestJson) {
                                    root.requestEdited(text);
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Response panel ─────────────────────────────────────────────
        Item {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 200

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                Label {
                    text: qsTr("Response")
                    color: PluginTheme.textSecondary
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    Layout.leftMargin: PluginTheme.gapTight
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: PluginTheme.surfaceMuted
                    radius: PluginTheme.radiusSmall

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: PluginTheme.gapTight

                        TextArea {
                            objectName: "responseTextArea"
                            text: root.responseJson
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            font.family: PluginTheme.monoFont
                            font.pixelSize: 13
                            color: root.responseJson
                                   ? PluginTheme.textPrimary
                                   : PluginTheme.textTertiary
                            selectionColor: PluginTheme.accentA
                            selectedTextColor: PluginTheme.textOnAccent
                            placeholderText: qsTr("Execute an action to see results.")

                            background: Rectangle { color: "transparent" }
                        }
                    }
                }
            }
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add plugins/ai_chat_plugin/qml/RequestResponseView.qml
git commit -m "feat(ai-chat): add RequestResponseView QML component

Horizontal SplitView with editable request JSON (left) and
read-only response JSON (right). Both TextAreas expose
objectName for Python JsonHighlighter attachment.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: ActionDebuggerPage.qml + ActionDebuggerWindow.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/ActionDebuggerPage.qml`
- Create: `plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml`

- [ ] **Step 1: Create ActionDebuggerPage.qml**

Create `plugins/ai_chat_plugin/qml/ActionDebuggerPage.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Root embeddable component for the Action Debugger.
 * Assembles SchemaTreeView, DetailPanel, ParamForm and
 * RequestResponseView into a two-level SplitView layout.
 */
Item {
    id: root

    required property var backend

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        SchemaTreeView {
            model: root.backend.schemaTreeModel
            SplitView.preferredWidth: 240
            SplitView.minimumWidth: 180

            onActionSelected: (moduleName, actionName) => {
                root.backend.selectAction(moduleName, actionName);
            }
            onModuleSelected: (moduleName) => {
                root.backend.selectAction(moduleName, "");
            }
        }

        SplitView {
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            DetailPanel {
                json: root.backend.detailJson
                SplitView.preferredHeight: 200
                SplitView.minimumHeight: 100
            }

            ParamForm {
                model: root.backend.paramListModel
                isExecuting: root.backend.isExecuting
                progress: root.backend.progress
                SplitView.preferredHeight: 240
                SplitView.minimumHeight: 120

                onExecuteClicked: root.backend.execute()
                onClearClicked: root.backend.clear()
            }

            RequestResponseView {
                requestJson: root.backend.requestJson
                responseJson: root.backend.responseJson
                SplitView.fillHeight: true
                SplitView.minimumHeight: 120

                onRequestEdited: (text) => {
                    root.backend.requestJson = text;
                }
            }
        }
    }
}
```

- [ ] **Step 2: Create ActionDebuggerWindow.qml**

Create `plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Standalone window wrapping ActionDebuggerPage with a toolbar.
 * Used by both hosted and standalone modes.
 */
ApplicationWindow {
    id: window

    width: 1000
    height: 700
    title: qsTr("Action Debugger — AI Chat Plugin")
    color: PluginTheme.bg
    visible: true

    // Backend is injected as a context property named "backend".
    required property var backend

    // Bind theme singleton to backend dark mode state.
    Binding {
        target: PluginTheme
        property: "darkMode"
        value: window.backend.isDark
    }

    header: ToolBar {
        background: Rectangle {
            color: PluginTheme.surface
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: PluginTheme.borderSubtle
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: PluginTheme.gap
            anchors.rightMargin: PluginTheme.gap

            Label {
                text: qsTr("Action Debugger")
                color: PluginTheme.textPrimary
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                icon.source: window.backend.iconPath + (
                    window.backend.isDark
                        ? "/darkTheme.svg"
                        : "/lightTheme.svg"
                )
                icon.color: PluginTheme.textSecondary
                icon.width: 20
                icon.height: 20
                onClicked: window.backend.toggleTheme()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Toggle theme")
                ToolTip.delay: 500

                background: Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: PluginTheme.radiusSmall / 2
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : parent.hovered
                             ? PluginTheme.surfaceMuted
                             : "transparent"

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }
        }
    }

    ActionDebuggerPage {
        anchors.fill: parent
        anchors.margins: PluginTheme.gap
        backend: window.backend
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add plugins/ai_chat_plugin/qml/ActionDebuggerPage.qml plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml
git commit -m "feat(ai-chat): add ActionDebuggerPage and Window QML components

ActionDebuggerPage is the embeddable root layout assembling
tree, detail, form and request/response panels via SplitView.
ActionDebuggerWindow wraps the page in an ApplicationWindow
with toolbar and theme toggle.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 9: Update Entry Points + Attach Highlighters

**Files:**
- Modify: `plugins/ai_chat_plugin/__init__.py`
- Modify: `plugins/ai_chat_plugin/__main__.py`

Both entry points need to create a `QQmlApplicationEngine`, register the
`DebuggerBackend` as a context property, and attach `JsonHighlighter`
instances to QML `TextArea` documents after the QML loads.

- [ ] **Step 1: Rewrite __main__.py**

Replace the contents of `plugins/ai_chat_plugin/__main__.py` with:

```python
"""Standalone entry point: python -m ai_chat_plugin.

Launches the Action Debugger window using QML.  When run from
``build/bin/plugins/``, the native pywrapper module is discovered
automatically so the full schema and execute features are available.
"""
from __future__ import annotations

import os
import sys
from pathlib import Path


def _setup_standalone_paths() -> None:
    """Add build/bin to sys.path and DLL search dirs for pywrapper access."""
    plugin_pkg = Path(__file__).resolve().parent   # ai_chat_plugin/
    plugins_dir = plugin_pkg.parent                # plugins/
    bin_dir = plugins_dir.parent                   # build/bin/

    if any(bin_dir.glob("opengeolab_pywrapper*.pyd")):
        bin_str = str(bin_dir)
        if bin_str not in sys.path:
            sys.path.insert(0, bin_str)
        if hasattr(os, "add_dll_directory"):
            os.add_dll_directory(bin_str)
        os.environ["PATH"] = bin_str + os.pathsep + os.environ.get("PATH", "")

        import ctypes
        cmd_dll = bin_dir / "opengeolab_command.dll"
        if cmd_dll.exists():
            ctypes.CDLL(str(cmd_dll), winmode=0)


def _create_engine():
    """Create QQmlApplicationEngine with backend and highlighters."""
    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine

    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin._qml_setup import setup_engine

    backend = DebuggerBackend()
    engine = QQmlApplicationEngine()

    engine.rootContext().setContextProperty("backend", backend)

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.load(QUrl.fromLocalFile(str(qml_dir / "ActionDebuggerWindow.qml")))

    if not engine.rootObjects():
        print("Error: QML failed to load.", file=sys.stderr)
        sys.exit(1)

    setup_engine(engine, backend)

    # Keep references alive.
    engine._backend = backend
    return engine


def main() -> None:
    """Create a QApplication and show the Action Debugger window."""
    _setup_standalone_paths()

    from PySide6.QtWidgets import QApplication

    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)

    engine = _create_engine()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Create _qml_setup.py helper**

Create `plugins/ai_chat_plugin/_qml_setup.py`:

```python
"""Shared QML engine setup: attach JsonHighlighters to TextArea documents.

Called after the QQmlApplicationEngine has loaded the QML root object.
Finds named TextArea components and attaches syntax highlighters.
"""
from __future__ import annotations

from PySide6.QtCore import QObject
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtQuick import QQuickTextDocument

from _shared.plugin_theme import DARK, LIGHT, ThemeColors
from ai_chat_plugin.json_highlighter import JsonHighlighter


def _find_child_by_name(root: QObject, name: str) -> QObject | None:
    """Recursively find a child QObject by objectName."""
    for child in root.children():
        if child.objectName() == name:
            return child
        result = _find_child_by_name(child, name)
        if result is not None:
            return result
    return None


def _get_theme(backend) -> ThemeColors:
    """Return the current theme based on backend dark-mode state."""
    return DARK if backend.isDark else LIGHT


def setup_engine(engine: QQmlApplicationEngine, backend) -> None:
    """Attach JsonHighlighters to all named TextArea documents.

    Must be called after engine.load() has completed successfully.

    QML TextArea.textDocument is a QQuickTextDocument.  We extract
    the underlying QTextDocument to pass to QSyntaxHighlighter.
    """
    root = engine.rootObjects()[0]

    highlighters: list[JsonHighlighter] = []

    for obj_name in ("detailTextArea", "requestTextArea", "responseTextArea"):
        area = _find_child_by_name(root, obj_name)
        if area is None:
            continue
        quick_doc = area.property("textDocument")
        if not isinstance(quick_doc, QQuickTextDocument):
            continue
        text_doc = quick_doc.textDocument()
        hl = JsonHighlighter(text_doc, theme=_get_theme(backend))
        highlighters.append(hl)

    # Update all highlighters when theme changes.
    def on_theme_changed() -> None:
        theme = _get_theme(backend)
        for hl in highlighters:
            hl.set_theme(theme)

    backend.isDarkChanged.connect(on_theme_changed)

    # Prevent garbage collection.
    engine._highlighters = highlighters
```

- [ ] **Step 3: Rewrite __init__.py launch_ui()**

Replace `plugins/ai_chat_plugin/__init__.py` with:

```python
"""AI Chat Plugin — Chat with AI powered by GitHub Copilot SDK.

Segment 1 provides an Action Debugger window for browsing and executing
OpenGeoLab commands. Full chat functionality is added in Segment 2.
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
    """Show the AI Chat plugin window (Action Debugger in Segment 1)."""
    import sys
    from pathlib import Path

    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin._qml_setup import setup_engine

    backend = DebuggerBackend()
    engine = QQmlApplicationEngine()

    engine.rootContext().setContextProperty("backend", backend)

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.load(QUrl.fromLocalFile(str(qml_dir / "ActionDebuggerWindow.qml")))

    if not engine.rootObjects():
        return {"ok": False, "message": "QML failed to load."}

    setup_engine(engine, backend)

    # prevent GC
    engine._backend = backend
    _active_engines.append(engine)

    # Clean up when window closes.
    window = engine.rootObjects()[0]
    window.closing.connect(lambda: _active_engines.remove(engine))

    return {"ok": True, "message": "AI Chat (Action Debugger) launched."}
```

- [ ] **Step 4: Commit**

```bash
git add plugins/ai_chat_plugin/__init__.py plugins/ai_chat_plugin/__main__.py plugins/ai_chat_plugin/_qml_setup.py
git commit -m "feat(ai-chat): update entry points for QML-based UI

Rewrite __init__.py and __main__.py to use QQmlApplicationEngine
instead of PySide6 widgets. Shared _qml_setup.py attaches
JsonHighlighter to named TextArea documents and syncs theme.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 10: Cleanup Old Widget Files

**Files:**
- Delete: `plugins/ai_chat_plugin/action_debugger.py`
- Delete: `plugins/ai_chat_plugin/param_form_builder.py`

- [ ] **Step 1: Delete old files**

```bash
git rm plugins/ai_chat_plugin/action_debugger.py plugins/ai_chat_plugin/param_form_builder.py
```

- [ ] **Step 2: Commit**

```bash
git commit -m "refactor(ai-chat): remove old PySide6 widget files

Delete action_debugger.py and param_form_builder.py, replaced by
debugger_backend.py + param_list_model.py + QML components.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 11: Build + Standalone Smoke Test

**Files:** None (verification only)

- [ ] **Step 1: Build the project**

```bash
cmake --build build --config RelWithDebInfo --parallel 4
```

Expected: Build succeeds (no code compilation needed — Python + QML only).

- [ ] **Step 2: Copy plugin to build output**

Since plugins are Python source files, they need to be synced to the build
output directory:

```powershell
Copy-Item -Path "plugins\ai_chat_plugin" -Destination "build\bin\plugins\ai_chat_plugin" -Recurse -Force
Copy-Item -Path "plugins\_shared" -Destination "build\bin\plugins\_shared" -Recurse -Force
```

- [ ] **Step 3: Run standalone smoke test**

```bash
cd build/bin && python -m ai_chat_plugin
```

Expected: Action Debugger window opens with QML UI. Tree populates with
modules if pywrapper is available. Theme toggle works. Window closes cleanly.

If the window fails to open, check stderr for QML import errors. Common
issues:
- Missing `qmldir` → verify `qml/theme/qmldir` is present
- Import path not set → verify `engine.addImportPath(str(qml_dir))`
- Singleton not found → verify `import theme` in all QML files

- [ ] **Step 4: Verify tree navigation + execute**

1. Click a module in the tree → detail panel shows module JSON
2. Click an action → detail panel shows action schema, param form populates
3. Fill in params → request JSON updates automatically
4. Click Execute → response JSON shows result
5. Click Clear → form and response reset
6. Click theme toggle → colors switch between dark and light

- [ ] **Step 5: Verify hosted mode smoke test (if main app available)**

Launch OpenGeoLab app → click AI Chat menu item → verify QML window opens
and behaves identically to standalone mode.

---

## Dependency Graph

```
Pre-requisite commits
        │
        ▼
   Task 1: PluginTheme
        │
        ├──► Task 4: SchemaTreeView
        ├──► Task 5: DetailPanel
        ├──► Task 6: ParamForm
        └──► Task 7: RequestResponseView
                │
   Task 2: ParamListModel ──────┐
   Task 3: DebuggerBackend ─────┤
                                ▼
              Task 8: Page + Window
                       │
                       ▼
              Task 9: Entry Points
                       │
                       ▼
              Task 10: Cleanup
                       │
                       ▼
              Task 11: Verification
```

- Tasks 1, 2, 3 are independent of each other.
- Tasks 4, 5, 6, 7 depend on Task 1 (PluginTheme) but are independent of
  each other.
- Task 8 depends on Tasks 4-7 (all QML components).
- Task 9 depends on Tasks 2, 3, 8 (backend + QML page).
- Task 10 depends on Task 9.
- Task 11 depends on Task 10.
