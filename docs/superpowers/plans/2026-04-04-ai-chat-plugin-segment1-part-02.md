# AI Chat Plugin Segment 1 — Part 2 of 2

> Part 文件：包含 Tasks 5–8（Schema Tree Model、Param Form Builder、Action Debugger 主窗口、QML 接入 + 集成验证）。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build the Action Debugger UI components (tree model, form builder, main window) and wire the plugin into the QML ribbon.

**Spec:** `docs/superpowers/specs/2026-04-03-ai-chat-plugin-design.md` — Segment 1

**Prerequisite:** Part 1 (Tasks 1–4) must be complete. The following files must exist:
- `plugins/_shared/plugin_theme.py`
- `plugins/ai_chat_plugin/__init__.py`
- `plugins/ai_chat_plugin/__main__.py`
- `plugins/ai_chat_plugin/scene_tools.py`
- `plugins/ai_chat_plugin/json_highlighter.py`

---

### Task 5: Create `schema_tree_model.py` — module → action tree

**Files:**
- Create: `plugins/ai_chat_plugin/schema_tree_model.py`

- [ ] **Step 1: Create `plugins/ai_chat_plugin/schema_tree_model.py`**

```python
"""QStandardItemModel representing the module → action hierarchy.

Populates from scene_tools cached schema. Custom data roles allow
retrieval of module/action names from model indexes.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QStandardItem, QStandardItemModel


MODULE_NAME_ROLE = Qt.ItemDataRole.UserRole + 1
ACTION_NAME_ROLE = Qt.ItemDataRole.UserRole + 2


class SchemaTreeModel(QStandardItemModel):
    """Tree model: root items are modules, children are actions."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setHorizontalHeaderLabels(["Command Schema"])

    def load_from_scene_tools(self) -> None:
        """Populate the tree from scene_tools cached schema.

        Clears existing items and rebuilds from current schema state.
        Safe to call multiple times (e.g., after invalidate_cache).
        """
        from ai_chat_plugin import scene_tools

        self.clear()
        self.setHorizontalHeaderLabels(["Command Schema"])

        modules = scene_tools.list_modules()

        if not modules:
            empty_item = QStandardItem("No modules available (standalone mode)")
            empty_item.setEnabled(False)
            empty_item.setSelectable(False)
            self.appendRow(empty_item)
            return

        for mod_info in modules:
            mod_name = mod_info["name"]
            mod_desc = mod_info.get("description", "")

            mod_item = QStandardItem(mod_name)
            mod_item.setData(mod_name, MODULE_NAME_ROLE)
            mod_item.setData(None, ACTION_NAME_ROLE)
            mod_item.setToolTip(mod_desc)
            mod_item.setEditable(False)

            # Fetch actions for this module.
            mod_detail = scene_tools.describe_module(mod_name)
            if mod_detail:
                for act_info in mod_detail.get("actions", []):
                    act_name = act_info["name"]
                    act_desc = act_info.get("description", "")

                    act_item = QStandardItem(act_name)
                    act_item.setData(mod_name, MODULE_NAME_ROLE)
                    act_item.setData(act_name, ACTION_NAME_ROLE)
                    act_item.setToolTip(act_desc)
                    act_item.setEditable(False)
                    mod_item.appendRow(act_item)

            self.appendRow(mod_item)

    @staticmethod
    def is_action_item(index) -> bool:
        """Return True if the index points to an action (not a module)."""
        return index.data(ACTION_NAME_ROLE) is not None

    @staticmethod
    def module_name(index) -> str | None:
        """Return the module name for the given index."""
        return index.data(MODULE_NAME_ROLE)

    @staticmethod
    def action_name(index) -> str | None:
        """Return the action name for the given index (None if module)."""
        return index.data(ACTION_NAME_ROLE)
```

- [ ] **Step 2: Smoke test in standalone mode**

