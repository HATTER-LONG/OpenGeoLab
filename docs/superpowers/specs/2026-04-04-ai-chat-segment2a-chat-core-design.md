# AI Chat Plugin — Segment 2a: Chat Core & Tools

## Problem Statement

Segment 1 delivered an Action Debugger that lets users browse and manually
execute OpenGeoLab commands.  Segment 2a adds the conversational layer: a chat
interface powered by the GitHub Copilot SDK where users type natural-language
requests, the AI discovers available commands via custom tools, executes them,
and reports results — all within a streaming chat UI.

## Spec Reference

Parent spec: `docs/superpowers/specs/2026-04-03-ai-chat-plugin-design.md`.

This document covers **Segment 2a** only (Chat Core + Tools).  Auth settings,
BYOK providers, and model selection are deferred to Segment 2b.

---

## Design Decisions (from brainstorm)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| UI framework | QML | Consistent with Segment 1 |
| AI backend | Copilot SDK (`github-copilot-sdk`) | Per original spec |
| Async strategy | QThread + asyncio event loop | Avoids qasync complexity in embedded runtime |
| Markdown rendering | Python `markdown` → HTML → QML `TextArea(textFormat: RichText)` | Lightweight, no WebEngine dependency |
| Window structure | Tab-merged with Action Debugger | Single plugin window, `TabBar` switching Chat / Debugger |
| Scope | Chat Core + 4 Tools | Auth / BYOK / Model Selector deferred to Segment 2b |

---

## Architecture Overview

```
┌────────────────────── QML (Main Thread) ──────────────────────┐
│  PluginWindow.qml (ApplicationWindow)                          │
│   ├─ TabBar: ["💬 Chat", "🔧 Action Debugger"]               │
│   ├─ StackLayout                                               │
│   │   ├─ ChatPage.qml ─── ListView + message delegates        │
│   │   └─ ActionDebuggerPage.qml (existing, unchanged)         │
│   └─ Toolbar (theme toggle, new session button)                │
├───────────────── ChatBackend (QObject, main thread) ───────────┤
│   owns: ChatMessageModel (QAbstractListModel)                  │
│   owns: CopilotWorker (QThread)                                │
│   bridges: QML ↔ worker via Signal/Slot                        │
├───────────────── CopilotWorker (QThread) ──────────────────────┤
│   runs:  asyncio.new_event_loop()                              │
│   owns:  CopilotClient + CopilotSession                       │
│   tools: list_modules, describe_module, execute_action         │
│   ask_user → Signal → main thread → user responds → Event     │
└────────────────────────────────────────────────────────────────┘
```

### Data Flow

1. User types message in `ChatInputArea` → `ChatBackend.sendMessage(text)`
2. `ChatBackend` appends a **user** row to `ChatMessageModel`, emits signal to
   `CopilotWorker`
3. Worker calls `await session.send(text)` in its asyncio loop
4. SDK sends to Copilot CLI subprocess (JSON-RPC over stdio)
5. Streaming events flow back:
   - `assistant.message_delta` → `Worker.messageDelta` signal → ChatBackend
     appends text to current assistant row
   - `tool.execution_start` → `Worker.toolStarted` signal → ChatBackend
     appends a **tool** row
   - Tool handler runs (e.g. `pywrapper.process()`) → result returned to SDK
   - `tool.execution_complete` → `Worker.toolCompleted` signal → ChatBackend
     updates tool row status
   - `assistant.message` (final) → Worker converts Markdown→HTML →
     `Worker.messageComplete` signal → ChatBackend replaces raw text with HTML
6. If SDK invokes `ask_user` tool:
   - Worker emits `askUserRequested(question, choices)` signal
   - ChatBackend appends an **askUser** row
   - QML shows interactive panel (choice buttons + freeform input)
   - User responds → `ChatBackend.respondToAskUser(answer)` slot
   - Worker's `threading.Event` is set → tool handler returns answer to SDK
7. `session.idle` event → ChatBackend sets `isStreaming = false`

---

## File Map

