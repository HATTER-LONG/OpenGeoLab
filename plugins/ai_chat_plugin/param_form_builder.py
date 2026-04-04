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