```bash
cd plugins && ../pyvenv/Scripts/python.exe -c "
import sys; sys.path.insert(0, '.')
from PySide6.QtWidgets import QApplication
app = QApplication(sys.argv)
from ai_chat_plugin.schema_tree_model import SchemaTreeModel
model = SchemaTreeModel()
model.load_from_scene_tools()
print('Row count:', model.rowCount())
if model.rowCount() > 0:
    item = model.item(0)
    print('First item:', item.text(), '| enabled:', item.isEnabled())
app.quit()
"
```
Expected (standalone):
```
Row count: 1
First item: No modules available (standalone mode) | enabled: False
```

- [ ] **Step 3: Commit**

```bash
git add plugins/ai_chat_plugin/schema_tree_model.py
git commit -m "feat(ai-chat): add schema tree model for module/action hierarchy

SchemaTreeModel (QStandardItemModel) populates from scene_tools cache.
Module root items with action children. Custom data roles for retrieving
module/action names. Shows placeholder in standalone mode.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: Create `param_form_builder.py` — auto form + JSON toggle

**Files:**
- Create: `plugins/ai_chat_plugin/param_form_builder.py`

- [ ] **Step 1: Create `plugins/ai_chat_plugin/param_form_builder.py`**

```python
"""Auto-generate parameter input form from an action's param schema.

Reads the ``params`` dict from ``describe_action()`` and creates typed Qt
widgets.  A toggle button switches between Form view (auto-generated
widgets) and Raw JSON view (plain text editor).  Both modes stay
synchronised: switching from Form → JSON serialises current widget values;
switching from JSON → Form parses and populates widgets.
"""
from __future__ import annotations

import json
from typing import Any

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QDoubleSpinBox,
    QFormLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QSpinBox,
    QStackedWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)