```
plugins/ai_chat_plugin/
├── __init__.py              # MODIFIED: launch tab-merged window
├── __main__.py              # MODIFIED: standalone entry for tab window
├── _qml_setup.py            # MODIFIED: setup both Chat + Debugger highlighters
├── chat_backend.py          # NEW: ChatBackend QObject controller
├── chat_message_model.py    # NEW: QAbstractListModel for messages
├── copilot_worker.py        # NEW: QThread running Copilot SDK
├── tool_handlers.py         # NEW: 4 tool definitions
├── markdown_converter.py    # NEW: Markdown → HTML with theme-aware CSS
├── debugger_backend.py      # EXISTING (unchanged)
├── param_list_model.py      # EXISTING (unchanged)
├── schema_tree_model.py     # EXISTING (unchanged)
├── json_highlighter.py      # EXISTING (unchanged)
├── scene_tools.py           # EXISTING (unchanged)
├── prompts/
│   ├── skill.md             # NEW: AI skill document
│   └── system_prompt.md     # NEW: system prompt (role, constraints)
└── qml/
    ├── PluginWindow.qml     # RENAMED from ActionDebuggerWindow.qml + tabs
    ├── ChatPage.qml         # NEW: chat page layout
    ├── MessageDelegate.qml  # NEW: message bubble delegate
    ├── ToolCallCard.qml     # NEW: collapsible tool call card
    ├── AskUserPanel.qml     # NEW: ask_user interaction widget
    ├── ChatInputArea.qml    # NEW: text input + send button
    ├── ActionDebuggerPage.qml  # EXISTING (unchanged)
    ├── DetailPanel.qml         # EXISTING (unchanged)
    ├── ParamForm.qml           # EXISTING (unchanged)
    ├── RequestResponseView.qml # EXISTING (unchanged)
    ├── SchemaTreeView.qml      # EXISTING (unchanged)
    └── theme/
        ├── PluginTheme.qml     # EXISTING (unchanged)
        └── qmldir              # EXISTING (unchanged)
```

---

## Component Specifications

### 1. PluginWindow.qml (refactored from ActionDebuggerWindow.qml)

Adds a `TabBar` + `StackLayout` wrapping the existing Action Debugger page and
the new Chat page.  Both pages share the same `backend` (DebuggerBackend) and a
new `chatBackend` (ChatBackend) property.

```
ApplicationWindow {
    required property var backend      // DebuggerBackend (existing)
    required property var chatBackend  // ChatBackend (new)

    header: ToolBar {
        RowLayout {
            TabBar { id: tabBar; Tab("💬 Chat"); Tab("🔧 Debugger") }
            Item { Layout.fillWidth: true }
            // theme toggle, new session button
        }
    }

    StackLayout {
        currentIndex: tabBar.currentIndex
        ChatPage { chatBackend: root.chatBackend }
        ActionDebuggerPage { backend: root.backend }
    }
}
```

Default tab is **Chat** (index 0).

### 2. ChatMessageModel (`chat_message_model.py`)

`QAbstractListModel` with the following roles:

| Role name | Type | Description |
|-----------|------|-------------|
| `msgId` | `str` | Unique message ID (UUID or SDK-provided message ID) |
| `msgType` | `str` | `"user"` / `"assistant"` / `"tool"` / `"askUser"` / `"system"` |
| `content` | `str` | Raw text (streaming) or final HTML (after Markdown conversion) |
| `toolName` | `str` | Tool name (only for `tool` type) |
| `toolCallId` | `str` | SDK tool_call_id (only for `tool` type, used for updates) |
| `toolStatus` | `str` | `"running"` / `"success"` / `"error"` (only for `tool` type) |
| `toolResult` | `str` | Tool result summary (only for `tool` type) |
| `choices` | `list[str]` | Choice buttons (only for `askUser` type) |
| `answered` | `bool` | Whether the user has responded (only for `askUser` type) |
| `isHtml` | `bool` | `True` when content has been converted to HTML |

**Streaming state machine** for assistant messages:

1. `appendMessage("assistant", "")` → creates row, `isHtml = False`
2. `appendToLastAssistant(delta)` → appends raw text to the **same row**
   (identified by `msgId`); QML shows raw text + blinking `▌` cursor
3. `finalizeAssistant(msgId, html)` → replaces `content` with HTML **in-place**
   on the same row, sets `isHtml = True`; QML switches from raw to rich display

Row identification uses `msgId`, not last-index.  `appendToLastAssistant()`
finds the row by scanning from the end for the matching ID.  This prevents
race conditions if tool rows are interleaved during streaming.

Key methods:

