# AI Chat Plugin — Design Specification

## Problem Statement

OpenGeoLab needs an AI assistant that allows users to interact with the
application through natural language. The assistant should be able to discover
and execute available commands, query scene state, and guide users through
complex workflows — all via a conversational interface powered by the GitHub
Copilot SDK.

## Proposed Approach

Build **ai_chat_plugin** as a standard Python plugin following the existing
plugin architecture. The plugin creates a PySide6 chat window, uses the Copilot
SDK Python client for AI interactions, and bridges to the C++ command system via
`opengeolab_pywrapper`. The design supports two launch modes: **hosted** (from
QML main application) and **standalone** (independent Python execution).

---

## Architecture Overview

```
plugins/
└── ai_chat_plugin/
    ├── __init__.py          # describe_plugin() + launch_ui()
    ├── __main__.py          # Standalone entry point
    ├── chat_window.py       # PySide6 main chat window
    ├── chat_service.py      # Copilot SDK wrapper (qasync bridge)
    ├── model_selector.py    # Model selection widget with billing info
    ├── message_widgets.py   # Chat bubbles, markdown rendering, tool cards
    ├── ask_user_dialog.py   # Floating ask_user input widget
    ├── scene_tools.py       # OpenGeoLab scene tools (conditional on pywrapper)
    ├── auth_config.py       # Authentication settings dialog + persistence
    ├── skill.md             # Skill document for AI context
    └── plugin_theme.py      # Theme adaptation (extend existing plugin_theme)
```

### Launch Modes

**Hosted mode** — launched from QML via `plugins.invoke_ui`:

- QApplication already running (owned by C++ host)
- `opengeolab_pywrapper` available for scene interaction
- Progress/status can bridge back to QML via existing mechanisms

**Standalone mode** — launched as `python -m ai_chat_plugin`:

- Plugin creates its own QApplication
- `opengeolab_pywrapper` not available; scene tools disabled gracefully
- Pure conversational AI without scene interaction
- Useful for development, testing, and independent deployment

### Data Flow

1. User clicks Ribbon "Chat" → `RequestService.executeOnMainThread()` →
   `plugins.invoke_ui(ai_chat_plugin)` → `launch_ui()` creates PySide6 window
2. Window initializes **qasync** event loop bridge, creates `CopilotClient`
3. User types message → `session.send()` via qasync → Copilot CLI processes
4. Streaming events (`assistant.message_delta`) → Qt signals → UI updates
5. AI calls `execute_action` tool → `opengeolab_pywrapper.process()` → C++
   CommandDispatcher executes operation → result returned to AI
6. AI calls `ask_user` tool → floating input widget shown → user responds →
   AI continues within same turn

---

## Custom Tools

Four tools registered with the Copilot SDK session:

### 1. `list_modules`

Lists all available OpenGeoLab command modules. Returns JSON array of module
names (e.g. `["geometry", "mesh", "scene", "io"]`). Only available in hosted
mode.

### 2. `describe_module`

Parameters: `module_name: str`

Calls `opengeolab_pywrapper.describe(module_name)` to retrieve all actions and
their parameter/return schemas. Returns the full describe JSON. Only available
in hosted mode.

### 3. `execute_action`

Parameters: `module: str`, `action: str`, `params: dict`

Constructs a standard protocol request `{module, action, param}` and calls
`opengeolab_pywrapper.process()`. Returns the JSON response. Only available in
hosted mode.

### 4. `ask_user`

Parameters: `question: str`, `choices: list[str] | None`

Shows a floating input widget in the chat area. The widget uses dashed borders
and a distinct background to visually indicate "AI is asking you a question".
Supports both choice buttons and freeform text input. Available in both modes.

The tool implementation uses an `asyncio.Future`: when `ask_user` is called, a
Future is created and the UI widget is shown. When the user submits a response,
the Future is resolved from the main thread via
`loop.call_soon_threadsafe(future.set_result, answer)`, allowing the tool
handler to `await` the result without blocking.

### Progressive Capability Discovery

The AI does NOT receive a dump of all commands upfront. Instead, a **skill
document** (`skill.md`) teaches the AI:

1. The request/response protocol (`{module, action, param}` →
   `{ok, result, errors}`)
2. The discovery workflow: `list_modules()` → `describe_module(name)` →
   `execute_action(module, action, params)`
3. Constraints: never guess action names; always discover via describe; confirm
   destructive operations with `ask_user`

---

## PySide6 Chat Window UI

### Window Layout (top to bottom)

**Top toolbar:**
- Model selector dropdown (name + billing multiplier badge)
- Auth config button
- New session button

**Message area** (scrollable):
- **User messages**: Consistent alignment, dark background
- **AI messages**: Markdown rendered (via `markdown` library → HTML →
  `QTextBrowser`), code blocks with syntax highlighting
- **Tool call cards**: Collapsible, show tool name + status
  (running/success/error)
- **ask_user widget**: Dashed border, distinct styling, choice buttons +
  freeform text input, visually indicates "continuation of current turn"

**Input area:**
- Multi-line text input (`QTextEdit`)
- Send button
- Enter to send, Shift+Enter for newline

**Streaming display:**
- Text appended incrementally via `assistant.message_delta` events
- Cursor/typing indicator animation while AI is responding

### Model Selector

Dropdown displays all models from `client.list_models()`:

- `⚡ Nx` badge for premium models (`billing.multiplier > 1`)
- `1x` for standard billing
- `🆓` for free-tier/low-cost models
- Disabled models (`policy.state == "disabled"`) shown grayed out
- Grouped by tier: Premium → Standard → Free

### Authentication Settings Dialog

Accessed via toolbar "Auth" button:

**GitHub Copilot mode:**
- Uses stored CLI credentials (default, no configuration needed)
- Status indicator showing connection health

**BYOK mode:**
- Provider selector: OpenAI, Azure AI Foundry, Anthropic, Ollama, Custom
- Base URL input
- API Key input (masked)
- Model name override
- "Test Connection" button

Configuration persisted to `~/.opengeolab/ai_chat_config.json`. API keys stored
with appropriate security consideration (OS keychain or encrypted file).

---

## qasync Integration

### Event Loop Bridge

```python
import qasync
import asyncio

class AIChatWindow(QMainWindow):
    def __init__(self, embedded: bool = True):
        super().__init__()
        self._loop = qasync.QEventLoop(QApplication.instance())
        asyncio.set_event_loop(self._loop)
        self._service = CopilotChatService(embedded=embedded)
```

The qasync library bridges Qt's event loop with Python's asyncio, allowing
`await` on Copilot SDK calls without blocking the UI. All async operations
(SDK calls, streaming events) are processed as part of Qt's event processing.

**GIL interaction note:** The host application calls
`pybind11::gil_scoped_release` before `QApplication::exec()`. When
`executeOnMainThread()` invokes `launch_ui()`, the GIL is re-acquired
internally. The qasync event loop runs within the Qt event loop on the main
thread, and the Copilot SDK's JSON-RPC communication happens over stdio to a
subprocess — no direct GIL contention with C++ rendering threads.

### CopilotChatService Signals

```python
class CopilotChatService(QObject):
    message_delta = Signal(str)              # Streaming text delta
    message_complete = Signal(str)           # Full message content
    tool_started = Signal(str, str)          # tool_name, description
    tool_completed = Signal(str, str, bool)  # tool_name, result, success
    ask_user_requested = Signal(str, list)   # question, choices
    session_error = Signal(str)              # Error message
    models_loaded = Signal(list)             # Model info list
    connecting = Signal()
    connected = Signal()
```

---

## Authentication

### Supported Methods (Initial Version)

1. **GitHub Signed-in User** — default; uses stored Copilot CLI credentials.
   Requires GitHub Copilot subscription.
2. **BYOK (Bring Your Own Key)** — user provides their own API key. Supports
   OpenAI, Azure AI Foundry, Anthropic, Ollama, and OpenAI-compatible
   endpoints. No Copilot subscription required.

### Configuration Persistence

Auth settings saved to `~/.opengeolab/ai_chat_config.json`:

```json
{
  "auth_method": "github" | "byok",
  "byok": {
    "provider": "openai" | "azure" | "anthropic" | "ollama" | "custom",
    "base_url": "https://...",
    "model": "gpt-4o",
    "wire_api": "completions" | "responses"
  },
  "last_model": "gpt-5.2-codex"
}
```

API keys stored separately via OS keychain or equivalent secure storage, never
in plaintext config files.

---

## Session Management

### Initial Version: Single Session

- One session per chat window instance
- No session persistence across window close/reopen
- "New Session" button clears history and creates fresh session
- Session ID auto-generated (not user-configurable)

### Future Extension Points

- Multi-session with sidebar session list
- Session persistence and resume
- Session export/import

---

## Error Handling

| Error | User Experience |
|-------|----------------|
| Copilot CLI not installed | Dialog with installation instructions and link |
| Authentication failure | Prompt to re-login or configure BYOK |
| Network timeout | Retry button in message area + error text |
| Tool execution failure | Tool card shows red status; AI receives error and explains |
| Session disconnected | Banner with "Reconnect" button |
| Model unavailable | Toast notification; prompt to switch model |
| Rate limit exceeded | Display remaining quota info; suggest waiting or switching model |

---

## Dependencies

New Python packages required:

| Package | Purpose |
|---------|---------|
| `github-copilot-sdk` | Copilot SDK Python client |
| `qasync` | Qt + asyncio event loop bridge |
| `markdown` | Markdown → HTML rendering |

These should be installed into the project's PySide6 venv (`pyvenv/`).

---

## Plugin Interface

### `__init__.py`

```python
def describe_plugin() -> dict:
    return {
        "name": "AI Chat",
        "description": "Chat with AI powered by GitHub Copilot SDK",
        "hasUI": True,
    }

def launch_ui() -> dict:
    from .chat_window import AIChatWindow
    window = AIChatWindow(embedded=True)
    window.show()
    return {"ok": True, "message": "AI Chat window opened"}
```

### `__main__.py`

```python
import sys
from PySide6.QtWidgets import QApplication
from .chat_window import AIChatWindow

def main():
    app = QApplication(sys.argv)
    window = AIChatWindow(embedded=False)
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
```

### Scene Tools Capability Detection

```python
# scene_tools.py
try:
    import opengeolab_pywrapper as wrapper
    HOSTED_MODE = True
except ImportError:
    HOSTED_MODE = False

def get_available_tools() -> list:
    tools = [ask_user_tool]  # Always available
    if HOSTED_MODE:
        tools += [list_modules_tool, describe_module_tool, execute_action_tool]
    return tools
```

---

## Scope Boundaries

### In Scope (This Spec)

- AI Chat plugin with PySide6 window
- Copilot SDK integration via qasync
- GitHub auth + BYOK authentication
- Model selection with billing info
- Streaming responses with Markdown rendering
- Custom tools: list_modules, describe_module, execute_action, ask_user
- Skill document for progressive capability discovery
- Hosted + standalone launch modes
- Theme matching with host application

### Out of Scope

- AI Suggest functionality (separate future spec)
- Multi-session management and persistence
- Image/vision input
- Voice input/output
- Plugin marketplace distribution
- Automated testing of AI responses
