# AI Chat Plugin — Segment 2b: Auth, Config & Model Selection

## Problem Statement

Segment 2a delivered a working chat interface with streaming responses, tool
execution, and ask\_user interaction — all hardcoded to GitHub Copilot
`use_logged_in_user` auth with the SDK default model.  Segment 2b adds user
control over authentication method, model selection, and persistent
configuration so the plugin works with both GitHub Copilot and third-party
providers.

## Spec Reference

Parent spec: `docs/superpowers/specs/2026-04-03-ai-chat-plugin-design.md`.
Previous segment: `docs/superpowers/specs/2026-04-04-ai-chat-segment2a-chat-core-design.md`.

This document covers **Segment 2b** only (Auth + Config + Model Selector).

---

## Design Decisions (from brainstorm)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Auth methods | GitHub Copilot (default) + BYOK | Per parent spec |
| BYOK providers | OpenAI, Azure, Anthropic, Ollama, Custom | SDK supports `provider` param in `create_session()` |
| Config persistence | JSON file (`~/.opengeolab/ai_chat_config.json`) | Per parent spec; plaintext API keys initially, keyring deferred |
| Model selector location | ChatPage, above input area | Keeps toolbar uncluttered; model is chat-specific |
| Auth settings UI | Overlay Popup panel | No separate Dialog window; stays in context |
| Model list source | SDK `list_models()` if available; fallback to text input | GitHub mode may have model listing; BYOK always manual |
| Session on model change | New session created | SDK does not support in-session model switching |

---

## Architecture Overview

```
┌────────────── Existing (Segment 2a) ──────────────┐
│  PluginWindow.qml → ChatPage.qml                   │
│  ChatBackend → CopilotWorker → Copilot SDK          │
└─────────────────────────────────────────────────────┘
        │ new dependency │
┌───────▼─────────────────────────────────────────────┐
│  ChatConfig (QObject)                                │
│    ├─ auth_method: "github" | "byok"                 │
│    ├─ byok_provider: user-facing name (see mapping)   │
│    ├─ byok_base_url: str                             │
│    ├─ byok_api_key: str                              │
│    ├─ byok_model: str                                │
│    ├─ byok_wire_api: "completions"|"responses"       │
│    ├─ last_model: str                                │
│    ├─ load() / save()                                │
│    └─ JSON file: ~/.opengeolab/ai_chat_config.json   │
├──────────────────────────────────────────────────────┤
│  ModelSelectorBar.qml                                │
│    ├─ Model dropdown (ComboBox or editable combo)    │
│    └─ Auth status indicator (colored dot + label)    │
├──────────────────────────────────────────────────────┤
│  AuthSettingsPanel.qml                               │
│    ├─ GitHub / BYOK radio toggle                     │
│    ├─ BYOK fields (provider, base_url, api_key, ...) │
│    ├─ Test Connection button                         │
│    └─ Save & Reconnect / Cancel                      │
└──────────────────────────────────────────────────────┘
```

### Data Flow

1. On startup, `ChatBackend` creates `ChatConfig` which loads
   `~/.opengeolab/ai_chat_config.json` (or uses defaults if absent).
2. `ChatBackend` passes `ChatConfig` reference to `CopilotWorker`.
3. `CopilotWorker._create_session()` reads config to build `create_session()`
   kwargs:
   - GitHub mode: `provider=None`, `model=config.last_model or None`
   - BYOK mode: `provider={type, base_url, api_key, wire_api}`,
     `model=config.byok_model`
4. User changes model in `ModelSelectorBar` → `ChatBackend` updates config →
   triggers `requestNewSession` with new model.
5. User changes auth in `AuthSettingsPanel` → saves config → triggers
   `requestNewSession`.

---

## Component Specifications

### 1. ChatConfig (`chat_config.py`)

A `QObject` subclass managing persistent configuration.

**JSON Schema:**

```json
{
  "auth_method": "github",
  "byok": {
    "provider": "openai",
    "base_url": "",
    "api_key": "",
    "model": "",
    "wire_api": "completions"
  },
  "last_model": ""
}
```

**Properties (all `Q_PROPERTY` with notify signals):**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `authMethod` | `str` | `"github"` | `"github"` or `"byok"` |
| `byokProvider` | `str` | `"openai"` | User-facing name: `"openai"`, `"azure"`, `"anthropic"`, `"ollama"`, `"custom"` |
| `byokBaseUrl` | `str` | `""` | API endpoint URL |
| `byokApiKey` | `str` | `""` | API key (plaintext in v1) |
| `byokModel` | `str` | `""` | Model name for BYOK provider |
| `byokWireApi` | `str` | `"completions"` | `"completions"` or `"responses"` |
| `lastModel` | `str` | `""` | Last used model (GitHub mode) |

**File path:** `~/.opengeolab/ai_chat_config.json`