class ParamFormBuilder(QWidget):
    """Widget that builds a parameter input form from a JSON schema dict.

    Signals:
        params_changed: Emitted when any form field value changes.
    """

    params_changed = Signal()

    # Indices for the stacked widget pages.
    _FORM_PAGE = 0
    _JSON_PAGE = 1

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._param_schema: dict[str, dict] = {}
        self._field_widgets: dict[str, QWidget] = {}

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)

        # Toggle bar.
        toggle_row = QHBoxLayout()
        toggle_row.setContentsMargins(0, 0, 0, 0)
        self._form_btn = QPushButton("Form")
        self._json_btn = QPushButton("JSON")
        self._form_btn.setCheckable(True)
        self._json_btn.setCheckable(True)
        self._form_btn.setChecked(True)
        self._form_btn.clicked.connect(lambda: self._switch_page(self._FORM_PAGE))
        self._json_btn.clicked.connect(lambda: self._switch_page(self._JSON_PAGE))
        toggle_row.addWidget(self._form_btn)
        toggle_row.addWidget(self._json_btn)
        toggle_row.addStretch()
        layout.addLayout(toggle_row)

        # Stacked widget: page 0 = form, page 1 = raw JSON.
        self._stack = QStackedWidget()

        # Page 0: scrollable form.
        self._form_widget = QWidget()
        self._form_layout = QFormLayout(self._form_widget)
        self._form_layout.setContentsMargins(0, 0, 0, 0)
        self._form_layout.setSpacing(6)
        self._stack.addWidget(self._form_widget)

        # Page 1: raw JSON editor.
        self._json_editor = QTextEdit()
        self._json_editor.setPlaceholderText('{"key": "value"}')
        self._json_editor.setAcceptRichText(False)
        self._stack.addWidget(self._json_editor)

        layout.addWidget(self._stack)

    # ── Public API ───────────────────────────────────────────────────

    def set_schema(self, params_schema: dict[str, dict]) -> None:
        """Rebuild the form from a new parameter schema.

        Args:
            params_schema: ``{"paramName": {"type": "...", "required": bool,
                            "description": "..."}, ...}``
        """
        self._param_schema = params_schema
        self._rebuild_form()

    def build_params(self) -> dict[str, Any]:
        """Collect current input values into a dict.

        If in JSON mode, parses the text editor content directly.
        If in Form mode, reads each widget's value.
        """
        if self._stack.currentIndex() == self._JSON_PAGE:
            text = self._json_editor.toPlainText().strip()
            if not text:
                return {}
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                return {}

        result: dict[str, Any] = {}
        for name, widget in self._field_widgets.items():
            value = self._read_widget(name, widget)
            if value is not None:
                result[name] = value
        return result

    def clear(self) -> None:
        """Reset all fields to defaults or empty."""
        for name, widget in self._field_widgets.items():
            schema = self._param_schema.get(name, {})
            self._set_widget_default(widget, schema)
        self._json_editor.clear()

    # ── Internal ─────────────────────────────────────────────────────

    def _switch_page(self, page: int) -> None:
        """Toggle between Form and JSON views, synchronising state."""
        if page == self._JSON_PAGE and self._stack.currentIndex() == self._FORM_PAGE:
            # Serialise form → JSON.
            params = self.build_params()
            self._json_editor.setPlainText(
                json.dumps(params, indent=2, ensure_ascii=False) if params else ""
            )
        elif page == self._FORM_PAGE and self._stack.currentIndex() == self._JSON_PAGE:
            # Parse JSON → form widgets.
            try:
                params = json.loads(self._json_editor.toPlainText())
                self._populate_form(params)
            except (json.JSONDecodeError, TypeError):
                pass  # Keep form as-is if JSON is invalid.

        self._stack.setCurrentIndex(page)
        self._form_btn.setChecked(page == self._FORM_PAGE)
        self._json_btn.setChecked(page == self._JSON_PAGE)

    def _rebuild_form(self) -> None:
        """Clear and recreate form widgets from the current schema."""
        # Remove old widgets.
        while self._form_layout.rowCount() > 0:
            self._form_layout.removeRow(0)
        self._field_widgets.clear()
        self._json_editor.clear()

        if not self._param_schema:
            self._form_layout.addRow(QLabel("No parameters"))
            return

        for name, spec in self._param_schema.items():
            ptype = spec.get("type", "string")
            required = spec.get("required", False)
            description = spec.get("description", "")

            label_text = f"{'* ' if required else ''}{name}"
            widget = self._create_widget(ptype, spec)
            widget.setToolTip(f"[{ptype}] {description}")
            self._field_widgets[name] = widget
            self._form_layout.addRow(label_text, widget)

    def _create_widget(self, ptype: str, spec: dict) -> QWidget:
        """Create a typed input widget for a parameter."""
        if ptype == "number":
            w = QDoubleSpinBox()
            w.setRange(-1e9, 1e9)
            w.setDecimals(4)
            w.setValue(0.0)
            w.valueChanged.connect(lambda: self.params_changed.emit())
            return w
        if ptype == "integer":
            w = QSpinBox()
            w.setRange(-2_147_483_648, 2_147_483_647)
            w.setValue(0)
            w.valueChanged.connect(lambda: self.params_changed.emit())
            return w
        if ptype == "boolean":
            w = QCheckBox()
            w.setChecked(False)
            w.stateChanged.connect(lambda: self.params_changed.emit())
            return w
        if ptype in ("array", "object"):
            w = QTextEdit()
            w.setPlaceholderText("[]" if ptype == "array" else "{}")
            w.setAcceptRichText(False)
            w.setMaximumHeight(80)
            w.textChanged.connect(self.params_changed.emit)
            return w
        # Default: string.
        w = QLineEdit()
        w.setPlaceholderText(spec.get("description", ""))
        w.textChanged.connect(lambda: self.params_changed.emit())
        return w

    def _read_widget(self, name: str, widget: QWidget) -> Any:
        """Read the current value from a widget, returning native Python types."""
        if isinstance(widget, QDoubleSpinBox):
            return widget.value()
        if isinstance(widget, QSpinBox):
            return widget.value()
        if isinstance(widget, QCheckBox):
            return widget.isChecked()
        if isinstance(widget, QTextEdit):
            text = widget.toPlainText().strip()
            if not text:
                return None
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                return text
        if isinstance(widget, QLineEdit):
            text = widget.text().strip()
            return text if text else None
        return None

    @staticmethod
    def _set_widget_default(widget: QWidget, schema: dict) -> None:
        """Reset a widget to its default or empty state."""
        if isinstance(widget, QDoubleSpinBox):
            widget.setValue(0.0)
        elif isinstance(widget, QSpinBox):
            widget.setValue(0)
        elif isinstance(widget, QCheckBox):
            widget.setChecked(False)
        elif isinstance(widget, QTextEdit):
            widget.clear()
        elif isinstance(widget, QLineEdit):
            widget.clear()

    def _populate_form(self, params: dict) -> None:
        """Set form widget values from a params dict."""
        for name, value in params.items():
            widget = self._field_widgets.get(name)
            if widget is None:
                continue
            if isinstance(widget, QDoubleSpinBox) and isinstance(value, (int, float)):
                widget.setValue(float(value))
            elif isinstance(widget, QSpinBox) and isinstance(value, int):
                widget.setValue(value)
            elif isinstance(widget, QCheckBox) and isinstance(value, bool):
                widget.setChecked(value)
            elif isinstance(widget, QTextEdit):
                widget.setPlainText(
                    json.dumps(value, indent=2, ensure_ascii=False)
                    if not isinstance(value, str) else value
                )
            elif isinstance(widget, QLineEdit) and isinstance(value, str):
                widget.setText(value)