- `appendMessage(msg_type, content, **kwargs)` → appends a new row, returns row index
- `appendToLastAssistant(delta)` → appends delta text to the active assistant row
- `finalizeAssistant(msg_id, html)` → replaces content with HTML, sets `isHtml = True`
- `updateToolStatus(tool_call_id, status, result)` → finds tool row by `toolCallId` and updates
- `markAskUserAnswered(row)` → sets `answered = True` for an askUser row
- `clear()` → removes all rows (for new session)

### 3. ChatBackend (`chat_backend.py`)

`QObject` controller bridging QML ↔ CopilotWorker.

**Properties (exposed to QML):**

| Property | Type | Description |
|----------|------|-------------|
| `messageModel` | `QObject` | ChatMessageModel instance (constant) |
| `isConnecting` | `bool` | True while SDK is initializing |
| `isStreaming` | `bool` | True while AI is responding |
| `connectionError` | `str` | Error message if connection fails |

**Invokable methods:**

| Method | Description |
|--------|-------------|
| `sendMessage(text: str)` | Append user message, forward to worker |
| `respondToAskUser(answer: str)` | Forward user's answer to worker |
| `newSession()` | Clear model, signal worker to create fresh session |
| `retryConnection()` | Re-attempt SDK connection |

**`newSession()` cleanup sequence:**
1. Set `isStreaming = False`
2. Call `model.clear()` to remove all rows
3. Signal `requestNewSession` to worker
4. Worker sets `_shutdown = True` → unblocks any pending `_ask_user_event` →
   exits the message-processing loop → closes the current `CopilotClient` →
   creates a new `CopilotClient` and `Session` → resets `_shutdown = False`
5. Worker emits `connectionReady` when the new session is established

The old `CopilotClient` is closed via `async with` context manager exit, which
terminates the CLI subprocess cleanly.  No orphaned threads or connections.

**Internal wiring:**

- Constructor creates `ChatMessageModel` and `CopilotWorker`
- Connects worker signals to model-update slots
- `sendMessage` appends user row, then creates an assistant row with a unique
  `msgId`, and forwards the text + `msgId` to the worker
- Worker's `messageDelta` → `_onDelta(delta)` → `model.appendToLastAssistant(delta)`
- Worker's `messageComplete(msgId, html)` → `model.finalizeAssistant(msgId, html)`
- Worker's `toolStarted(name, toolCallId)` → append tool row with `toolCallId`
- Worker's `toolCompleted(toolCallId, result, success)` → `model.updateToolStatus(toolCallId, ...)`
- Worker's `askUserRequested` → append askUser row
- `respondToAskUser` → signal to worker → `threading.Event.set()`

### 4. CopilotWorker (`copilot_worker.py`)

`QThread` subclass that hosts the Copilot SDK in a dedicated asyncio event loop.

**Signals (worker → main thread):**

| Signal | Parameters | When |
|--------|------------|------|
| `messageDelta` | `str` | Each streaming text chunk |
| `messageComplete` | `str` | Final message as HTML |
| `toolStarted` | `str, str` | tool_name, tool_call_id |
| `toolCompleted` | `str, str, bool` | tool_call_id, result_text, success |
| `askUserRequested` | `str, list` | question, choices |
| `connectionReady` | — | SDK connected successfully |
| `connectionFailed` | `str` | Error message |
| `sessionIdle` | — | Session finished processing |

**Slots (main thread → worker):**

| Slot | Parameters | Description |
|------|------------|-------------|
| `queueMessage` | `str` | Enqueue a user message to send |
| `setAskUserResponse` | `str` | Provide the user's ask_user answer |
| `requestNewSession` | — | Disconnect current session, create new one |

**Internal design:**

```python
class CopilotWorker(QThread):
    def run(self):
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)
        self._loop.run_until_complete(self._main())

    async def _main(self):
        async with CopilotClient() as client:
            self.connectionReady.emit()
            self._session = await client.create_session(
                on_permission_request=PermissionHandler.approve_all,
                tools=build_tools(self),
                streaming=True,
                system_message=SystemMessageConfig(text=load_system_prompt()),
                skill_directories=[str(prompts_dir)],
                on_event=self._on_event,
                on_user_input_request=self._on_user_input_request,
            )
            # Process message queue until shutdown
            while not self._shutdown:
                msg = await self._get_next_message()
                if msg:
                    await self._session.send(msg)
```

**ask_user bridging:**

