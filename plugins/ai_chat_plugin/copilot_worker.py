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

# Map user-facing BYOK provider name → SDK provider type.
# Ollama and Custom both speak the OpenAI-compatible API.
_SDK_TYPE_MAP: dict[str, str] = {
    "openai": "openai",
    "azure": "azure",
    "anthropic": "anthropic",
    "ollama": "openai",
    "custom": "openai",
}


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
    reasoningDelta = Signal(str)        # streaming reasoning chunk
    reasoningComplete = Signal(str)     # final reasoning text
    toolStarted = Signal(str, str)      # tool_name, tool_call_id
    toolCompleted = Signal(str, str, bool)  # tool_call_id, result, success
    askUserRequested = Signal(str, list)    # question, choices
    modelsLoaded = Signal(str)              # JSON-encoded model list
    connectionReady = Signal()
    connectionFailed = Signal(str)      # error message
    sessionIdle = Signal()

    def __init__(self, config, parent=None) -> None:
        super().__init__(parent)
        self._config = config
        self._loop: asyncio.AbstractEventLoop | None = None
        self._queue: asyncio.Queue[str | None] | None = None
        self._shutdown = False
        self._terminate = False  # permanent exit (vs session restart)

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

    def requestShutdown(self) -> None:
        """Permanently stop the worker thread (app exit)."""
        self._terminate = True
        self._shutdown = True
        self._ask_user_event.set()
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
            from copilot.session import PermissionHandler, SystemMessageReplaceConfig
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
                # Build SubprocessConfig — BYOK may not need GitHub login
                subprocess_kwargs: dict = {"log_level": "warning"}
                if self._config.authMethod != "byok":
                    subprocess_kwargs["use_logged_in_user"] = True

                async with CopilotClient(
                    SubprocessConfig(**subprocess_kwargs),
                    auto_start=True,
                ) as client:
                    # Fetch available models (non-blocking, cached by SDK)
                    await self._fetch_models(client)

                    tools = build_tools()
                    session_kwargs: dict = {
                        "on_permission_request": PermissionHandler.approve_all,
                        "tools": tools,
                        "streaming": True,
                        "system_message": SystemMessageReplaceConfig(
                            mode="replace",
                            content=_load_prompt("system_prompt.md"),
                        ),
                        "skill_directories": [str(_PROMPTS_DIR)],
                        "on_event": self._on_event,
                        "on_user_input_request": self._on_user_input,
                    }

                    if self._config.authMethod == "byok":
                        provider_config: dict = {
                            "type": _SDK_TYPE_MAP.get(
                                self._config.byokProvider, "openai",
                            ),
                            "base_url": self._config.byokBaseUrl,
                        }
                        if self._config.byokApiKey:
                            provider_config["api_key"] = self._config.byokApiKey
                        if self._config.byokProvider in ("openai", "azure"):
                            provider_config["wire_api"] = self._config.byokWireApi
                        session_kwargs["provider"] = provider_config
                        session_kwargs["model"] = self._config.byokModel
                    else:
                        if self._config.lastModel:
                            session_kwargs["model"] = self._config.lastModel

                    session = await client.create_session(**session_kwargs)
                    self.connectionReady.emit()

                    while not self._shutdown:
                        msg = await self._queue.get()
                        if msg is None:
                            break
                        await session.send(msg)

            except Exception as exc:
                self.connectionFailed.emit(
                    f"SDK connection error: {traceback.format_exc(limit=5)}"
                )
                break

            # If permanent shutdown requested, exit the thread
            if self._terminate:
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
                # event.data.result is a Result object with .content / .detailed_content
                raw_result = getattr(event.data, "result", None)
                if raw_result is not None:
                    result = (
                        getattr(raw_result, "detailed_content", None)
                        or getattr(raw_result, "content", None)
                        or ""
                    )
                else:
                    result = str(getattr(event.data, "error", ""))
                success = bool(getattr(event.data, "success", False))
                self.toolCompleted.emit(call_id, result, success)
            case "session.idle":
                self.sessionIdle.emit()
            case "session.error":
                self.connectionFailed.emit(str(event.data))
            case "assistant.reasoning_delta":
                delta = getattr(event.data, "delta_content", None) or ""
                if delta:
                    self.reasoningDelta.emit(delta)
            case "assistant.reasoning":
                text = getattr(event.data, "reasoning_text", None) or ""
                if text:
                    self.reasoningComplete.emit(text)

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

    async def _fetch_models(self, client) -> None:
        """Fetch available models from the SDK and emit modelsLoaded signal."""
        import json

        try:
            raw_models = await client.list_models()
            result: list[dict] = []
            for m in raw_models:
                entry: dict = {
                    "id": getattr(m, "id", ""),
                    "name": getattr(m, "name", ""),
                }
                caps = getattr(m, "capabilities", None)
                if caps:
                    supports = getattr(caps, "supports", None)
                    entry["vision"] = bool(
                        getattr(supports, "vision", False)
                    ) if supports else False
                billing = getattr(m, "billing", None)
                if billing:
                    entry["multiplier"] = getattr(billing, "multiplier", 1)
                policy = getattr(m, "policy", None)
                if policy:
                    entry["state"] = getattr(policy, "state", "enabled")
                else:
                    entry["state"] = "enabled"
                result.append(entry)
            # Emit as JSON string to avoid cross-thread Python object issues
            self.modelsLoaded.emit(json.dumps(result))
        except Exception:
            # Model listing is best-effort; don't block connection
            pass