```

- [ ] **Step 2: Smoke test form builder**

```bash
cd plugins && ../pyvenv/Scripts/python.exe -c "
import sys; sys.path.insert(0, '.')
from PySide6.QtWidgets import QApplication
app = QApplication(sys.argv)
from ai_chat_plugin.param_form_builder import ParamFormBuilder
fb = ParamFormBuilder()
fb.set_schema({
    'width':  {'type': 'number',  'required': False, 'description': 'Box width'},
    'name':   {'type': 'string',  'required': False, 'description': 'Shape name'},
    'tessellate': {'type': 'boolean', 'required': False, 'description': 'Tessellate?'},
})
print('Field count:', len(fb._field_widgets))
params = fb.build_params()
print('Default params:', params)
app.quit()
"
```
Expected:
```
Field count: 3
Default params: {'width': 0.0, 'tessellate': False}
```
(Note: `name` is empty string → excluded by `_read_widget`.)

- [ ] **Step 3: Commit**

```bash
git add plugins/ai_chat_plugin/param_form_builder.py
git commit -m "feat(ai-chat): add auto-generated param form builder

ParamFormBuilder reads action param schema and creates typed widgets
(QDoubleSpinBox, QSpinBox, QCheckBox, QLineEdit, QTextEdit). Toggle
between Form and Raw JSON modes with bidirectional sync. Will be used
in the Action Debugger execute panel.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: Create `action_debugger.py` — main window

**Files:**
- Create: `plugins/ai_chat_plugin/action_debugger.py`

- [ ] **Step 1: Create `plugins/ai_chat_plugin/action_debugger.py`**