**Behavior:**
- `load()` called in constructor; missing file → use defaults silently.
- `save()` called on every property setter; creates parent directory if needed.
- Unknown keys in JSON are preserved (forward compatibility).
- Thread-safe reads: worker thread may read properties; Python GIL provides
  sufficient synchronization for simple property reads.

**Test requirements (TDD):**
- Load from valid JSON file
- Load with missing file (defaults)
- Load with partial JSON (missing keys use defaults)
- Save creates file and parent directory
- Round-trip: save then load preserves all fields
- Unknown keys preserved after save
- Property setters trigger save

### 2. CopilotWorker Changes

**Modified method: `_create_session()`**

Currently hardcodes `use_logged_in_user=True` and no `model`/`provider`.
After Segment 2b:

```python
async def _create_session(self) -> None:
    config = self._config  # ChatConfig reference

    # Build provider kwargs based on auth method
    session_kwargs = {
        "on_permission_request": PermissionHandler.approve_all,
        "tools": tools,
        "streaming": True,
        "system_message": SystemMessageConfig(text=system_prompt),
        "skill_directories": [str(_PROMPTS_DIR)],
        "on_event": self._on_event,
        "on_user_input_request": self._on_user_input,
    }

    if config.authMethod == "byok":
        # Map user-facing provider name to SDK provider type.
        # Ollama and Custom both use "openai" type (OpenAI-compatible API).
        _SDK_TYPE_MAP = {
            "openai": "openai",
            "azure": "azure",
            "anthropic": "anthropic",
            "ollama": "openai",
            "custom": "openai",
        }
        provider_config = {
            "type": _SDK_TYPE_MAP.get(config.byokProvider, "openai"),
            "base_url": config.byokBaseUrl,
        }
        if config.byokApiKey:
            provider_config["api_key"] = config.byokApiKey
        if config.byokProvider in ("openai", "azure"):
            provider_config["wire_api"] = config.byokWireApi
        session_kwargs["provider"] = provider_config
        session_kwargs["model"] = config.byokModel
    else:
        # GitHub mode: model is optional
        if config.lastModel:
            session_kwargs["model"] = config.lastModel

    session = await self._client.create_session(**session_kwargs)
```

**New constructor parameter:**
- `CopilotWorker.__init__(self, config: ChatConfig, parent=None)`

**`requestNewSession` behavior unchanged** — it sets `_shutdown=True` and
unblocks the queue; the next iteration re-calls `_create_session()` which
reads the updated config.

### 3. ChatBackend Changes

**New ownership:**
- `ChatBackend` creates and owns `ChatConfig`.
- `ChatBackend` passes `ChatConfig` to `CopilotWorker`.
- `ChatBackend` exposes `chatConfig` as a `Q_PROPERTY` for QML binding.

**New signals/methods:**

| Method/Property | Purpose |
|----------------|---------|
| `chatConfig` (Q_PROPERTY) | Expose ChatConfig to QML |
| `switchModel(model: str)` (Q_INVOKABLE) | Update config + requestNewSession |
| `testConnection()` (Q_INVOKABLE) | Test BYOK connection (async) |
| `connectionStatus` (Q_PROPERTY) | `"connected"`, `"connecting"`, `"error"` |

**Model listing:**
- `ChatBackend` exposes a `modelList` property (list of dicts with `name`,
  `billing_multiplier` fields).
- On `connectionReady`, if GitHub mode, attempt to query available models
  from SDK.  If SDK doesn't support `list_models()`, expose an empty list
  (QML falls back to editable ComboBox).
- In BYOK mode, `modelList` is always empty (user types model name).

### 4. ModelSelectorBar.qml

New QML component placed in ChatPage between streaming indicator and input
area.

**Layout:**

```
┌───────────────────────────────────────────────────┐
│ [▼ gpt-4o ⚡2x ]                    🟢 Connected │
└───────────────────────────────────────────────────┘
```

**Left side — Model selector:**
- `ComboBox` with `editable: true` (user can type custom model name).
- Populated from `chatBackend.modelList` when available.
- Billing badge suffix: `⚡Nx` for `multiplier > 1`, `🆓` for `multiplier < 1`,
  nothing for `multiplier == 1`.
- On selection change → `chatBackend.switchModel(selectedModel)`.
- Disabled during streaming (`chatBackend.isStreaming`).

**Right side — Auth status indicator:**
- Colored dot + text label:
  - 🟢 `Connected` — `connectionStatus == "connected"`
  - 🟡 `Connecting...` — `connectionStatus == "connecting"`
  - 🔴 `Error` — `connectionStatus == "error"`
- Clickable → opens `AuthSettingsPanel` as `Popup`.
- Shows current auth method as subtle label: `"GitHub"` or `"BYOK: OpenAI"`.

**Theming:** Uses `PluginTheme` singleton, consistent with existing components.

### 5. AuthSettingsPanel.qml

Overlay `Popup` that appears when clicking the auth status indicator.

**Layout:**

