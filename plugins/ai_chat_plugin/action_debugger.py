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