```python
# In worker thread (called by SDK when AI uses ask_user):
async def _on_user_input_request(self, request, invocation):
    question = request.get("question", "")
    choices = request.get("choices", [])
    self._ask_user_event.clear()   # Clear BEFORE emitting signal
    self._ask_user_answer = None
    self.askUserRequested.emit(question, choices)
    # Block until main thread provides answer, with 5-minute timeout
    answered = await asyncio.get_event_loop().run_in_executor(
        None, lambda: self._ask_user_event.wait(timeout=300)
    )
    if not answered or self._ask_user_answer is None:
        return {"answer": "(no response)", "wasFreeform": True}
    return {"answer": self._ask_user_answer, "wasFreeform": True}

# In main thread (slot):
@Slot(str)
def setAskUserResponse(self, answer):
    self._ask_user_answer = answer
    self._ask_user_event.set()
```

**Race condition prevention:** `_ask_user_event.clear()` is called **before**
`askUserRequested.emit()`, so even if the main thread calls `setAskUserResponse`
before `wait()` starts, the event will be set and `wait()` returns immediately.

**Timeout:** 5-minute timeout prevents permanent hangs if user closes the window
or navigates away.  On timeout, a fallback "(no response)" is returned to the SDK.

**New session during ask_user:** `requestNewSession` sets `_shutdown = True` and
also sets `_ask_user_event` to unblock any waiting request.

**Message queue:**

Uses `asyncio.Queue` (unbounded) in the worker loop.  The `queueMessage` slot is
called from the main thread; it uses `loop.call_soon_threadsafe(queue.put_nowait, msg)`
to safely enqueue from a different thread.  Users cannot send new messages while
`isStreaming` is True (UI disables the send button), so queue flooding is not a concern.

### 5. Tool Handlers (`tool_handlers.py`)

Four tools registered with the SDK session, all using `@define_tool`:

```python
from pydantic import BaseModel, Field
from copilot import define_tool
from copilot.tools import ToolInvocation
from ai_chat_plugin import scene_tools

class DescribeModuleParams(BaseModel):
    module_name: str = Field(description="Name of the module to describe")

class ExecuteActionParams(BaseModel):
    module: str = Field(description="Module name")
    action: str = Field(description="Action name")
    params: dict = Field(default_factory=dict, description="Action parameters")

@define_tool(description="List all available OpenGeoLab command modules",
             skip_permission=True)
def list_modules() -> str:
    modules = scene_tools.list_modules()
    return json.dumps(modules, default=str)

@define_tool(description="Describe a module's actions and their parameter schemas",
             skip_permission=True)
def describe_module(params: DescribeModuleParams) -> str:
    result = scene_tools.describe_module(params.module_name)
    return json.dumps(result, default=str)

@define_tool(description="Execute an OpenGeoLab action with given parameters")
def execute_action(params: ExecuteActionParams) -> str:
    result = scene_tools.execute_action(
        params.module, params.action, params.params
    )
    return json.dumps(result, default=str)
```

**Tool return type:** All tools return `str` (JSON-serialized).  The `default=str`
parameter in `json.dumps()` ensures non-serializable values are coerced to strings
rather than crashing the worker thread.  Tool handlers must **never** raise
unhandled exceptions; if `scene_tools` raises, catch and return an error JSON:
`{"ok": false, "error": "<traceback summary>"}`.

**Tool result truncation:** Results exceeding 50 KB are truncated to the first
50 KB with a trailing `"...(truncated)"` suffix.  This prevents excessively large
results from overwhelming the SDK or QML display.

The `ask_user` tool is handled by the SDK's built-in `on_user_input_request`
callback — no custom tool definition needed.

**Standalone mode:** When `scene_tools.HOSTED_MODE` is `False`, `list_modules`,
`describe_module`, and `execute_action` are omitted from the tools list.
The `ask_user` callback is always registered.

### 6. Markdown Converter (`markdown_converter.py`)

Converts Markdown text to themed HTML suitable for QML `TextArea(RichText)`.

