"""QAbstractListModel for chat messages displayed in the QML ListView.

Each row represents a message with roles matching the QML delegate expectations.
Row identification uses ``msgId`` for streaming updates rather than last-index
to prevent race conditions when tool rows interleave during streaming.
"""
from __future__ import annotations

import uuid
from typing import Any

from PySide6.QtCore import QAbstractListModel, QModelIndex, Qt, Slot


class _Roles:
    """Custom role enum starting after Qt.UserRole."""

    MSG_ID = Qt.UserRole + 1
    MSG_TYPE = Qt.UserRole + 2
    CONTENT = Qt.UserRole + 3
    TOOL_NAME = Qt.UserRole + 4
    TOOL_CALL_ID = Qt.UserRole + 5
    TOOL_STATUS = Qt.UserRole + 6
    TOOL_RESULT = Qt.UserRole + 7
    CHOICES = Qt.UserRole + 8
    ANSWERED = Qt.UserRole + 9
    IS_HTML = Qt.UserRole + 10


_ROLE_NAMES = {
    _Roles.MSG_ID: b"msgId",
    _Roles.MSG_TYPE: b"msgType",
    _Roles.CONTENT: b"content",
    _Roles.TOOL_NAME: b"toolName",
    _Roles.TOOL_CALL_ID: b"toolCallId",
    _Roles.TOOL_STATUS: b"toolStatus",
    _Roles.TOOL_RESULT: b"toolResult",
    _Roles.CHOICES: b"choices",
    _Roles.ANSWERED: b"answered",
    _Roles.IS_HTML: b"isHtml",
}


def _new_row(
    msg_type: str,
    content: str,
    *,
    msg_id: str = "",
    tool_name: str = "",
    tool_call_id: str = "",
    tool_status: str = "",
    tool_result: str = "",
    choices: list[str] | None = None,
    answered: bool = False,
    is_html: bool = False,
) -> dict[int, Any]:
    """Build a role-to-value dict for one row."""
    return {
        _Roles.MSG_ID: msg_id or str(uuid.uuid4()),
        _Roles.MSG_TYPE: msg_type,
        _Roles.CONTENT: content,
        _Roles.TOOL_NAME: tool_name,
        _Roles.TOOL_CALL_ID: tool_call_id,
        _Roles.TOOL_STATUS: tool_status,
        _Roles.TOOL_RESULT: tool_result,
        _Roles.CHOICES: choices or [],
        _Roles.ANSWERED: answered,
        _Roles.IS_HTML: is_html,
    }


class ChatMessageModel(QAbstractListModel):
    """List model backing the chat message ListView in QML."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._rows: list[dict[int, Any]] = []

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:  # noqa: N802
        """Return the number of top-level rows."""
        return 0 if parent.isValid() else len(self._rows)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> Any:
        """Return role data for a model index."""
        if not index.isValid() or not 0 <= index.row() < len(self._rows):
            return None
        return self._rows[index.row()].get(role)

    def roleNames(self) -> dict[int, bytes]:  # noqa: N802
        """Return QML-visible custom role names."""
        return _ROLE_NAMES

    @Slot(str, str, result=int)
    def appendMessage(self, msg_type: str, content: str, **kwargs: Any) -> int:  # noqa: N802
        """Append a new message row and return its row index."""
        row_kwargs: dict[str, Any] = {}
        kwarg_map = {
            "msgId": "msg_id",
            "toolName": "tool_name",
            "toolCallId": "tool_call_id",
            "toolStatus": "tool_status",
            "toolResult": "tool_result",
            "choices": "choices",
            "answered": "answered",
            "isHtml": "is_html",
        }
        for camel_name, snake_name in kwarg_map.items():
            if camel_name in kwargs:
                row_kwargs[snake_name] = kwargs[camel_name]
            elif snake_name in kwargs:
                row_kwargs[snake_name] = kwargs[snake_name]

        row = _new_row(msg_type, content, **row_kwargs)
        row_index = len(self._rows)
        self.beginInsertRows(QModelIndex(), row_index, row_index)
        self._rows.append(row)
        self.endInsertRows()
        return row_index

    def appendToLastAssistant(self, delta: str) -> None:  # noqa: N802
        """Append streaming text to the last non-finalized assistant row.

        Uses scan-from-end (not msgId) because SDK streaming deltas don't
        carry a frontend message ID.  The invariant that at most one
        non-finalized assistant row exists is enforced by the UI disabling
        send during streaming.  ``finalizeAssistant`` uses msgId for the
        final replacement once the SDK's ``assistant.message`` event fires.
        """
        for row_index in range(len(self._rows) - 1, -1, -1):
            row = self._rows[row_index]
            if row[_Roles.MSG_TYPE] == "assistant" and not row[_Roles.IS_HTML]:
                row[_Roles.CONTENT] += delta
                index = self.index(row_index, 0)
                self.dataChanged.emit(index, index, [_Roles.CONTENT])
                return

    def finalizeAssistant(self, msg_id: str, html: str) -> None:  # noqa: N802
        """Replace assistant content with finalized HTML for the matching msgId."""
        for row_index in range(len(self._rows) - 1, -1, -1):
            row = self._rows[row_index]
            if row[_Roles.MSG_ID] == msg_id:
                row[_Roles.CONTENT] = html
                row[_Roles.IS_HTML] = True
                index = self.index(row_index, 0)
                self.dataChanged.emit(index, index, [_Roles.CONTENT, _Roles.IS_HTML])
                return

    def updateToolStatus(self, tool_call_id: str, status: str, result: str) -> None:  # noqa: N802
        """Update status and result for a tool row identified by toolCallId."""
        for row_index, row in enumerate(self._rows):
            if row[_Roles.TOOL_CALL_ID] == tool_call_id:
                row[_Roles.TOOL_STATUS] = status
                row[_Roles.TOOL_RESULT] = result
                index = self.index(row_index, 0)
                self.dataChanged.emit(index, index, [_Roles.TOOL_STATUS, _Roles.TOOL_RESULT])
                return

    def markAskUserAnswered(self, row_index: int) -> None:  # noqa: N802
        """Mark an askUser row as answered."""
        if 0 <= row_index < len(self._rows):
            self._rows[row_index][_Roles.ANSWERED] = True
            index = self.index(row_index, 0)
            self.dataChanged.emit(index, index, [_Roles.ANSWERED])

    @Slot()
    def clear(self) -> None:
        """Remove all rows."""
        if not self._rows:
            return
        self.beginRemoveRows(QModelIndex(), 0, len(self._rows) - 1)
        self._rows.clear()
        self.endRemoveRows()