```
┌─ Authentication Settings ──────────────────────┐
│                                                 │
│  Auth Method:                                   │
│  ( ● ) GitHub Copilot  ( ○ ) BYOK              │
│                                                 │
│  ─── BYOK Settings (disabled when GitHub) ──── │
│  Provider:  [▼ OpenAI          ]                │
│  Base URL:  [https://api.openai.com/v1    ]     │
│  API Key:   [••••••••••••••••             ]     │
│  Model:     [gpt-4o                       ]     │
│  Wire API:  [▼ completions     ]                │
│                                                 │
│  [Test Connection]                              │
│                                                 │
│  Status: ✅ Connected / ❌ Error message        │
│                                                 │
│         [Cancel]  [Save & Reconnect]            │
└─────────────────────────────────────────────────┘
```

**Behavior:**
- `RadioButton` group for GitHub / BYOK toggle.
- BYOK section disabled + dimmed when GitHub is selected.
- Provider `ComboBox` options: `["OpenAI", "Azure", "Anthropic", "Ollama", "Custom"]`.
  - Selecting Ollama pre-fills base\_url with `http://localhost:11434/v1`.
  - Selecting Azure shows `wire_api` ComboBox; otherwise hidden.
- API Key field uses `echoMode: TextInput.Password`.
- "Test Connection" button:
  - Calls `chatBackend.testConnection()`.
  - Implementation: creates a temporary `CopilotClient` + `create_session()`
    with the unsaved panel settings, using a short timeout.  On success emits
    `testConnectionResult(True, "")`, on failure emits
    `testConnectionResult(False, errorMessage)`.  The temporary client is
    disposed immediately after the test.
  - Shows spinner while testing, then success/failure message.
  - Does NOT save config — only validates connectivity.
- "Save & Reconnect" button:
  - Writes all fields to `ChatConfig` (triggers save to JSON).
  - Calls `chatBackend.newSession()` to reconnect with new settings.
  - Closes the popup.
- "Cancel" button:
  - Discards unsaved changes, closes popup.
  - Reverts local QML state to current `ChatConfig` values.

**Strings:** All user-visible text wrapped in `qsTr()`.

---

## ChatPage.qml Changes

Insert `ModelSelectorBar` between streaming indicator and `ChatInputArea`:

```qml
// ── Streaming indicator ─────────────────────────
Rectangle { ... }  // existing

// ── Model selector bar (NEW) ────────────────────
ModelSelectorBar {
    Layout.fillWidth: true
    Layout.margins: PluginTheme.gapTight
    chatBackend: root.chatBackend
}

// ── Input area ──────────────────────────────────
ChatInputArea { ... }  // existing
```

---

## Entry Point Changes

Both `__init__.py` and `__main__.py` need to:
1. Create `ChatConfig` before `ChatBackend`.
2. Pass `ChatConfig` to `ChatBackend` constructor.

```python
# __init__.py launch_ui() and __main__.py _create_engine():
config = ChatConfig()
chat_backend = ChatBackend(config=config)
```

---

## Configuration File Lifecycle

| Event | Action |
|-------|--------|
| First launch | No file exists; defaults used (GitHub auth, no model) |
| User opens Auth panel + saves | File created at `~/.opengeolab/ai_chat_config.json` |
| User changes model | `last_model` updated, file saved |
| File manually deleted | Next launch uses defaults again |
| File has unknown keys | Preserved on save (forward compat) |

---

## Error Handling

| Error | User Experience |
|-------|----------------|
| BYOK with empty model | "Save & Reconnect" disabled; tooltip explains model is required |
| BYOK connection fails | Auth status → 🔴; error banner in ChatPage; Auth panel shows error detail |
| Test Connection fails | Inline error message in Auth panel; config NOT saved |
| Config file unwritable | Warning logged; settings work in-memory but lost on restart |
| Invalid JSON in config file | Logged warning; defaults used; file overwritten on next save |

---

## Security Considerations

- **v1:** API keys stored in plaintext JSON.  File permissions set to
  user-read-only (`0o600` on Unix; ACL on Windows is best-effort).
- **Future:** Migrate to `keyring` library for OS keychain integration.
- API key field in QML uses password echo mode to prevent shoulder surfing.
- Config file path is in user home directory, not in project/repo directory.

---

## Scope Boundaries

### In Scope (This Segment)

- ChatConfig with JSON persistence
- CopilotWorker + ChatBackend integration with config
- ModelSelectorBar QML component
- AuthSettingsPanel QML component (Popup)
- ChatPage layout update
- Entry point changes
- GitHub Copilot + BYOK (OpenAI, Azure, Anthropic, Ollama, Custom)
- Test Connection functionality
- Model selection with new session creation

### Out of Scope

- OS keychain / keyring for API key storage (future iteration)
- Model billing info badges (depends on SDK `list_models()` availability)
- Multi-session management
- Session persistence across restarts
- Rate limit display