```python
import markdown

def markdown_to_html(text: str, dark_mode: bool = True) -> str:
    """Convert Markdown to themed HTML fragment.

    Uses the ``fenced_code`` and ``tables`` extensions.  Wraps output
    in a ``<style>`` block with colours derived from *dark_mode*.
    Input Markdown is NOT pre-sanitized; the ``markdown`` library
    escapes raw HTML by default (no ``html`` extension enabled), so
    embedded ``<script>`` or ``<img onerror>`` tags are rendered as
    literal text, preventing XSS.
    """
    html_body = markdown.markdown(
        text,
        extensions=["fenced_code", "tables"],
    )
    css = _build_css(dark_mode)
    return f"<style>{css}</style>{html_body}"
```

**Calling context:** `markdown_to_html()` is called on the **main thread** inside
`ChatBackend._onComplete()`, not in the worker.  The `dark_mode` parameter is
read from `ChatBackend._dark_mode` (a Python `bool` synced from QML's
`PluginTheme.darkMode` property).  Theme changes do **not** re-render past
messages; only new `messageComplete` events use the current theme value.

CSS adapts to `PluginTheme.darkMode`:
- Body: transparent background, theme text color
- Code blocks: `<pre>` with monospace font, subtle background
- Inline code: backtick style with background highlight
- Links: accent color
- Tables: bordered with alternating row colors

### 7. Prompt Files (`prompts/`)

**`prompts/system_prompt.md`** — System prompt setting the AI's role:

```markdown
You are the OpenGeoLab AI Assistant. You help users interact with the
OpenGeoLab CAD application through natural language.

You have access to tools that let you discover and execute commands:
1. Use `list_modules` to see available modules
2. Use `describe_module` to learn about a module's actions
3. Use `execute_action` to run commands

Rules:
- Always discover actions before executing them (never guess names)
- Confirm destructive operations with the user before executing
- Report results clearly and suggest next steps
```

**`prompts/skill.md`** — Skill document teaching the AI the command protocol:

```markdown
# OpenGeoLab Command Protocol

## Request Format
{module, action, param} → {ok, result/errors}

## Discovery Workflow
1. list_modules() → module names
2. describe_module(name) → actions with param/return schemas
3. execute_action(module, action, params) → result

## Available Modules (discovered at runtime)
Use list_modules to discover. Common modules:
- geometry: Create shapes (create_box, create_cylinder, etc.)
- mesh: Generate meshes from geometry
- scene: Selection, camera, labels
- io: Import/export files

## Constraints
- Parameter names and types must match the schema exactly
- shapeId values come from geometry creation results
- Entity types: GeoSolid, GeoFace, GeoEdge, GeoVertex
```

### 8. QML Components

#### ChatPage.qml

Main chat page layout:

```
ColumnLayout {
    spacing: 0

    // Message list — takes all available space
    ListView {
        id: messageList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: chatBackend.messageModel
        clip: true
        spacing: 8
        verticalLayoutDirection: ListView.TopToBottom

        delegate: Loader {
            width: ListView.view.width
            sourceComponent: {
                switch (model.msgType) {
                    case "user":      return userDelegate
                    case "assistant": return assistantDelegate
                    case "tool":      return toolDelegate
                    case "askUser":   return askUserDelegate
                    case "system":    return systemDelegate
                }
            }
        }

        // Auto-scroll to bottom on new messages
        onCountChanged: positionViewAtEnd()
    }

    // Streaming / connection status bar
    Rectangle { /* typing indicator, connection status */ }

    // Input area
    ChatInputArea {
        Layout.fillWidth: true
        enabled: !chatBackend.isConnecting
        onSendMessage: function(text) { chatBackend.sendMessage(text) }
    }
}
```

#### MessageDelegate.qml

Per-message visual representation.  Uses `Loader` to switch between components
based on `msgType`.

**User bubble:**
- Right-aligned rounded rectangle, dark surface color
- `Text` with user's raw text, wrapping enabled

**Assistant bubble:**
- Left-aligned, light/transparent background
- `TextArea { readOnly: true; textFormat: TextEdit.RichText }` for HTML content
- During streaming (`isHtml == false`): shows raw text + blinking `▌` cursor
- After complete (`isHtml == true`): shows formatted HTML

**System message:**
- Centered, muted text, no bubble

#### ToolCallCard.qml

Collapsible card showing tool execution:

```
Rectangle {
    radius: 8; border: 1px theme.border
    ColumnLayout {
        RowLayout {
            // Status icon (spinner / ✓ / ✗)
            Label { text: model.toolName; font.bold: true }
            Label { text: model.toolStatus; color: statusColor }
            // Expand/collapse toggle
        }
        // Collapsed by default; expand shows toolResult JSON
        TextArea {
            visible: expanded
            text: model.toolResult
            readOnly: true
            font.family: "monospace"
        }
    }
}
```

