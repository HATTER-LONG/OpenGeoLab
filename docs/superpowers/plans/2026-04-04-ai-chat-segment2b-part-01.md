# AI Chat Plugin — Segment 2b: Part 1 of 2

> Part 文件：Python Backend — ChatConfig, CopilotWorker, ChatBackend, Entry Points.
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Add persistent auth/model configuration, wire it into CopilotWorker and ChatBackend, and expose model-switching and connection-testing to QML.

**Architecture:** New `ChatConfig` QObject owns a JSON file at `~/.opengeolab/ai_chat_config.json`. `CopilotWorker` reads config to build `create_session()` kwargs (provider, model). `ChatBackend` owns `ChatConfig`, exposes it as a Q\_PROPERTY, and adds `switchModel`, `testConnection`, and `connectionStatus`. Entry points updated to pass config through.

**Tech Stack:** Python 3.13, PySide6 6.9, `github-copilot-sdk`, `urllib` (stdlib)

**Spec:** `docs/superpowers/specs/2026-04-04-ai-chat-segment2b-auth-config-design.md`

---

### Task 1: ChatConfig with TDD

**Files:**
- Create: `plugins/ai_chat_plugin/chat_config.py`
- Create: `plugins/ai_chat_plugin/tests/test_chat_config.py`

- [ ] **Step 1: Write 8 failing tests**

Create `plugins/ai_chat_plugin/tests/test_chat_config.py`:

```python
"""Tests for ChatConfig persistent configuration."""
from __future__ import annotations

import json
from pathlib import Path

import pytest

from ai_chat_plugin.chat_config import ChatConfig


@pytest.fixture
def config_path(tmp_path: Path) -> Path:
    """Return a config file path inside a fresh temp directory."""
    return tmp_path / "config" / "ai_chat_config.json"


class TestChatConfigLoad:
    """Loading behaviour — defaults, valid JSON, partial JSON, corrupt JSON."""

    def test_missing_file_uses_defaults(self, config_path: Path) -> None:
        cfg = ChatConfig(config_path=config_path)
        assert cfg.authMethod == "github"
        assert cfg.byokProvider == "openai"
        assert cfg.byokBaseUrl == ""
        assert cfg.byokApiKey == ""
        assert cfg.byokModel == ""
        assert cfg.byokWireApi == "completions"
        assert cfg.lastModel == ""

    def test_load_valid_json(self, config_path: Path) -> None:
        config_path.parent.mkdir(parents=True, exist_ok=True)
        config_path.write_text(json.dumps({
            "auth_method": "byok",
            "byok": {
                "provider": "ollama",
                "base_url": "http://localhost:11434/v1",
                "api_key": "",
                "model": "llama3",
                "wire_api": "completions",
            },
            "last_model": "gpt-4o",
        }), encoding="utf-8")

        cfg = ChatConfig(config_path=config_path)
        assert cfg.authMethod == "byok"
        assert cfg.byokProvider == "ollama"
        assert cfg.byokBaseUrl == "http://localhost:11434/v1"
        assert cfg.byokModel == "llama3"
        assert cfg.lastModel == "gpt-4o"

    def test_load_partial_json_fills_defaults(self, config_path: Path) -> None:
        config_path.parent.mkdir(parents=True, exist_ok=True)
        config_path.write_text(
            json.dumps({"auth_method": "byok"}), encoding="utf-8",
        )

        cfg = ChatConfig(config_path=config_path)
        assert cfg.authMethod == "byok"
        assert cfg.byokProvider == "openai"
        assert cfg.byokModel == ""

    def test_load_invalid_json_uses_defaults(self, config_path: Path) -> None:
        config_path.parent.mkdir(parents=True, exist_ok=True)
        config_path.write_text("{bad json!!!", encoding="utf-8")

        cfg = ChatConfig(config_path=config_path)
        assert cfg.authMethod == "github"


class TestChatConfigSave:
    """Persistence — directory creation, round-trip, unknown key preservation."""

    def test_save_creates_parent_directory(self, config_path: Path) -> None:
        cfg = ChatConfig(config_path=config_path)
        cfg.save()
        assert config_path.exists()

    def test_round_trip_preserves_all_fields(self, config_path: Path) -> None:
        cfg1 = ChatConfig(config_path=config_path)
        cfg1.authMethod = "byok"
        cfg1.byokProvider = "anthropic"
        cfg1.byokBaseUrl = "https://api.anthropic.com"
        cfg1.byokApiKey = "sk-test"
        cfg1.byokModel = "claude-3"
        cfg1.byokWireApi = "responses"
        cfg1.lastModel = "gpt-4o"

        cfg2 = ChatConfig(config_path=config_path)
        assert cfg2.authMethod == "byok"
        assert cfg2.byokProvider == "anthropic"
        assert cfg2.byokBaseUrl == "https://api.anthropic.com"
        assert cfg2.byokApiKey == "sk-test"
        assert cfg2.byokModel == "claude-3"
        assert cfg2.byokWireApi == "responses"
        assert cfg2.lastModel == "gpt-4o"

    def test_unknown_keys_preserved(self, config_path: Path) -> None:
        config_path.parent.mkdir(parents=True, exist_ok=True)
        config_path.write_text(json.dumps({
            "auth_method": "github",
            "future_feature": True,
            "byok": {"provider": "openai", "experimental_flag": 42},
        }), encoding="utf-8")

        cfg = ChatConfig(config_path=config_path)
        cfg.lastModel = "gpt-4o"  # triggers save

        raw = json.loads(config_path.read_text(encoding="utf-8"))
        assert raw["future_feature"] is True
        assert raw["byok"]["experimental_flag"] == 42

    def test_property_setter_auto_saves(self, config_path: Path) -> None:
        cfg = ChatConfig(config_path=config_path)
        assert not config_path.exists()

        cfg.authMethod = "byok"
        assert config_path.exists()
        raw = json.loads(config_path.read_text(encoding="utf-8"))
        assert raw["auth_method"] == "byok"
```

