# AI Chat Plugin — Segment 2a: Part 2 of 3

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build the Copilot SDK integration layer (CopilotWorker QThread, ChatBackend controller) and refactor the window to a tab-merged layout.

**Architecture:** `CopilotWorker` is a `QThread` running an `asyncio` event loop that hosts the Copilot SDK client.  `ChatBackend` is a main-thread `QObject` bridging QML ↔ worker via Qt signals/slots.  The existing `ActionDebuggerWindow.qml` is renamed to `PluginWindow.qml` and gets a `TabBar` + `StackLayout`.

**Tech Stack:** Python 3.13, PySide6 6.9, `github-copilot-sdk`, asyncio, QThread

**Spec:** `docs/superpowers/specs/2026-04-04-ai-chat-segment2a-chat-core-design.md`

**Depends on:** Part 1 (chat_message_model, tool_handlers, markdown_converter, prompts)

---

### Task 6: Create CopilotWorker

**Files:**
- Create: `plugins/ai_chat_plugin/copilot_worker.py`

- [ ] **Step 1: Implement `copilot_worker.py`**

Create file `plugins/ai_chat_plugin/copilot_worker.py`:

```python
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
```

- [ ] **Step 2: Verify import works**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -c "import sys; sys.path.insert(0, 'plugins'); from ai_chat_plugin.copilot_worker import CopilotWorker; print('OK')"
```

Expected: `OK` (no import errors, but does NOT start the thread).

- [ ] **Step 3: Commit**

```powershell
git add plugins/ai_chat_plugin/copilot_worker.py
git commit -m "feat(ai-chat): add CopilotWorker QThread for SDK integration

QThread with asyncio event loop hosting the Copilot SDK client.
Communicates via Qt signals (worker→main) and asyncio.Queue (main→worker).
Includes ask_user bridging with 5-minute timeout, new session support,
and structured error propagation.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: Create ChatBackend

**Files:**
- Create: `plugins/ai_chat_plugin/chat_backend.py`

- [ ] **Step 1: Implement `chat_backend.py`**

Create file `plugins/ai_chat_plugin/chat_backend.py`:

```python
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
```

- [ ] **Step 2: Verify import works**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -c "import sys; sys.path.insert(0, 'plugins'); from ai_chat_plugin.chat_backend import ChatBackend; print('OK')"
```

Expected: `OK` (needs QApplication at runtime, but import should work).

- [ ] **Step 3: Commit**

```powershell
git add plugins/ai_chat_plugin/chat_backend.py
git commit -m "feat(ai-chat): add ChatBackend controller bridging QML and SDK

Main-thread QObject owning ChatMessageModel and CopilotWorker.
Routes worker signals to model updates, handles Markdown conversion
on main thread, and exposes sendMessage/respondToAskUser/newSession
to QML.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: Refactor Window to Tab Layout

**Files:**
- Rename: `plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml` → `plugins/ai_chat_plugin/qml/PluginWindow.qml`
- Modify: `plugins/ai_chat_plugin/qml/PluginWindow.qml` (add TabBar + StackLayout)

- [ ] **Step 1: Rename the window file**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
Move-Item plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml plugins/ai_chat_plugin/qml/PluginWindow.qml
```

- [ ] **Step 2: Rewrite `PluginWindow.qml` with tab layout**

Replace the entire content of `plugins/ai_chat_plugin/qml/PluginWindow.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Tab-merged plugin window: Chat + Action Debugger.
 * Default tab is Chat (index 0).
 */
