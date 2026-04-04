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