```python
"""Action Debugger — interactive browser and executor for OpenGeoLab commands.

Layout:
  ┌─────────────┬──────────────────────────────┐
  │  TreeView    │  Action Detail (JSON)        │
  │  (modules →  │  ────────────────────────    │
  │   actions)   │  Execute Panel               │
  │              │  [Form | JSON]  [▶] [Clear]  │
  │              │  Response + Progress          │
  └─────────────┴──────────────────────────────┘
"""
from __future__ import annotations

import json
import threading

from PySide6.QtCore import QMetaObject, Qt, Q_ARG, Slot
from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QProgressBar,
    QPushButton,
    QSplitter,
    QTextBrowser,
    QTreeView,
    QVBoxLayout,
    QWidget,
)

from _shared.plugin_theme import build_stylesheet, current_theme
from ai_chat_plugin.json_highlighter import JsonHighlighter
from ai_chat_plugin.param_form_builder import ParamFormBuilder
from ai_chat_plugin.schema_tree_model import (
    SchemaTreeModel,
)


class ActionDebuggerWindow(QWidget):
    """Main Action Debugger window for browsing and executing commands."""

    def __init__(self, embedded: bool = True) -> None:
        super().__init__()
        self._embedded = embedded

        theme = current_theme()
        self.setWindowTitle("Action Debugger — AI Chat Plugin")
        self.resize(960, 640)
        self.setStyleSheet(build_stylesheet(theme))

        # ── State ────────────────────────────────────────────────────
        self._current_module: str | None = None
        self._current_action: str | None = None

        # ── Layout ───────────────────────────────────────────────────
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        splitter = QSplitter(Qt.Orientation.Horizontal)

        # ── Left: Tree View ──────────────────────────────────────────
        self._tree_model = SchemaTreeModel(self)
        self._tree_model.load_from_scene_tools()

        self._tree_view = QTreeView()
        self._tree_view.setModel(self._tree_model)
        self._tree_view.setHeaderHidden(True)
        self._tree_view.setMinimumWidth(200)
        self._tree_view.expandAll()
        self._tree_view.selectionModel().currentChanged.connect(
            self._on_tree_selection_changed
        )
        splitter.addWidget(self._tree_view)

        # ── Right panel ──────────────────────────────────────────────
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(8, 8, 8, 8)
        right_layout.setSpacing(8)

        right_splitter = QSplitter(Qt.Orientation.Vertical)

        # Top: Action detail.
        self._detail_browser = QTextBrowser()
        self._detail_browser.setOpenLinks(False)
        self._detail_browser.setPlainText("← Select an action from the tree.")
        self._detail_highlighter = JsonHighlighter(
            self._detail_browser.document(), theme=theme
        )
        right_splitter.addWidget(self._detail_browser)

        # Bottom: Execute panel.
        exec_panel = QWidget()
        exec_layout = QVBoxLayout(exec_panel)
        exec_layout.setContentsMargins(0, 0, 0, 0)
        exec_layout.setSpacing(6)

        # Param form.
        self._param_form = ParamFormBuilder()
        exec_layout.addWidget(self._param_form)

        # Buttons row.
        btn_row = QHBoxLayout()
        btn_row.setSpacing(8)

        self._exec_btn = QPushButton("▶ Execute")
        self._exec_btn.setEnabled(False)
        self._exec_btn.clicked.connect(self._on_execute)
        btn_row.addWidget(self._exec_btn)

        clear_btn = QPushButton("Clear")
        clear_btn.setProperty("role", "secondary")
        clear_btn.clicked.connect(self._on_clear)
        btn_row.addWidget(clear_btn)

        btn_row.addStretch()
        exec_layout.addLayout(btn_row)

        # Progress bar.
        self._progress_bar = QProgressBar()
        self._progress_bar.setRange(0, 100)
        self._progress_bar.setValue(0)
        self._progress_bar.setTextVisible(False)
        self._progress_bar.setFixedHeight(4)
        self._progress_bar.setVisible(False)
        exec_layout.addWidget(self._progress_bar)

        self._progress_label = QLabel("")
        self._progress_label.setProperty("role", "secondary")
        self._progress_label.setVisible(False)
        exec_layout.addWidget(self._progress_label)

        # Response display.
        response_label = QLabel("Response:")
        response_label.setProperty("role", "secondary")
        exec_layout.addWidget(response_label)

        self._response_browser = QTextBrowser()
        self._response_browser.setOpenLinks(False)
        self._response_browser.setPlaceholderText("Execute an action to see results.")
        self._response_highlighter = JsonHighlighter(
            self._response_browser.document(), theme=theme
        )
        exec_layout.addWidget(self._response_browser)

        right_splitter.addWidget(exec_panel)
        right_splitter.setStretchFactor(0, 2)  # Detail gets 2/5.
        right_splitter.setStretchFactor(1, 3)  # Execute gets 3/5.

        right_layout.addWidget(right_splitter)
        splitter.addWidget(right_panel)

        # Splitter proportions: 30% tree, 70% detail+execute.
        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 7)
        root.addWidget(splitter)

    # ── Slots ────────────────────────────────────────────────────────

    def _on_tree_selection_changed(self, current, _previous) -> None:
        """Handle tree item selection — update detail and form."""
        from ai_chat_plugin import scene_tools

        mod_name = SchemaTreeModel.module_name(current)
        act_name = SchemaTreeModel.action_name(current)

        self._current_module = mod_name
        self._current_action = act_name

        if act_name and mod_name:
            # Action selected — show full schema.
            action_schema = scene_tools.describe_action(mod_name, act_name)
            if action_schema:
                pretty = json.dumps(action_schema, indent=2, ensure_ascii=False)
                self._detail_browser.setPlainText(pretty)
                self._param_form.set_schema(action_schema.get("params", {}))
                self._exec_btn.setEnabled(True)
            else:
                self._detail_browser.setPlainText(
                    f"Action '{act_name}' not found in module '{mod_name}'."
                )
                self._param_form.set_schema({})
                self._exec_btn.setEnabled(False)
        elif mod_name:
            # Module selected — show module overview.
            mod_detail = scene_tools.describe_module(mod_name)
            if mod_detail:
                pretty = json.dumps(mod_detail, indent=2, ensure_ascii=False)
                self._detail_browser.setPlainText(pretty)
            else:
                self._detail_browser.setPlainText(f"Module '{mod_name}' not found.")
            self._param_form.set_schema({})
            self._exec_btn.setEnabled(False)
        else:
            self._detail_browser.setPlainText("← Select an action from the tree.")
            self._param_form.set_schema({})
            self._exec_btn.setEnabled(False)

    def _on_execute(self) -> None:
        """Execute the selected action on a worker thread."""
        if not self._current_module or not self._current_action:
            return

        params = self._param_form.build_params()
        module = self._current_module
        action = self._current_action

        self._exec_btn.setEnabled(False)
        self._response_browser.setPlainText("Executing...")
        self._progress_bar.setValue(0)
        self._progress_bar.setVisible(True)
        self._progress_label.setText("")
        self._progress_label.setVisible(True)

        def progress_callback(progress: float, message: str) -> None:
            pct = int(progress * 100)
            QMetaObject.invokeMethod(
                self,
                "_update_progress",
                Qt.ConnectionType.QueuedConnection,
                Q_ARG(int, pct),
                Q_ARG(str, message),
            )

        def run() -> None:
            from ai_chat_plugin import scene_tools

            try:
                result = scene_tools.execute_action(
                    module, action, params, progress_callback=progress_callback
                )
                result_json = json.dumps(result, indent=2, ensure_ascii=False)
            except Exception as exc:
                result_json = json.dumps(
                    {"ok": False, "error": str(exc)}, indent=2
                )
            QMetaObject.invokeMethod(
                self,
                "_on_execute_done",
                Qt.ConnectionType.QueuedConnection,
                Q_ARG(str, result_json),
            )

        threading.Thread(target=run, daemon=True).start()

    def _on_clear(self) -> None:
        """Clear the param form and response display."""
        self._param_form.clear()
        self._response_browser.clear()
        self._progress_bar.setVisible(False)
        self._progress_label.setVisible(False)

    @Slot(int, str)
    def _update_progress(self, percent: int, message: str) -> None:
        """Update progress bar from worker thread signal."""
        self._progress_bar.setValue(percent)
        self._progress_label.setText(
            f"{percent}%  {message}" if message else f"{percent}%"
        )

    @Slot(str)
    def _on_execute_done(self, result_json: str) -> None:
        """Display execution result from worker thread."""
        self._response_browser.setPlainText(result_json)
        self._exec_btn.setEnabled(True)
        self._progress_bar.setValue(100)
        self._progress_label.setText("Done")
```