ApplicationWindow {
    id: window

    width: 1000
    height: 700
    title: qsTr("AI Chat — OpenGeoLab")
    color: PluginTheme.bg
    visible: true

    required property var backend      // DebuggerBackend (existing)
    required property var chatBackend  // ChatBackend (new)

    // Bind theme singleton to backend dark mode state.
    Binding {
        target: PluginTheme
        property: "darkMode"
        value: window.backend.isDark
    }

    // Sync dark mode to ChatBackend for Markdown conversion
    Binding {
        target: window.chatBackend
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

            TabBar {
                id: tabBar
                Layout.fillWidth: false
                background: Item {}

                TabButton {
                    text: qsTr("💬 Chat")
                    width: implicitWidth
                    font.pixelSize: 13

                    background: Rectangle {
                        color: tabBar.currentIndex === 0
                               ? PluginTheme.surfaceStrong
                               : "transparent"
                        radius: PluginTheme.radiusSmall
                        Behavior on color {
                            ColorAnimation { duration: PluginTheme.animFast }
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: tabBar.currentIndex === 0
                               ? PluginTheme.textPrimary
                               : PluginTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TabButton {
                    text: qsTr("🔧 Debugger")
                    width: implicitWidth
                    font.pixelSize: 13

                    background: Rectangle {
                        color: tabBar.currentIndex === 1
                               ? PluginTheme.surfaceStrong
                               : "transparent"
                        radius: PluginTheme.radiusSmall
                        Behavior on color {
                            ColorAnimation { duration: PluginTheme.animFast }
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: tabBar.currentIndex === 1
                               ? PluginTheme.textPrimary
                               : PluginTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // New Session button (only visible on Chat tab)
            ToolButton {
                visible: tabBar.currentIndex === 0
                icon.source: window.backend.iconPath + "/new_session.svg"
                icon.color: PluginTheme.textSecondary
                icon.width: 20
                icon.height: 20
                onClicked: window.chatBackend.newSession()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("New session")
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

            // Theme toggle
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

    StackLayout {
        anchors.fill: parent
        anchors.margins: PluginTheme.gap
        currentIndex: tabBar.currentIndex

        // Tab 0: Chat
        ChatPage {
            chatBackend: window.chatBackend
        }

        // Tab 1: Action Debugger
        ActionDebuggerPage {
            backend: window.backend
        }
    }
}
```

- [ ] **Step 3: Verify file structure**

```powershell
Get-ChildItem plugins/ai_chat_plugin/qml/ -Name
```

Expected: `PluginWindow.qml` present, `ActionDebuggerWindow.qml` absent.

- [ ] **Step 4: Commit**

```powershell
git add plugins/ai_chat_plugin/qml/PluginWindow.qml
git rm plugins/ai_chat_plugin/qml/ActionDebuggerWindow.qml 2>$null
git add -u plugins/ai_chat_plugin/qml/
git commit -m "refactor(ai-chat): rename window to PluginWindow with tab layout

Rename ActionDebuggerWindow.qml to PluginWindow.qml.  Add TabBar with
Chat (default) and Debugger tabs using StackLayout.  New required
chatBackend property for the Chat page.  Theme toggle and new session
button in the toolbar.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 9: Update Entry Points

**Files:**
- Modify: `plugins/ai_chat_plugin/__init__.py`
- Modify: `plugins/ai_chat_plugin/__main__.py`
- Modify: `plugins/ai_chat_plugin/_qml_setup.py`

- [ ] **Step 1: Update `__init__.py`**

Replace the content of `plugins/ai_chat_plugin/__init__.py`:

```python
"""AI Chat Plugin — Chat with AI powered by GitHub Copilot SDK.

Provides a tab-merged window with Chat (Copilot SDK conversation) and
Action Debugger (command browser/executor).
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
    """Show the AI Chat plugin window (Chat + Action Debugger tabs)."""
    from pathlib import Path

    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtWidgets import QApplication

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine

    from PySide6.QtQuickControls2 import QQuickStyle
    QQuickStyle.setStyle("Basic")

    backend = DebuggerBackend()
    chat_backend = ChatBackend()
    engine = QQmlApplicationEngine()

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.setInitialProperties({
        "backend": backend,
        "chatBackend": chat_backend,
    })
    engine.load(QUrl.fromLocalFile(str(qml_dir / "PluginWindow.qml")))

    if not engine.rootObjects():
        return {"ok": False, "message": "QML failed to load."}

    setup_engine(engine, backend)

    # Keep references alive — no explicit teardown (PySide6 handles it).
    engine._backend = backend
    engine._chat_backend = chat_backend
    _active_engines.append(engine)

    return {"ok": True, "message": "AI Chat launched."}
```

- [ ] **Step 2: Update `__main__.py`**

Replace the content of `plugins/ai_chat_plugin/__main__.py`:

```python
"""Standalone entry point: python -m ai_chat_plugin.

Launches the tab-merged AI Chat window with Chat and Action Debugger tabs.
When run from ``build/bin/plugins/``, the native pywrapper module is
discovered automatically so full schema and execute features are available.
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
    """Create QQmlApplicationEngine with both backends."""
    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtQuickControls2 import QQuickStyle

    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine

    QQuickStyle.setStyle("Basic")

    backend = DebuggerBackend()
    chat_backend = ChatBackend()
    engine = QQmlApplicationEngine()

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.setInitialProperties({
        "backend": backend,
        "chatBackend": chat_backend,
    })
    engine.load(QUrl.fromLocalFile(str(qml_dir / "PluginWindow.qml")))

    if not engine.rootObjects():
        print("Error: QML failed to load.", file=sys.stderr)
        sys.exit(1)

    setup_engine(engine, backend)

    # Keep references alive.
    engine._backend = backend
    engine._chat_backend = chat_backend
    return engine


def main() -> None:
    """Create a QApplication and show the AI Chat window."""
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

- [ ] **Step 3: Update `_qml_setup.py` (no changes needed for chat)**

The existing `_qml_setup.py` only sets up JSON highlighters for the Action Debugger page's named TextAreas.  The Chat page does not use `QSyntaxHighlighter` (it uses `TextArea(RichText)` with pre-rendered HTML).  **No changes required** to `_qml_setup.py`.

Verify it still works by confirming the objectName lookup is safe when Chat tab is active:

```powershell
# The existing _find_child_by_name is safe — it silently returns None for
# children not found, so when ChatPage is loaded first, the debugger
# TextAreas won't exist yet but no error occurs.
```

- [ ] **Step 4: Commit**

```powershell
git add plugins/ai_chat_plugin/__init__.py plugins/ai_chat_plugin/__main__.py
git commit -m "feat(ai-chat): update entry points for tab-merged window

Both hosted and standalone modes now create ChatBackend + DebuggerBackend,
set both as initial properties, and load PluginWindow.qml.  No explicit
teardown (same pattern as Segment 1 crash fix).

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

**End of Part 2.** Continue to Part 3 for QML chat components and integration verification.