#### AskUserPanel.qml

Interactive panel when AI asks the user a question:

```
Rectangle {
    border.style: Qt.DashLine  // Dashed border
    color: PluginTheme.surfaceStrong

    ColumnLayout {
        Label { text: model.content }  // Question text

        // Choice buttons (if choices provided)
        // Single-select: clicking any button immediately sends that choice
        // and disables the panel (no multi-select).
        Flow {
            Repeater {
                model: choices
                Button {
                    text: modelData
                    onClicked: chatBackend.respondToAskUser(modelData)
                }
            }
        }

        // Freeform input (always available alongside choices)
        // If user types and sends freeform text, it is used instead of
        // any choice button.  Only one response is sent per panel.
        RowLayout {
            TextField { id: freeformInput }
            Button {
                text: qsTr("Send")
                enabled: freeformInput.text.trim().length > 0
                onClicked: chatBackend.respondToAskUser(freeformInput.text)
            }
        }
    }
}
```

Disabled after user responds (`model.answered == true`).  Clicking a choice
button OR sending freeform text both call `respondToAskUser()` once and mark
the row as answered — only the first response is accepted.

#### ChatInputArea.qml

Text input area at the bottom:

```
Rectangle {
    color: PluginTheme.surface
    RowLayout {
        TextArea {
            id: inputField
            placeholderText: qsTr("Ask OpenGeoLab AI...")
            wrapMode: TextEdit.Wrap
            Keys.onReturnPressed: {
                if (!(event.modifiers & Qt.ShiftModifier)) {
                    sendMessage(inputField.text)
                    inputField.clear()
                    event.accepted = true
                }
            }
        }
        Button {
            text: "➤"
            enabled: inputField.text.trim().length > 0
            onClicked: {
                sendMessage(inputField.text)
                inputField.clear()
            }
        }
    }
    signal sendMessage(string text)
}
```

- Enter sends, Shift+Enter inserts newline
- Send button disabled when input is empty

---

## Entry Point Changes

### `__init__.py`

`launch_ui()` now creates both `DebuggerBackend` and `ChatBackend`, sets both
as initial properties, and loads `PluginWindow.qml`:

```python
backend = DebuggerBackend()
chat_backend = ChatBackend()
engine = QQmlApplicationEngine()
engine.setInitialProperties({
    "backend": backend,
    "chatBackend": chat_backend,
})
engine.load(QUrl.fromLocalFile(str(qml_dir / "PluginWindow.qml")))
```

### `__main__.py`

Standalone mode creates the same tab window.  `ChatBackend` connects without
scene tools (tools list is empty, only `ask_user` via SDK callback).

---

## Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| `github-copilot-sdk` | latest | Copilot SDK Python client |
| `markdown` | ≥ 3.6 | Markdown → HTML conversion |
| `pydantic` | ≥ 2.0 | Tool parameter schemas (SDK dependency) |

These must be installed into `pyvenv/`.  Installation should be added to the
CMake `SetupPySideVenv.cmake` script or documented as a manual step.

**Note:** `qasync` is NOT required (we use QThread instead).

---

## Copilot SDK Integration Details

### CopilotClient Configuration

```python
from copilot import CopilotClient
from copilot.client import SubprocessConfig

client = CopilotClient(
    SubprocessConfig(
        log_level="warning",
        use_logged_in_user=True,  # Use GitHub CLI credentials
    ),
    auto_start=True,
)
```

### Session Configuration

```python
from copilot.session import PermissionHandler, SystemMessageConfig

session = await client.create_session(
    on_permission_request=PermissionHandler.approve_all,
    tools=tools_list,
    streaming=True,
    system_message=SystemMessageConfig(
        text=load_prompt("system_prompt.md"),
    ),
    skill_directories=[str(prompts_dir)],
    on_event=self._on_event,
    on_user_input_request=self._on_user_input,
)
```

**`system_message` vs `skill_directories`:** The `system_message` sets the
baseline AI persona and rules.  The `skill_directories` point to `prompts/`
which contains `skill.md` — the SDK reads skill files automatically and makes
them available as additional context.  There is no conflict: the system prompt
defines "who you are", and the skill document provides domain-specific knowledge
about OpenGeoLab's capabilities.  `prompts/` may contain multiple `.md` files;
the SDK auto-discovers all of them.