- [ ] **Step 2: Smoke test — standalone window launch**

```bash
cd plugins && ../pyvenv/Scripts/python.exe -c "
import sys; sys.path.insert(0, '.')
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer
app = QApplication(sys.argv)
from ai_chat_plugin.action_debugger import ActionDebuggerWindow
w = ActionDebuggerWindow(embedded=False)
w.show()
# Auto-close after 2 seconds for CI-friendly testing.
QTimer.singleShot(2000, app.quit)
app.exec()
print('Window opened and closed successfully.')
"
```
Expected:
```
Window opened and closed successfully.
```
(Window should appear briefly showing "No modules available (standalone mode)" in the tree and "← Select an action from the tree." in the detail panel.)

- [ ] **Step 3: Commit**

```bash
git add plugins/ai_chat_plugin/action_debugger.py
git commit -m "feat(ai-chat): add Action Debugger main window

ActionDebuggerWindow with horizontal QSplitter: left TreeView browsing
module/action hierarchy, right panel with JSON detail (syntax highlighted),
param form (auto-generated + JSON toggle), Execute button with worker
thread, progress bar, and response display. Themed via _shared.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: QML wiring + build verification + integration test

**Files:**
- Modify: `src/app/resource/qml/Main.qml:72-76` (add `aiChat` action handler)

- [ ] **Step 1: Add `aiChat` handler in `Main.qml` `openActionPage` function**

In `src/app/resource/qml/Main.qml`, add a handler for the `aiChat` action key. Insert after the `"newModel"` handler block (after line 98) and before the `// PySide6 UI plugins` comment on line 100:

