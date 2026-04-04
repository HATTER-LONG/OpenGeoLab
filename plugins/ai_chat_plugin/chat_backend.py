"""ChatBackend — main-thread QObject bridging QML and CopilotWorker.

Owns the ChatMessageModel and CopilotWorker.  Exposes properties and
invokable methods for QML, and routes worker signals to model updates.
"""
from __future__ import annotations

import uuid

from PySide6.QtCore import (
    Property,
    QObject,
    Signal,
    Slot,
)

from ai_chat_plugin.chat_message_model import ChatMessageModel
from ai_chat_plugin.copilot_worker import CopilotWorker
from ai_chat_plugin.markdown_converter import markdown_to_html


class ChatBackend(QObject):
    """Controller bridging QML chat UI and the Copilot SDK worker."""

    # Notify signals for Q_PROPERTY
    isConnectingChanged = Signal()
    isStreamingChanged = Signal()
    connectionErrorChanged = Signal()
    darkModeChanged = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._message_model = ChatMessageModel(self)
        self._worker = CopilotWorker(self)
        self._is_connecting = True
        self._is_streaming = False
        self._connection_error = ""
        self._dark_mode = True
        self._active_msg_id = ""

        # Connect worker signals
        self._worker.messageDelta.connect(self._on_delta)
        self._worker.messageComplete.connect(self._on_complete)
        self._worker.toolStarted.connect(self._on_tool_started)
        self._worker.toolCompleted.connect(self._on_tool_completed)
        self._worker.askUserRequested.connect(self._on_ask_user)
        self._worker.connectionReady.connect(self._on_connected)
        self._worker.connectionFailed.connect(self._on_connection_failed)
        self._worker.sessionIdle.connect(self._on_idle)

        # Start the worker thread
        self._worker.start()

    # -- Properties for QML ----------------------------------------------------

    @Property(QObject, constant=True)
    def messageModel(self):
        return self._message_model

    def _get_is_connecting(self) -> bool:
        return self._is_connecting

    isConnecting = Property(
        bool, _get_is_connecting, notify=isConnectingChanged
    )

    def _get_is_streaming(self) -> bool:
        return self._is_streaming

    isStreaming = Property(
        bool, _get_is_streaming, notify=isStreamingChanged
    )

    def _get_connection_error(self) -> str:
        return self._connection_error

    connectionError = Property(
        str, _get_connection_error, notify=connectionErrorChanged
    )

    def _get_dark_mode(self) -> bool:
        return self._dark_mode

    def _set_dark_mode(self, value: bool) -> None:
        if self._dark_mode != value:
            self._dark_mode = value
            self.darkModeChanged.emit()

    darkMode = Property(
        bool, _get_dark_mode, _set_dark_mode, notify=darkModeChanged
    )

    # -- Invokable methods for QML ---------------------------------------------

    @Slot(str)
    def sendMessage(self, text: str) -> None:
        """Append user message and forward to the SDK worker."""
        text = text.strip()
        if not text:
            return
        self._message_model.appendMessage("user", text)

        # Create an assistant placeholder row for streaming
        self._active_msg_id = str(uuid.uuid4())
        self._message_model.appendMessage(
            "assistant", "", msgId=self._active_msg_id
        )

        self._set_streaming(True)
        self._worker.queueMessage(text)

    @Slot(str)
    def respondToAskUser(self, answer: str) -> None:
        """Forward the user's ask_user response to the worker."""
        # Find the last askUser row and mark it answered
        for i in range(self._message_model.rowCount() - 1, -1, -1):
            idx = self._message_model.index(i, 0)
            role_names = self._message_model.roleNames()
            msg_type_role = [r for r, n in role_names.items() if n == b"msgType"][0]
            answered_role = [r for r, n in role_names.items() if n == b"answered"][0]
            if (
                self._message_model.data(idx, msg_type_role) == "askUser"
                and not self._message_model.data(idx, answered_role)
            ):
                self._message_model.markAskUserAnswered(i)
                break
        self._worker.setAskUserResponse(answer)

    @Slot()
    def newSession(self) -> None:
        """Clear the chat and create a fresh SDK session."""
        self._set_streaming(False)
        self._message_model.clear()
        self._set_connecting(True)
        self._worker.requestNewSession()

    @Slot()
    def retryConnection(self) -> None:
        """Retry SDK connection after a failure."""
        self._connection_error = ""
        self.connectionErrorChanged.emit()
        self._set_connecting(True)
        # Restart the worker thread
        if not self._worker.isRunning():
            self._worker.start()

    # -- Worker signal handlers ------------------------------------------------

    def _on_delta(self, delta: str) -> None:
        self._message_model.appendToLastAssistant(delta)

    def _on_complete(self, raw_markdown: str) -> None:
        html = markdown_to_html(raw_markdown, dark_mode=self._dark_mode)
        self._message_model.finalizeAssistant(self._active_msg_id, html)
        self._active_msg_id = ""

    def _on_tool_started(self, tool_name: str, tool_call_id: str) -> None:
        self._message_model.appendMessage(
            "tool", "",
            toolName=tool_name,
            toolCallId=tool_call_id,
            toolStatus="running",
        )

    def _on_tool_completed(
        self, tool_call_id: str, result: str, success: bool
    ) -> None:
        status = "success" if success else "error"
        self._message_model.updateToolStatus(tool_call_id, status, result)

    def _on_ask_user(self, question: str, choices: list) -> None:
        self._message_model.appendMessage(
            "askUser", question, choices=choices
        )

    def _on_connected(self) -> None:
        self._set_connecting(False)
        self._connection_error = ""
        self.connectionErrorChanged.emit()
        self._message_model.appendMessage(
            "system", "Connected to AI assistant. How can I help?"
        )

    def _on_connection_failed(self, error: str) -> None:
        self._set_connecting(False)
        self._set_streaming(False)
        self._connection_error = error
        self.connectionErrorChanged.emit()

    def _on_idle(self) -> None:
        self._set_streaming(False)

    # -- Helpers ---------------------------------------------------------------

    def _set_streaming(self, value: bool) -> None:
        if self._is_streaming != value:
            self._is_streaming = value
            self.isStreamingChanged.emit()

    def _set_connecting(self, value: bool) -> None:
        if self._is_connecting != value:
            self._is_connecting = value
            self.isConnectingChanged.emit()