### Event Handling

```python
def _on_event(self, event):
    match event.type.value:
        case "assistant.message_delta":
            self.messageDelta.emit(event.data.delta_content or "")
        case "assistant.message":
            # markdown_to_html is called on main thread (see ChatBackend wiring),
            # NOT here.  Worker emits raw Markdown text.
            self.messageComplete.emit(event.data.content or "")
        case "tool.execution_start":
            self.toolStarted.emit(
                event.data.tool_name, event.data.tool_call_id
            )
        case "tool.execution_complete":
            # event.data.result is already a string (tool handlers return str).
            # result_type is "success" or "error" — no other values.
            result_str = str(event.data.result) if event.data.result else ""
            success = getattr(event.data, "result_type", "error") == "success"
            self.toolCompleted.emit(
                event.data.tool_call_id, result_str, success
            )
        case "session.idle":
            self.sessionIdle.emit()
        case "session.error":
            self.connectionFailed.emit(str(event.data))
```

**Note:** `messageComplete` emits raw Markdown text (not HTML).  The HTML
conversion happens on the main thread in `ChatBackend._onComplete()`, which
calls `markdown_to_html(text, self._dark_mode)` before updating the model.
This avoids threading issues with the `dark_mode` parameter.

---

## Error Handling

| Scenario | Behavior |
|----------|----------|
| Copilot CLI not found | `connectionFailed("Copilot CLI not found…")` → QML shows banner with install instructions + retry button |
| Auth failure | `connectionFailed("Authentication failed…")` → banner with "please sign in" message + retry button |
| Network timeout | `session.error` event → `ChatBackend` appends a `system` message row: "⚠ Connection interrupted. Please try again." |
| Tool execution error | Tool handler catches exception → returns `{"ok": false, "error": "<message>"}` → AI receives error text and can report it |
| SDK process crash | `_main()` catches `Exception` at top-level → emits `connectionFailed(traceback_summary)` → worker thread exits cleanly |
| Streaming interrupted | Partial raw text preserved in the assistant row (not finalized to HTML); a `system` message is appended: "⚠ Response interrupted." |
| ask_user timeout | After 5 min with no response, returns `"(no response)"` to SDK (see ask_user bridging above) |

**Error propagation pattern:** All errors in CopilotWorker are caught inside
`_main()` with a broad `except Exception as exc:` → the traceback is logged via
`print()` and `connectionFailed.emit(str(exc))` is called.  Individual tool
handler exceptions are caught inside each `@define_tool` function and converted
to error JSON.  Errors never silently crash the worker thread.

---

## Scope Boundaries

### In Scope (Segment 2a)

- Tab-merged plugin window (Chat + Action Debugger)
- Chat message list with user/assistant/tool/askUser delegates
- Copilot SDK integration via QThread + asyncio
- 4 custom tools (list_modules, describe_module, execute_action, ask_user)
- Markdown → HTML rendering for AI responses
- Streaming display with blinking cursor
- Skill document and system prompt
- New session button
- Hosted + standalone modes
- Python dependency installation (`github-copilot-sdk`, `markdown`)

### Out of Scope (Segment 2b+)

- Authentication settings dialog (GitHub + BYOK)
- Model selector dropdown
- Session persistence
- Code syntax highlighting in Markdown code blocks
- Image/vision input
- Voice input/output

---

## Testing Strategy

### Standalone Smoke Test

```bash
cd build/bin/plugins
python -m ai_chat_plugin
```

Verify: window opens with Chat tab active, input area visible, can switch to
Debugger tab.  If Copilot CLI is not installed, connection error banner is shown
but no crash.

### Hosted Mode Test

Launch OpenGeoLab → AI Chat plugin → verify:

1. Chat tab opens by default
2. Type "list all available modules" → AI uses `list_modules` tool → reports
   modules
3. Type "describe the geometry module" → AI uses `describe_module` → reports
   actions
4. Type "create a box at origin with size 2" → AI uses `execute_action` →
   geometry appears
5. Streaming: text appears incrementally during AI response
6. Tool cards: show running/success status
7. Switch to Debugger tab → existing functionality works unchanged
8. Close app → no crash (no-teardown pattern preserved)