- [ ] **Step 2: Run tests to verify they fail**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_chat_config.py -v
```

Expected: `ModuleNotFoundError: No module named 'ai_chat_plugin.chat_config'` (8 ERRORS).

- [ ] **Step 3: Write ChatConfig implementation**

Create `plugins/ai_chat_plugin/chat_config.py`:

```python
"""Persistent configuration for the AI Chat Plugin.

Manages authentication method, BYOK provider settings, and model selection.
Configuration is stored as JSON at ``~/.opengeolab/ai_chat_config.json``.
"""
from __future__ import annotations

import json
from pathlib import Path

from PySide6.QtCore import Property, QObject, Signal


_DEFAULT_CONFIG_PATH = Path.home() / ".opengeolab" / "ai_chat_config.json"

_DEFAULTS: dict = {
    "auth_method": "github",
    "byok": {
        "provider": "openai",
        "base_url": "",
        "api_key": "",
        "model": "",
        "wire_api": "completions",
    },
    "last_model": "",
}


def _deep_merge(defaults: dict, overrides: dict) -> dict:
    """Merge *overrides* into *defaults* recursively, preserving unknown keys."""
    result = dict(defaults)
    for key, value in overrides.items():
        if key in result and isinstance(result[key], dict) and isinstance(value, dict):
            result[key] = _deep_merge(result[key], value)
        else:
            result[key] = value
    return result