```qml
        // AI Chat — launch via plugin system.
        if (actionKey === "aiChat") {
            root.statusNote = qsTr("Launching AI Chat…");
            root.menuOpen = false;
            RequestService.executeOnMainThread(JSON.stringify({
                module: "plugins",
                action: "invoke_ui",
                param: {
                    pluginName: "ai_chat_plugin"
                },
                mute: true
            }));
            return;
        }
```

This follows the same pattern as `pluginUI_` handling but with a hardcoded plugin name for the ribbon-bound action key.

- [ ] **Step 2: Build the project**

```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: Build succeeds without errors. The post-build step copies `plugins/ai_chat_plugin/` to `build/bin/plugins/ai_chat_plugin/`.

- [ ] **Step 3: Verify plugin discovery**

Launch the built application, then check the log/status for plugin discovery. The AI Chat plugin should appear in the plugin list:
```json
{"name": "AI Chat", "description": "Chat with AI powered by GitHub Copilot SDK.", "hasUI": true, "moduleName": "ai_chat_plugin"}
```

- [ ] **Step 4: Verify hosted mode — AI tab → Chat button**

1. Launch the application
2. Click the "AI" tab in the ribbon
3. Click the "Chat" button
4. Expected: Action Debugger window opens with the tree populated (geometry, scene, mesh, io modules with their actions)
5. Select an action (e.g., geometry → create_box)
6. Expected: Right panel shows full action schema JSON with syntax highlighting
7. Fill in parameters (Form mode or JSON mode)
8. Click "▶ Execute"
9. Expected: Progress bar appears, response JSON shown in response panel

- [ ] **Step 5: Verify standalone mode**

```bash
cd build/bin/plugins && ../../../pyvenv/Scripts/python.exe -m ai_chat_plugin
```
Expected: Window opens with "No modules available (standalone mode)" in tree, Execute button disabled.

- [ ] **Step 6: Verify existing plugins not broken**

1. In the application, go to Plugins tab
2. Click demo_ui_plugin → window should open with themed UI
3. Click selection_demo_plugin → window should open with themed UI
4. Both should function normally (Create Box, Select, Camera controls)

- [ ] **Step 7: Commit**

```bash
git add src/app/resource/qml/Main.qml
git commit -m "feat(app): wire aiChat ribbon action to ai_chat_plugin

Add handler in Main.qml openActionPage() for the 'aiChat' action key.
Launches ai_chat_plugin via RequestService.executeOnMainThread() using
the standard plugins.invoke_ui protocol.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
