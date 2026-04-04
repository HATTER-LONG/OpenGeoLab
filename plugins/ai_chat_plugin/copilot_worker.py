"""QThread hosting the Copilot SDK in a dedicated asyncio event loop.

Runs CopilotClient and CopilotSession in a background thread.  Communicates
with the main thread exclusively through Qt signals (worker → main) and
thread-safe ``asyncio.Queue`` (main → worker).

The ``ask_user`` bridging uses ``threading.Event`` with a 5-minute timeout
to prevent deadlocks.
"""
from __future__ import annotations

import asyncio
import threading
import traceback
from pathlib import Path

from PySide6.QtCore import QThread, Signal, Slot


_PROMPTS_DIR = Path(__file__).resolve().parent / "prompts"
_ASK_USER_TIMEOUT = 300  # 5 minutes


def _load_prompt(filename: str) -> str:
    """Read a prompt file from the prompts/ directory."""
    path = _PROMPTS_DIR / filename
    if path.is_file():
        return path.read_text(encoding="utf-8")
    return ""


class CopilotWorker(QThread):
    """Background thread running the Copilot SDK asyncio event loop."""

    # Worker → main thread signals
    messageDelta = Signal(str)          # streaming text chunk
    messageComplete = Signal(str)       # final message (raw Markdown, NOT msgId)
    # Note: msgId is tracked by ChatBackend._active_msg_id, not passed through
    # this signal, because the SDK event doesn't carry frontend-generated IDs.
    # Only one message streams at a time (UI disables send), so this is safe.
    toolStarted = Signal(str, str)      # tool_name, tool_call_id
    toolCompleted = Signal(str, str, bool)  # tool_call_id, result, success
    askUserRequested = Signal(str, list)    # question, choices
    connectionReady = Signal()
    connectionFailed = Signal(str)      # error message
    sessionIdle = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._loop: asyncio.AbstractEventLoop | None = None
        self._queue: asyncio.Queue[str | None] | None = None
        self._shutdown = False

        # ask_user bridging state
        self._ask_user_event = threading.Event()
        self._ask_user_answer: str | None = None

    # -- Slots (main thread → worker) -----------------------------------------

    @Slot(str)
    def queueMessage(self, text: str) -> None:
        """Enqueue a user message from the main thread."""
        if self._loop is not None and self._queue is not None:
            self._loop.call_soon_threadsafe(self._queue.put_nowait, text)

    @Slot(str)
    def setAskUserResponse(self, answer: str) -> None:
        """Provide the user's response to an ask_user request."""
        self._ask_user_answer = answer
        self._ask_user_event.set()

    @Slot()
    def requestNewSession(self) -> None:
        """Signal the worker to tear down and recreate the SDK session."""
        self._shutdown = True
        self._ask_user_event.set()  # Unblock any waiting ask_user
        # Wake the message queue so _main() exits the await-get loop
        if self._loop is not None and self._queue is not None:
            self._loop.call_soon_threadsafe(self._queue.put_nowait, None)

    # -- QThread entry point ---------------------------------------------------

    def run(self) -> None:
        """Create an asyncio event loop and run the SDK main coroutine."""
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_until_complete(self._main())
        except Exception as exc:
            self.connectionFailed.emit(
                f"Worker crashed: {traceback.format_exc(limit=5)}"
            )
        finally:
            self._loop.close()
            self._loop = None

    # -- Internal async logic --------------------------------------------------

    async def _main(self) -> None:
        """Connect to Copilot SDK and process messages until shutdown."""
        try:
            from copilot import CopilotClient
            from copilot.client import SubprocessConfig
            from copilot.session import PermissionHandler, SystemMessageConfig
        except ImportError as exc:
            self.connectionFailed.emit(
                f"Copilot SDK not installed: {exc}\n"
                "Install with: pip install github-copilot-sdk"
            )
            return

        from ai_chat_plugin.tool_handlers import build_tools

        while not self._shutdown:
            self._queue = asyncio.Queue()
            try:
                async with CopilotClient(
                    SubprocessConfig(
                        log_level="warning",
                        use_logged_in_user=True,
                    ),
                    auto_start=True,
                ) as client:
                    tools = build_tools()
                    session = await client.create_session(
                        on_permission_request=PermissionHandler.approve_all,
                        tools=tools,
                        streaming=True,
                        system_message=SystemMessageConfig(
                            text=_load_prompt("system_prompt.md"),
                        ),
                        skill_directories=[str(_PROMPTS_DIR)],
                        on_event=self._on_event,
                        on_user_input_request=self._on_user_input,
                    )
                    self.connectionReady.emit()

                    # Process messages until shutdown
                    while not self._shutdown:
                        msg = await self._queue.get()
                        if msg is None:
                            break
                        await session.send(msg)
                        # Wait for session to become idle
                        # (events are handled via on_event callback)

            except Exception as exc:
                self.connectionFailed.emit(
                    f"SDK connection error: {traceback.format_exc(limit=5)}"
                )
                break

            # If requestNewSession was called, reset and loop
            if self._shutdown:
                self._shutdown = False
                # connectionReady will be emitted on successful reconnect

    def _on_event(self, event) -> None:
        """Handle SDK session events — emit Qt signals for main thread."""
        event_type = event.type.value if hasattr(event.type, "value") else str(event.type)

        match event_type:
            case "assistant.message_delta":
                delta = getattr(event.data, "delta_content", None) or ""
                self.messageDelta.emit(delta)
            case "assistant.message":
                content = getattr(event.data, "content", None) or ""
                self.messageComplete.emit(content)
            case "tool.execution_start":
                name = getattr(event.data, "tool_name", "unknown")
                call_id = getattr(event.data, "tool_call_id", "")
                self.toolStarted.emit(name, call_id)
            case "tool.execution_complete":
                call_id = getattr(event.data, "tool_call_id", "")
                result = str(getattr(event.data, "result", ""))
                result_type = getattr(event.data, "result_type", "error")
                success = result_type == "success"
                self.toolCompleted.emit(call_id, result, success)
            case "session.idle":
                self.sessionIdle.emit()
            case "session.error":
                self.connectionFailed.emit(str(event.data))

    async def _on_user_input(self, request, invocation) -> dict:
        """Handle SDK ask_user requests by bridging to the main thread."""
        question = request.get("question", "")
        choices = request.get("choices", [])

        self._ask_user_event.clear()
        self._ask_user_answer = None
        self.askUserRequested.emit(question, choices)

        # Block until main thread responds (5-min timeout)
        answered = await asyncio.get_event_loop().run_in_executor(
            None, lambda: self._ask_user_event.wait(timeout=_ASK_USER_TIMEOUT)
        )

        if not answered or self._ask_user_answer is None:
            return {"answer": "(no response)", "wasFreeform": True}
        return {"answer": self._ask_user_answer, "wasFreeform": True}