class ChatConfig(QObject):
    """Persistent configuration backed by a JSON file.

    Every property setter automatically persists the change to disk.
    Unknown keys in the JSON file are preserved across saves.
    """

    authMethodChanged = Signal()
    byokProviderChanged = Signal()
    byokBaseUrlChanged = Signal()
    byokApiKeyChanged = Signal()
    byokModelChanged = Signal()
    byokWireApiChanged = Signal()
    lastModelChanged = Signal()

    def __init__(
        self,
        config_path: Path | None = None,
        parent: QObject | None = None,
    ) -> None:
        super().__init__(parent)
        self._path = config_path or _DEFAULT_CONFIG_PATH
        self._data: dict = {}
        self.load()

    # -- Persistence -----------------------------------------------------------

    def load(self) -> None:
        """Load configuration from disk; use defaults for missing keys."""
        raw: dict = {}
        if self._path.is_file():
            try:
                raw = json.loads(self._path.read_text(encoding="utf-8"))
            except (json.JSONDecodeError, OSError):
                raw = {}
        self._data = _deep_merge(_DEFAULTS, raw)

    def save(self) -> None:
        """Write configuration to disk, creating parent directory if needed."""
        self._path.parent.mkdir(parents=True, exist_ok=True)
        self._path.write_text(
            json.dumps(self._data, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

    # -- Property helpers ------------------------------------------------------

    def _byok(self) -> dict:
        """Return the ``byok`` sub-dict, creating it if absent."""
        return self._data.setdefault("byok", {})

    # -- authMethod ------------------------------------------------------------

    def _get_auth_method(self) -> str:
        return self._data.get("auth_method", "github")

    def _set_auth_method(self, value: str) -> None:
        if self._data.get("auth_method") != value:
            self._data["auth_method"] = value
            self.authMethodChanged.emit()
            self.save()

    authMethod = Property(
        str, _get_auth_method, _set_auth_method, notify=authMethodChanged,
    )

    # -- byokProvider ----------------------------------------------------------

    def _get_byok_provider(self) -> str:
        return self._byok().get("provider", "openai")

    def _set_byok_provider(self, value: str) -> None:
        if self._byok().get("provider") != value:
            self._byok()["provider"] = value
            self.byokProviderChanged.emit()
            self.save()

    byokProvider = Property(
        str, _get_byok_provider, _set_byok_provider, notify=byokProviderChanged,
    )

    # -- byokBaseUrl -----------------------------------------------------------

    def _get_byok_base_url(self) -> str:
        return self._byok().get("base_url", "")

    def _set_byok_base_url(self, value: str) -> None:
        if self._byok().get("base_url") != value:
            self._byok()["base_url"] = value
            self.byokBaseUrlChanged.emit()
            self.save()

    byokBaseUrl = Property(
        str, _get_byok_base_url, _set_byok_base_url, notify=byokBaseUrlChanged,
    )

    # -- byokApiKey ------------------------------------------------------------

    def _get_byok_api_key(self) -> str:
        return self._byok().get("api_key", "")

    def _set_byok_api_key(self, value: str) -> None:
        if self._byok().get("api_key") != value:
            self._byok()["api_key"] = value
            self.byokApiKeyChanged.emit()
            self.save()

    byokApiKey = Property(
        str, _get_byok_api_key, _set_byok_api_key, notify=byokApiKeyChanged,
    )

    # -- byokModel -------------------------------------------------------------

    def _get_byok_model(self) -> str:
        return self._byok().get("model", "")

    def _set_byok_model(self, value: str) -> None:
        if self._byok().get("model") != value:
            self._byok()["model"] = value
            self.byokModelChanged.emit()
            self.save()

    byokModel = Property(
        str, _get_byok_model, _set_byok_model, notify=byokModelChanged,
    )

    # -- byokWireApi -----------------------------------------------------------

    def _get_byok_wire_api(self) -> str:
        return self._byok().get("wire_api", "completions")

    def _set_byok_wire_api(self, value: str) -> None:
        if self._byok().get("wire_api") != value:
            self._byok()["wire_api"] = value
            self.byokWireApiChanged.emit()
            self.save()

    byokWireApi = Property(
        str, _get_byok_wire_api, _set_byok_wire_api, notify=byokWireApiChanged,
    )

    # -- lastModel -------------------------------------------------------------

    def _get_last_model(self) -> str:
        return self._data.get("last_model", "")

    def _set_last_model(self, value: str) -> None:
        if self._data.get("last_model") != value:
            self._data["last_model"] = value
            self.lastModelChanged.emit()
            self.save()

    lastModel = Property(
        str, _get_last_model, _set_last_model, notify=lastModelChanged,
    )
```

- [ ] **Step 4: Run tests to verify they pass**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/test_chat_config.py -v
```

Expected: 8 passed.

- [ ] **Step 5: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/chat_config.py plugins/ai_chat_plugin/tests/test_chat_config.py
git commit -m "feat(ai-chat): add ChatConfig with JSON persistence (8 tests)

TDD: persistent configuration for auth method, BYOK provider settings,
and model selection. Stores JSON at ~/.opengeolab/ai_chat_config.json.
Unknown keys preserved for forward compatibility.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: CopilotWorker Config Integration

**Files:**
- Modify: `plugins/ai_chat_plugin/copilot_worker.py`

- [ ] **Step 1: Add config parameter and SDK type map**

In `copilot_worker.py`, add the module-level type map after `_ASK_USER_TIMEOUT`:

```python
# Map user-facing BYOK provider name → SDK provider type.
# Ollama and Custom both speak the OpenAI-compatible API.
_SDK_TYPE_MAP: dict[str, str] = {
    "openai": "openai",
    "azure": "azure",
    "anthropic": "anthropic",
    "ollama": "openai",
    "custom": "openai",
}
```

- [ ] **Step 2: Update constructor to accept config**

Replace the existing `__init__` method:

```python
def __init__(self, config, parent=None) -> None:
    super().__init__(parent)
    self._config = config
    self._loop: asyncio.AbstractEventLoop | None = None
    self._queue: asyncio.Queue[str | None] | None = None
    self._shutdown = False

    # ask_user bridging state
    self._ask_user_event = threading.Event()
    self._ask_user_answer: str | None = None
```

- [ ] **Step 3: Update `_main()` to build session from config**

Replace the `CopilotClient` / `create_session` block inside the `while not self._shutdown` loop (lines 117–135 of the original) with:

```python
                # Build SubprocessConfig — BYOK may not need GitHub login
                subprocess_kwargs: dict = {"log_level": "warning"}
                if self._config.authMethod != "byok":
                    subprocess_kwargs["use_logged_in_user"] = True

                async with CopilotClient(
                    SubprocessConfig(**subprocess_kwargs),
                    auto_start=True,
                ) as client:
                    tools = build_tools()
                    session_kwargs: dict = {
                        "on_permission_request": PermissionHandler.approve_all,
                        "tools": tools,
                        "streaming": True,
                        "system_message": SystemMessageConfig(
                            text=_load_prompt("system_prompt.md"),
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
```

Note: the outer `try/except`, the `if self._shutdown: self._shutdown = False` reset, and all other logic outside this block remain **unchanged**.

- [ ] **Step 4: Run full pytest suite**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
```

Expected: all existing tests still pass (ChatConfig 8 + model 12 + markdown 10 + tools 9 = 39 passed). CopilotWorker has no unit tests (requires SDK), but import should succeed.

- [ ] **Step 5: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/copilot_worker.py
git commit -m "feat(ai-chat): wire ChatConfig into CopilotWorker session creation

CopilotWorker now accepts a ChatConfig reference and builds
create_session() kwargs from it: GitHub mode uses use_logged_in_user,
BYOK mode passes provider config with SDK type mapping.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: ChatBackend — Config, switchModel, connectionStatus

**Files:**
- Modify: `plugins/ai_chat_plugin/chat_backend.py`

This task adds the `chatConfig` property, `connectionStatus` derived property, and `switchModel` invokable.

- [ ] **Step 1: Add imports and new signals**

At the top of `chat_backend.py`, add the import:

```python
from ai_chat_plugin.chat_config import ChatConfig
```

Inside the `ChatBackend` class, add a new signal after the existing ones:

```python
    connectionStatusChanged = Signal()
```

- [ ] **Step 2: Update constructor to accept config**

Replace:

```python
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._message_model = ChatMessageModel(self)
        self._worker = CopilotWorker(self)
```

With:

```python
    def __init__(self, config=None, parent=None) -> None:
        super().__init__(parent)
        self._config = config if config is not None else ChatConfig(parent=self)
        self._message_model = ChatMessageModel(self)
        self._worker = CopilotWorker(config=self._config, parent=self)
```

- [ ] **Step 3: Add chatConfig and connectionStatus properties**

After the existing `darkMode` property block, add:

```python
    @Property(QObject, constant=True)
    def chatConfig(self):
        """Expose ChatConfig to QML for binding."""
        return self._config

    def _get_connection_status(self) -> str:
        """Computed status: 'connecting', 'error', or 'connected'."""
        if self._is_connecting:
            return "connecting"
        if self._connection_error:
            return "error"
        return "connected"

    connectionStatus = Property(
        str, _get_connection_status, notify=connectionStatusChanged,
    )
```

- [ ] **Step 4: Add switchModel invokable**

After the existing `retryConnection` slot, add:

```python
    @Slot(str)
    def switchModel(self, model: str) -> None:
        """Update selected model in config and create a new SDK session."""
        model = model.strip()
        if self._config.authMethod == "byok":
            self._config.byokModel = model
        else:
            self._config.lastModel = model
        self.newSession()
```

- [ ] **Step 5: Emit connectionStatusChanged in state transitions**

Update `_set_connecting` to also emit `connectionStatusChanged`:

```python
    def _set_connecting(self, value: bool) -> None:
        if self._is_connecting != value:
            self._is_connecting = value
            self.isConnectingChanged.emit()
            self.connectionStatusChanged.emit()
```

Reorder `_on_connected` so error is cleared before `_set_connecting`:

```python
    def _on_connected(self) -> None:
        self._connection_error = ""
        self.connectionErrorChanged.emit()
        self._set_connecting(False)
        self._message_model.appendMessage(
            "system", "Connected to AI assistant. How can I help?"
        )
```

Reorder `_on_connection_failed` so error is set before connecting is cleared (prevents brief "connected" flicker in `connectionStatus`):

```python
    def _on_connection_failed(self, error: str) -> None:
        self._connection_error = error
        self.connectionErrorChanged.emit()
        self._set_connecting(False)
        self._set_streaming(False)
        self.connectionStatusChanged.emit()
```

- [ ] **Step 6: Run full pytest suite**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
```

Expected: 39 passed.

- [ ] **Step 7: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/chat_backend.py
git commit -m "feat(ai-chat): add chatConfig, connectionStatus, switchModel to ChatBackend

ChatBackend now owns ChatConfig and exposes it as a Q_PROPERTY for QML
binding. connectionStatus is a derived property ('connecting'|'error'|
'connected'). switchModel updates config and triggers a new session.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: ChatBackend — testConnection

**Files:**
- Modify: `plugins/ai_chat_plugin/chat_backend.py`

Adds an HTTP-based connection test for BYOK providers. The QML AuthSettingsPanel (Part 2) will call this with unsaved form data as JSON.

> **Design note:** The spec suggests creating a temporary CopilotClient + session. We use a simpler HTTP health check (`GET /models`) instead, which is faster and works without spawning a subprocess. The endpoint is standard for OpenAI-compatible APIs (OpenAI, Azure, Ollama, vLLM, etc.). For Anthropic, the check targets the base URL root.

- [ ] **Step 1: Add testConnection signal and thread tracking**

Add a new signal and `import threading` at the module level:

```python
import threading
```

Inside `ChatBackend`, add a new signal alongside the existing ones:

```python
    testConnectionResult = Signal(bool, str)  # (success, error_message)
```

In `__init__`, add:

```python
        self._test_thread: threading.Thread | None = None
```

- [ ] **Step 2: Add testConnection slot**

After `switchModel`, add:

```python
    @Slot(str)
    def testConnection(self, settings_json: str) -> None:
        """Test BYOK connectivity via HTTP health check (background thread).

        *settings_json* is a JSON object with keys: ``provider``,
        ``base_url``, ``api_key``.  Emits ``testConnectionResult(success,
        error_message)`` when done.  Does NOT modify ChatConfig.
        """
        import json as _json

        if self._test_thread is not None and self._test_thread.is_alive():
            return  # test already in progress

        settings = _json.loads(settings_json)
        self._test_thread = threading.Thread(
            target=self._run_connection_test,
            args=(settings,),
            daemon=True,
        )
        self._test_thread.start()

    def _run_connection_test(self, settings: dict) -> None:
        """HTTP health check against BYOK provider (runs in background)."""
        import urllib.error
        import urllib.request

        base_url = (settings.get("base_url") or "").rstrip("/")
        api_key = settings.get("api_key", "")

        if not base_url:
            self.testConnectionResult.emit(False, "Base URL is required")
            return

        try:
            url = f"{base_url}/models"
            req = urllib.request.Request(url, method="GET")
            req.add_header("Accept", "application/json")
            if api_key:
                req.add_header("Authorization", f"Bearer {api_key}")
            with urllib.request.urlopen(req, timeout=10) as resp:
                self.testConnectionResult.emit(True, "")
        except urllib.error.HTTPError as exc:
            self.testConnectionResult.emit(
                False, f"HTTP {exc.code}: {exc.reason}",
            )
        except urllib.error.URLError as exc:
            self.testConnectionResult.emit(
                False, f"Connection failed: {exc.reason}",
            )
        except Exception as exc:
            self.testConnectionResult.emit(False, str(exc))
```

- [ ] **Step 3: Run full pytest suite**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
```

Expected: 39 passed.

- [ ] **Step 4: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/chat_backend.py
git commit -m "feat(ai-chat): add testConnection HTTP health check to ChatBackend

testConnection(settings_json) runs a background HTTP GET to
base_url/models for BYOK provider validation. Emits
testConnectionResult(bool, str) signal for QML consumption.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: Entry Points Update

**Files:**
- Modify: `plugins/ai_chat_plugin/__init__.py`
- Modify: `plugins/ai_chat_plugin/__main__.py`

- [ ] **Step 1: Update `__init__.py` launch\_ui()**

In `plugins/ai_chat_plugin/__init__.py`, add the ChatConfig import alongside ChatBackend and create config before backend.

Replace (lines 33–40):

```python
    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine
```

With:

```python
    from ai_chat_plugin.chat_config import ChatConfig
    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine
```

Replace (lines 39–40):

```python
    backend = DebuggerBackend()
    chat_backend = ChatBackend()
```

With:

```python
    backend = DebuggerBackend()
    config = ChatConfig()
    chat_backend = ChatBackend(config=config)
```

Add `config` to the engine reference block — replace:

```python
    engine._backend = backend
    engine._chat_backend = chat_backend
```

With:

```python
    engine._backend = backend
    engine._config = config
    engine._chat_backend = chat_backend
```

- [ ] **Step 2: Update `__main__.py` _create\_engine()**

In `plugins/ai_chat_plugin/__main__.py`, add the ChatConfig import and create config.

Replace (lines 40–42):

```python
    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine
```

With:

```python
    from ai_chat_plugin.chat_config import ChatConfig
    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine
```

Replace (lines 46–47):

```python
    backend = DebuggerBackend()
    chat_backend = ChatBackend()
```

With:

```python
    backend = DebuggerBackend()
    config = ChatConfig()
    chat_backend = ChatBackend(config=config)
```

Replace:

```python
    engine._backend = backend
    engine._chat_backend = chat_backend
```

With:

```python
    engine._backend = backend
    engine._config = config
    engine._chat_backend = chat_backend
```

- [ ] **Step 3: Run full pytest suite + import smoke check**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
.\pyvenv\Scripts\python.exe -c "from ai_chat_plugin.chat_config import ChatConfig; print('ChatConfig OK')"
.\pyvenv\Scripts\python.exe -c "from ai_chat_plugin.chat_backend import ChatBackend; print('ChatBackend OK')"
```

Expected: 39 passed, both imports OK.

- [ ] **Step 4: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/__init__.py plugins/ai_chat_plugin/__main__.py
git commit -m "feat(ai-chat): wire ChatConfig into plugin entry points

Both launch_ui() and __main__ create ChatConfig before ChatBackend
and pass it through. Config reference kept on engine to prevent GC.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
