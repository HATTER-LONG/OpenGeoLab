# Plugin Cleanup & HTTP Server Plugin — Part 2 of 2

> Part 文件：包含 Task 3（实现 http_server_plugin）的完整任务与代码片段，保证自足可读。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Implement `http_server_plugin` — a local HTTP REST API plugin that allows external programs to call any OpenGeoLab action via JSON over HTTP.

**Architecture:** Three-layer design: `server_core.py` (pure Python HTTP server on daemon thread) → `server_backend.py` (QObject bridge for QML) → `ServerWindow.qml` (management panel). Plugin entry point in `__init__.py` follows the existing `ai_chat_plugin` pattern. TDD approach: write tests for each layer before implementing.

**Tech Stack:** Python 3.12+, `http.server` stdlib, PySide6 (QObject + QML), pytest

**Spec:** `docs/superpowers/specs/2025-07-14-plugin-cleanup-and-http-server-design.md`

---

## File Structure

| Action | File | Responsibility |
|--------|------|---------------|
| Create | `plugins/http_server_plugin/__init__.py` | Plugin entry: `describe_plugin()` + `launch_ui()` |
| Create | `plugins/http_server_plugin/server_core.py` | HTTP server thread management + request handling |
| Create | `plugins/http_server_plugin/server_backend.py` | QObject bridge (properties, signals, slots for QML) |
| Create | `plugins/http_server_plugin/qml/ServerWindow.qml` | Management panel UI |
| Create | `plugins/http_server_plugin/qml/theme/qmldir` | QML module declaration for theme singleton |
| Create | `plugins/http_server_plugin/tests/__init__.py` | Test package marker |
| Create | `plugins/http_server_plugin/tests/test_server_core.py` | Unit tests for HTTP server |
| Create | `plugins/http_server_plugin/tests/test_server_backend.py` | Unit tests for QObject bridge |

---

### Task 3a: Implement server_core.py with TDD

**Files:**
- Create: `plugins/http_server_plugin/tests/__init__.py`
- Create: `plugins/http_server_plugin/tests/test_server_core.py`
- Create: `plugins/http_server_plugin/__init__.py` (empty placeholder)
- Create: `plugins/http_server_plugin/server_core.py`

#### Step 1: Create package structure

- [ ] **Step 1: Create plugin package and test directories**

```powershell
New-Item -ItemType Directory -Force -Path plugins\http_server_plugin\tests
New-Item -ItemType Directory -Force -Path plugins\http_server_plugin\qml\theme
```

- [ ] **Step 2: Create empty __init__.py files**

Create `plugins/http_server_plugin/__init__.py`:

```python
"""HTTP Server Plugin — local REST API for OpenGeoLab actions."""
from __future__ import annotations
```

Create `plugins/http_server_plugin/tests/__init__.py`:

```python
"""Tests for http_server_plugin."""
```

#### Step 2: Write failing tests for server_core

- [ ] **Step 3: Write test_server_core.py**

Create `plugins/http_server_plugin/tests/test_server_core.py`:

```python
"""Tests for server_core — HTTP server thread and request handling."""
from __future__ import annotations

import json
import urllib.request
import urllib.error
from unittest.mock import patch

import pytest


@pytest.fixture()
def mock_process():
    """Mock pywrapper.process to return a canned JSON response."""
    def fake_process(request_json: str, progress_callback=None) -> str:
        request = json.loads(request_json)
        return json.dumps({
            "protocolVersion": "1.0",
            "ok": True,
            "module": request.get("module", ""),
            "action": request.get("action", ""),
            "summary": "Mocked.",
            "result": {},
            "errors": [],
        })

    with patch(
        "http_server_plugin.server_core._get_process_fn",
        return_value=fake_process,
    ):
        yield fake_process


@pytest.fixture()
def server(mock_process):
    """Start a ServerCore on a random port and yield (core, base_url)."""
    from http_server_plugin.server_core import ServerCore

    core = ServerCore()
    core.start("127.0.0.1", 0)
    assert core.is_running()
    base_url = f"http://127.0.0.1:{core.port}"
    yield core, base_url
    core.stop()


def _post_json(url: str, body: dict) -> tuple[int, dict]:
    """Send a POST with JSON body, return (status_code, parsed_body)."""
    data = json.dumps(body).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        return exc.code, json.loads(exc.read().decode("utf-8"))


def _get_json(url: str) -> tuple[int, dict]:
    """Send a GET request, return (status_code, parsed_body)."""
    req = urllib.request.Request(url, method="GET")
    try:
        with urllib.request.urlopen(req) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        return exc.code, json.loads(exc.read().decode("utf-8"))


class TestHealthEndpoint:
    """GET /api/v1/health returns status info."""

    def test_health_returns_running(self, server):
        core, base_url = server
        status, body = _get_json(f"{base_url}/api/v1/health")
        assert status == 200
        assert body["status"] == "running"
        assert body["version"] == "1.0"


class TestActionEndpoint:
    """POST /api/v1/action forwards JSON to process()."""

    def test_valid_action_returns_200(self, server):
        core, base_url = server
        status, body = _post_json(f"{base_url}/api/v1/action", {
            "module": "geometry",
            "action": "create_box",
            "param": {"width": 10},
        })
        assert status == 200
        assert body["ok"] is True
        assert body["module"] == "geometry"
        assert body["action"] == "create_box"

    def test_invalid_json_returns_400(self, server):
        core, base_url = server
        data = b"not json at all"
        req = urllib.request.Request(
            f"{base_url}/api/v1/action",
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            with urllib.request.urlopen(req) as resp:
                status = resp.status
                body = json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as exc:
            status = exc.code
            body = json.loads(exc.read().decode("utf-8"))
        assert status == 400
        assert body["ok"] is False
        assert "error" in body

    def test_missing_module_returns_400(self, server):
        core, base_url = server
        status, body = _post_json(f"{base_url}/api/v1/action", {
            "action": "create_box",
        })
        assert status == 400
        assert body["ok"] is False

    def test_process_exception_returns_500(self, server):
        """If process() raises, the server returns 500."""
        core, base_url = server
        def exploding_process(request_json, progress_callback=None):
            raise RuntimeError("kaboom")

        with patch(
            "http_server_plugin.server_core._get_process_fn",
            return_value=exploding_process,
        ):
            status, body = _post_json(f"{base_url}/api/v1/action", {
                "module": "test",
                "action": "boom",
                "param": {},
            })
        assert status == 500
        assert body["ok"] is False
        assert "kaboom" in body["error"]


class TestUnknownRoutes:
    """Unknown paths return 404."""

    def test_get_unknown_path_returns_404(self, server):
        core, base_url = server
        req = urllib.request.Request(f"{base_url}/unknown", method="GET")
        try:
            with urllib.request.urlopen(req) as resp:
                status = resp.status
        except urllib.error.HTTPError as exc:
            status = exc.code
        assert status == 404


class TestServerLifecycle:
    """Start/stop and state management."""

    def test_start_and_stop(self, mock_process):
        from http_server_plugin.server_core import ServerCore

        core = ServerCore()
        assert not core.is_running()
        core.start("127.0.0.1", 0)
        assert core.is_running()
        assert core.port > 0
        core.stop()
        assert not core.is_running()

    def test_double_start_raises(self, mock_process):
        from http_server_plugin.server_core import ServerCore

        core = ServerCore()
        core.start("127.0.0.1", 0)
        with pytest.raises(RuntimeError, match="already running"):
            core.start("127.0.0.1", 0)
        core.stop()

    def test_stop_when_not_running_is_safe(self, mock_process):
        from http_server_plugin.server_core import ServerCore

        core = ServerCore()
        core.stop()  # should not raise


class TestRequestLog:
    """Server core records request log entries."""

    def test_log_entry_after_action(self, server):
        core, base_url = server
        _post_json(f"{base_url}/api/v1/action", {
            "module": "geometry",
            "action": "create_box",
            "param": {},
        })
        log = core.get_request_log()
        assert len(log) == 1
        entry = log[0]
        assert entry["method"] == "POST"
        assert entry["path"] == "/api/v1/action"
        assert entry["module"] == "geometry"
        assert entry["action"] == "create_box"
        assert entry["status"] == 200
        assert entry["ok"] is True
        assert "duration_ms" in entry
        assert "time" in entry

    def test_log_capped_at_100(self, server):
        core, base_url = server
        for i in range(105):
            _get_json(f"{base_url}/api/v1/health")
        log = core.get_request_log()
        assert len(log) == 100

    def test_log_includes_request_response(self, server):
        core, base_url = server
        request_body = {"module": "scene", "action": "fit_to_scene", "param": {}}
        _post_json(f"{base_url}/api/v1/action", request_body)
        log = core.get_request_log()
        entry = log[0]
        assert "request_body" in entry
        assert "response_body" in entry
        assert entry["request_body"]["module"] == "scene"
```

- [ ] **Step 4: Run tests to verify they fail**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\http_server_plugin\tests\test_server_core.py -v
```

Expected: FAIL — `ModuleNotFoundError: No module named 'http_server_plugin.server_core'`

#### Step 3: Implement server_core.py

- [ ] **Step 5: Write server_core.py**

Create `plugins/http_server_plugin/server_core.py`:

```python
"""HTTP server core — daemon-thread HTTP server for OpenGeoLab actions.

Provides a lightweight HTTP API that transparently forwards JSON requests
to the embedded Python runtime's ``process()`` function.
"""
from __future__ import annotations

import json
import threading
import time
from collections import deque
from http.server import HTTPServer, BaseHTTPRequestHandler
from typing import Any, Callable


_LOG_MAX_ENTRIES = 100


def _get_process_fn() -> Callable[[str, Any], str]:
    """Resolve the runtime ``process()`` callable.

    In hosted mode this imports ``opengeolab_runtime.process``.
    Tests can patch this function to supply a mock.
    """
    from opengeolab_runtime import process
    return process


class _RequestHandler(BaseHTTPRequestHandler):
    """Handle incoming HTTP requests for the action API."""

    # Silence per-request log lines from BaseHTTPRequestHandler.
    def log_message(self, format: str, *args: Any) -> None:  # noqa: A002
        pass

    # ── CORS preflight ──────────────────────────────────────────────
    def do_OPTIONS(self) -> None:
        """Respond to CORS preflight requests."""
        self._send_cors_headers(204)
        self.end_headers()

    # ── GET ──────────────────────────────────────────────────────────
    def do_GET(self) -> None:
        start = time.monotonic()
        if self.path == "/api/v1/health":
            body = {"status": "running", "version": "1.0"}
            self._send_json(200, body)
            self._record_log("GET", self.path, 200, True, time.monotonic() - start)
        else:
            self._send_json(404, {"ok": False, "error": "Not found"})
            self._record_log("GET", self.path, 404, False, time.monotonic() - start)

    # ── POST ─────────────────────────────────────────────────────────
    def do_POST(self) -> None:
        start = time.monotonic()
        if self.path != "/api/v1/action":
            self._send_json(404, {"ok": False, "error": "Not found"})
            self._record_log("POST", self.path, 404, False, time.monotonic() - start)
            return

        content_length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(content_length)

        # Parse JSON.
        try:
            request_body = json.loads(raw)
        except (json.JSONDecodeError, UnicodeDecodeError) as exc:
            body = {"ok": False, "error": f"Invalid JSON: {exc}"}
            self._send_json(400, body)
            self._record_log(
                "POST", self.path, 400, False, time.monotonic() - start,
            )
            return

        # Validate required fields.
        if "module" not in request_body or "action" not in request_body:
            body = {"ok": False, "error": "Missing required fields: module, action"}
            self._send_json(400, body)
            self._record_log(
                "POST", self.path, 400, False, time.monotonic() - start,
                request_body=request_body,
            )
            return

        # Forward to process().
        try:
            process_fn = _get_process_fn()
            response_str = process_fn(json.dumps(request_body), None)
            response_body = json.loads(response_str)
            self._send_json(200, response_body)
            self._record_log(
                "POST", self.path, 200,
                response_body.get("ok", False),
                time.monotonic() - start,
                module=request_body.get("module", ""),
                action=request_body.get("action", ""),
                request_body=request_body,
                response_body=response_body,
            )
        except Exception as exc:
            body = {"ok": False, "error": f"Internal server error: {exc}"}
            self._send_json(500, body)
            self._record_log(
                "POST", self.path, 500, False, time.monotonic() - start,
                module=request_body.get("module", ""),
                action=request_body.get("action", ""),
                request_body=request_body,
                response_body=body,
            )

    # ── Helpers ──────────────────────────────────────────────────────
    def _send_cors_headers(self, code: int) -> None:
        self.send_response(code)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def _send_json(self, code: int, body: dict[str, Any]) -> None:
        payload = json.dumps(body).encode("utf-8")
        self._send_cors_headers(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def _record_log(
        self,
        method: str,
        path: str,
        status: int,
        ok: bool,
        elapsed: float,
        *,
        module: str = "",
        action: str = "",
        request_body: dict[str, Any] | None = None,
        response_body: dict[str, Any] | None = None,
    ) -> None:
        core: ServerCore = self.server.core  # type: ignore[attr-defined]
        entry: dict[str, Any] = {
            "time": time.strftime("%H:%M:%S"),
            "method": method,
            "path": path,
            "status": status,
            "ok": ok,
            "duration_ms": round(elapsed * 1000),
        }
        if module:
            entry["module"] = module
        if action:
            entry["action"] = action
        if request_body is not None:
            entry["request_body"] = request_body
        if response_body is not None:
            entry["response_body"] = response_body
        core._append_log(entry)


class ServerCore:
    """Manages an HTTP server running on a daemon thread.

    Usage::

        core = ServerCore()
        core.start("127.0.0.1", 8080)
        # ... server is running ...
        core.stop()
    """

    def __init__(self) -> None:
        self._server: HTTPServer | None = None
        self._thread: threading.Thread | None = None
        self._log: deque[dict[str, Any]] = deque(maxlen=_LOG_MAX_ENTRIES)
        self._lock = threading.Lock()
        self._port: int = 0

    @property
    def port(self) -> int:
        """Return the actual port the server is listening on."""
        return self._port

    def is_running(self) -> bool:
        """Return True if the server thread is alive."""
        return self._thread is not None and self._thread.is_alive()

    def start(self, host: str, port: int) -> None:
        """Start the HTTP server on *host*:*port* in a daemon thread.

        Use ``port=0`` to let the OS pick an available port.

        Raises ``RuntimeError`` if the server is already running.
        Raises ``OSError`` if the port is unavailable.
        """
        if self.is_running():
            raise RuntimeError("Server is already running")

        self._server = HTTPServer((host, port), _RequestHandler)
        self._server.core = self  # type: ignore[attr-defined]
        self._port = self._server.server_address[1]
        self._thread = threading.Thread(
            target=self._server.serve_forever,
            daemon=True,
        )
        self._thread.start()

    def stop(self) -> None:
        """Shut down the server and wait for the thread to exit."""
        if self._server is not None:
            self._server.shutdown()
            self._server.server_close()
        if self._thread is not None:
            self._thread.join(timeout=5)
        self._server = None
        self._thread = None
        self._port = 0

    def get_request_log(self) -> list[dict[str, Any]]:
        """Return a copy of the request log (most recent last)."""
        with self._lock:
            return list(self._log)

    def _append_log(self, entry: dict[str, Any]) -> None:
        with self._lock:
            self._log.append(entry)
```

- [ ] **Step 6: Run tests to verify they pass**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\http_server_plugin\tests\test_server_core.py -v
```

Expected: all tests PASS.

- [ ] **Step 7: Commit**

```powershell
git add plugins/http_server_plugin/__init__.py plugins/http_server_plugin/server_core.py plugins/http_server_plugin/tests/
git commit -m "feat(http_server): add server_core with HTTP API and tests

Implements ServerCore: a daemon-thread HTTP server providing:
- POST /api/v1/action: JSON passthrough to process()
- GET /api/v1/health: health check endpoint
- Request logging (capped at 100 entries)
- CORS headers for browser access

Includes comprehensive pytest suite covering happy path,
error handling, lifecycle, and log management.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3b: Implement server_backend.py with TDD

**Files:**
- Create: `plugins/http_server_plugin/tests/test_server_backend.py`
- Create: `plugins/http_server_plugin/server_backend.py`

- [ ] **Step 1: Write test_server_backend.py**

Create `plugins/http_server_plugin/tests/test_server_backend.py`:

```python
"""Tests for server_backend — QObject bridge between ServerCore and QML."""
from __future__ import annotations

import json
from unittest.mock import MagicMock, patch, PropertyMock

import pytest


@pytest.fixture()
def backend():
    """Create a ServerBackend with a mocked ServerCore."""
    with patch("http_server_plugin.server_backend.ServerCore") as MockCore:
        mock_core = MockCore.return_value
        mock_core.is_running.return_value = False
        mock_core.port = 0
        mock_core.get_request_log.return_value = []

        from http_server_plugin.server_backend import ServerBackend
        be = ServerBackend()
        yield be, mock_core


class TestProperties:
    """Default property values."""

    def test_default_host(self, backend):
        be, _ = backend
        assert be.host == "127.0.0.1"

    def test_default_port(self, backend):
        be, _ = backend
        assert be.port == 8080

    def test_default_not_running(self, backend):
        be, _ = backend
        assert be.running is False


class TestStartStop:
    """Start and stop lifecycle."""

    def test_start_calls_core_start(self, backend):
        be, mock_core = backend
        mock_core.is_running.return_value = True
        mock_core.port = 8080
        be.start()
        mock_core.start.assert_called_once_with("127.0.0.1", 8080)

    def test_start_sets_running_true(self, backend):
        be, mock_core = backend
        mock_core.is_running.return_value = True
        mock_core.port = 8080
        be.start()
        assert be.running is True

    def test_stop_calls_core_stop(self, backend):
        be, mock_core = backend
        mock_core.is_running.return_value = True
        mock_core.port = 8080
        be.start()
        mock_core.is_running.return_value = False
        be.stop()
        mock_core.stop.assert_called_once()
        assert be.running is False

    def test_start_failure_emits_error(self, backend):
        be, mock_core = backend
        mock_core.start.side_effect = OSError("Port 8080 is already in use.")
        error_spy = MagicMock()
        be.errorOccurred.connect(error_spy)
        be.start()
        error_spy.assert_called_once()
        assert "8080" in error_spy.call_args[0][0]
        assert be.running is False

    def test_host_port_changes(self, backend):
        be, _ = backend
        be.host = "0.0.0.0"
        assert be.host == "0.0.0.0"
        be.port = 9090
        assert be.port == 9090


class TestRequestLog:
    """Request log property reflects core state."""

    def test_refresh_log_updates_property(self, backend):
        be, mock_core = backend
        mock_core.get_request_log.return_value = [
            {"time": "14:00:00", "method": "GET", "path": "/api/v1/health",
             "status": 200, "ok": True, "duration_ms": 1},
        ]
        be.refreshLog()
        log = be.requestLog
        assert len(log) == 1
        assert log[0]["method"] == "GET"
```

- [ ] **Step 2: Run tests to verify they fail**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\http_server_plugin\tests\test_server_backend.py -v
```

Expected: FAIL — `ModuleNotFoundError: No module named 'http_server_plugin.server_backend'`

- [ ] **Step 3: Write server_backend.py**

Create `plugins/http_server_plugin/server_backend.py`:

```python
"""QObject bridge between ServerCore and QML management panel.

Exposes properties, signals and slots that the ServerWindow.qml UI binds to.
All property changes emit signals for QML reactivity.
"""
from __future__ import annotations

from PySide6.QtCore import Property, QObject, QTimer, Signal, Slot


class ServerBackend(QObject):
    """Bridge between :class:`ServerCore` and the QML management panel."""

    # ── Signals ──────────────────────────────────────────────────────
    hostChanged = Signal()
    portChanged = Signal()
    runningChanged = Signal()
    requestLogChanged = Signal()
    errorOccurred = Signal(str)
    requestReceived = Signal(dict)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        from http_server_plugin.server_core import ServerCore

        self._core = ServerCore()
        self._host = "127.0.0.1"
        self._port = 8080
        self._running = False
        self._request_log: list[dict] = []

        # Poll core for new log entries while running.
        self._poll_timer = QTimer(self)
        self._poll_timer.setInterval(500)
        self._poll_timer.timeout.connect(self.refreshLog)

    # ── Properties ───────────────────────────────────────────────────
    @Property(str, notify=hostChanged)
    def host(self) -> str:
        return self._host

    @host.setter  # type: ignore[attr-defined]
    def host(self, value: str) -> None:
        if self._host != value:
            self._host = value
            self.hostChanged.emit()

    @Property(int, notify=portChanged)
    def port(self) -> int:
        return self._port

    @port.setter  # type: ignore[attr-defined]
    def port(self, value: int) -> None:
        if self._port != value:
            self._port = value
            self.portChanged.emit()

    @Property(bool, notify=runningChanged)
    def running(self) -> bool:
        return self._running

    @Property("QVariantList", notify=requestLogChanged)
    def requestLog(self) -> list:
        return self._request_log

    # ── Slots ────────────────────────────────────────────────────────
    @Slot()
    def start(self) -> None:
        """Start the HTTP server on the configured host and port."""
        try:
            self._core.start(self._host, self._port)
            self._running = True
            self.runningChanged.emit()
            self._poll_timer.start()
        except Exception as exc:
            self._running = False
            self.runningChanged.emit()
            self.errorOccurred.emit(str(exc))

    @Slot()
    def stop(self) -> None:
        """Stop the HTTP server."""
        self._poll_timer.stop()
        self._core.stop()
        self._running = False
        self.runningChanged.emit()

    @Slot()
    def refreshLog(self) -> None:
        """Refresh the request log from the core."""
        new_log = self._core.get_request_log()
        if len(new_log) != len(self._request_log):
            self._request_log = new_log
            self.requestLogChanged.emit()
```

- [ ] **Step 4: Run tests to verify they pass**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\http_server_plugin\tests\test_server_backend.py -v
```

Expected: all tests PASS.

- [ ] **Step 5: Commit**

```powershell
git add plugins/http_server_plugin/server_backend.py plugins/http_server_plugin/tests/test_server_backend.py
git commit -m "feat(http_server): add ServerBackend QObject bridge with tests

QObject bridge exposing host, port, running, requestLog properties
and start/stop/refreshLog slots for the QML management panel.
Polls ServerCore every 500ms for new log entries while running.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3c: Implement QML Management Panel

**Files:**
- Create: `plugins/http_server_plugin/qml/theme/PluginTheme.qml`
- Create: `plugins/http_server_plugin/qml/theme/qmldir`
- Create: `plugins/http_server_plugin/qml/ServerWindow.qml`

- [ ] **Step 1: Create QML theme singleton**

The HTTP server plugin reuses the same theme as `ai_chat_plugin` for visual consistency.

Create `plugins/http_server_plugin/qml/theme/PluginTheme.qml`:

```qml
pragma Singleton

import QtQuick

QtObject {
    id: root

    property bool darkMode: true

    // ── Backgrounds ────────────────────────────────────────────────
    readonly property color bg: darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color surface: darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted: darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong: darkMode ? "#1d2630" : "#dfe9f4"

    // ── Text ───────────────────────────────────────────────────────
    readonly property color textPrimary: darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary: darkMode ? "#a0acb9" : "#60748b"
    readonly property color textTertiary: darkMode ? "#7d8997" : "#8397ac"

    // ── Accents ────────────────────────────────────────────────────
    readonly property color accent: darkMode ? "#5aa2ff" : "#1473e6"

    // ── Semantic ───────────────────────────────────────────────────
    readonly property color success: darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color danger: darkMode ? "#ff8d7d" : "#d9534f"

    // ── Borders ────────────────────────────────────────────────────
    readonly property color border: darkMode ? "#293442" : "#c9d6e3"
    readonly property color borderSubtle: darkMode ? "#1d2630" : "#e0e9f2"

    // ── Spacing ────────────────────────────────────────────────────
    readonly property int gap: 12
    readonly property int radius: 8
}
```

Create `plugins/http_server_plugin/qml/theme/qmldir`:

```
singleton PluginTheme 1.0 PluginTheme.qml
```

- [ ] **Step 2: Create ServerWindow.qml**

Create `plugins/http_server_plugin/qml/ServerWindow.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

ApplicationWindow {
    id: window

    width: 640
    height: 560
    title: qsTr("HTTP Server — OpenGeoLab")
    color: PluginTheme.bg
    visible: true

    required property var backend

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: PluginTheme.gap
        spacing: PluginTheme.gap

        // ── Config Section ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: configColumn.implicitHeight + 24
            color: PluginTheme.surface
            radius: PluginTheme.radius
            border.width: 1
            border.color: PluginTheme.borderSubtle

            ColumnLayout {
                id: configColumn

                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: qsTr("HTTP Server")
                    color: PluginTheme.textPrimary
                    font.pixelSize: 16
                    font.bold: true
                }

                RowLayout {
                    spacing: 8

                    Text {
                        text: qsTr("Host:")
                        color: PluginTheme.textSecondary
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                    }

                    TextField {
                        id: hostField

                        text: window.backend.host
                        enabled: !window.backend.running
                        Layout.preferredWidth: 140
                        font.pixelSize: 13
                        color: PluginTheme.textPrimary
                        placeholderText: "127.0.0.1"

                        background: Rectangle {
                            color: PluginTheme.surfaceStrong
                            radius: 4
                            border.width: 1
                            border.color: hostField.activeFocus ? PluginTheme.accent : PluginTheme.border
                        }

                        onEditingFinished: window.backend.host = text
                    }

                    Text {
                        text: qsTr("Port:")
                        color: PluginTheme.textSecondary
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                    }

                    TextField {
                        id: portField

                        text: window.backend.port.toString()
                        enabled: !window.backend.running
                        Layout.preferredWidth: 80
                        font.pixelSize: 13
                        color: PluginTheme.textPrimary
                        placeholderText: "8080"
                        validator: IntValidator {
                            bottom: 1
                            top: 65535
                        }

                        background: Rectangle {
                            color: PluginTheme.surfaceStrong
                            radius: 4
                            border.width: 1
                            border.color: portField.activeFocus ? PluginTheme.accent : PluginTheme.border
                        }

                        onEditingFinished: window.backend.port = parseInt(text)
                    }
                }

                RowLayout {
                    spacing: 12

                    Button {
                        id: toggleBtn

                        text: window.backend.running ? qsTr("■ Stop Server") : qsTr("▶ Start Server")
                        font.pixelSize: 13
                        font.bold: true

                        contentItem: Text {
                            text: toggleBtn.text
                            color: "#ffffff"
                            font: toggleBtn.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 130
                            implicitHeight: 32
                            color: window.backend.running ? PluginTheme.danger : PluginTheme.accent
                            radius: 6
                            opacity: toggleBtn.hovered ? 0.85 : 1.0
                        }

                        onClicked: {
                            if (window.backend.running) {
                                window.backend.stop();
                            } else {
                                window.backend.start();
                            }
                        }
                    }

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: window.backend.running ? PluginTheme.success : PluginTheme.danger
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: window.backend.running ? qsTr("Running") : qsTr("Stopped")
                        color: window.backend.running ? PluginTheme.success : PluginTheme.textSecondary
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }
        }

        // ── Error banner ────────────────────────────────────────────
        Rectangle {
            id: errorBanner

            Layout.fillWidth: true
            Layout.preferredHeight: visible ? errorText.implicitHeight + 16 : 0
            visible: false
            color: PluginTheme.danger
            radius: PluginTheme.radius
            opacity: 0.9

            Text {
                id: errorText

                anchors.fill: parent
                anchors.margins: 8
                color: "#ffffff"
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }

            Connections {
                target: window.backend

                function onErrorOccurred(message) {
                    errorText.text = message;
                    errorBanner.visible = true;
                    errorHideTimer.restart();
                }
            }

            Timer {
                id: errorHideTimer

                interval: 5000
                onTriggered: errorBanner.visible = false
            }
        }

        // ── Request Log ─────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: PluginTheme.surface
            radius: PluginTheme.radius
            border.width: 1
            border.color: PluginTheme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: qsTr("Request Log")
                    color: PluginTheme.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                }

                ListView {
                    id: logList

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: window.backend.requestLog
                    clip: true
                    spacing: 2

                    delegate: Rectangle {
                        id: logEntry

                        required property int index
                        required property var modelData

                        width: logList.width
                        height: logContent.implicitHeight + 8
                        color: logEntry.index === logList.currentIndex
                               ? PluginTheme.surfaceStrong
                               : (logMouse.containsMouse ? PluginTheme.surfaceMuted : "transparent")
                        radius: 4

                        ColumnLayout {
                            id: logContent

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: 6
                            spacing: 2

                            Text {
                                text: {
                                    const d = logEntry.modelData;
                                    const label = d.module
                                        ? d.module + "." + d.action
                                        : d.path;
                                    return d.time + "  " + d.method + "  " + label;
                                }
                                color: PluginTheme.textPrimary
                                font.pixelSize: 12
                                font.family: "monospace"
                            }

                            Text {
                                text: {
                                    const d = logEntry.modelData;
                                    const statusText = d.ok ? "OK" : "FAIL";
                                    return "→ " + d.status + " " + statusText
                                           + " (" + d.duration_ms + "ms)";
                                }
                                color: logEntry.modelData.ok
                                       ? PluginTheme.success
                                       : PluginTheme.danger
                                font.pixelSize: 11
                                font.family: "monospace"
                            }
                        }

                        MouseArea {
                            id: logMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: logList.currentIndex = logEntry.index
                        }
                    }

                    onCountChanged: {
                        if (count > 0) {
                            positionViewAtEnd();
                        }
                    }
                }
            }
        }

        // ── Detail Panel ────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            color: PluginTheme.surface
            radius: PluginTheme.radius
            border.width: 1
            border.color: PluginTheme.borderSubtle
            visible: logList.currentIndex >= 0 && logList.currentIndex < window.backend.requestLog.length

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Text {
                    text: qsTr("Request / Response Detail")
                    color: PluginTheme.textPrimary
                    font.pixelSize: 13
                    font.bold: true
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: width
                    contentHeight: detailText.implicitHeight
                    clip: true

                    Text {
                        id: detailText

                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 11
                        font.family: "monospace"
                        color: PluginTheme.textSecondary
                        text: {
                            const idx = logList.currentIndex;
                            const log = window.backend.requestLog;
                            if (idx < 0 || idx >= log.length) return "";
                            const entry = log[idx];
                            let result = "";
                            if (entry.request_body) {
                                result += "Request:\n" + JSON.stringify(entry.request_body, null, 2) + "\n\n";
                            }
                            if (entry.response_body) {
                                result += "Response:\n" + JSON.stringify(entry.response_body, null, 2);
                            }
                            return result || qsTr("No detail available.");
                        }
                    }
                }
            }
        }
    }
}
```

- [ ] **Step 3: Verify build succeeds**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: build succeeds (QML is loaded at runtime; this confirms no resource-level issues).

- [ ] **Step 4: Commit**

```powershell
git add plugins/http_server_plugin/qml/
git commit -m "feat(http_server): add ServerWindow QML management panel

Includes:
- Host/port configuration (editable when stopped)
- Start/Stop toggle with status indicator
- Request log ListView with auto-scroll
- Detail panel showing full request/response JSON
- Error banner with auto-hide timer
- All strings wrapped in qsTr() for i18n

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3d: Implement Plugin Entry Point (__init__.py)

**Files:**
- Modify: `plugins/http_server_plugin/__init__.py`

- [ ] **Step 1: Write __init__.py with describe_plugin and launch_ui**

Replace `plugins/http_server_plugin/__init__.py` with:

```python
"""HTTP Server Plugin — local REST API for OpenGeoLab actions.

Provides a management panel to start/stop an HTTP server that forwards
JSON requests to ``opengeolab_runtime.process()``.
"""
from __future__ import annotations

_active_engines: list = []


def describe_plugin() -> dict:
    """Return plugin metadata for the runtime discovery system."""
    return {
        "name": "HTTP Server",
        "description": "Local REST API for external action execution.",
        "hasUI": True,
    }


def launch_ui() -> dict:
    """Show the HTTP Server management panel."""
    from pathlib import Path

    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtWidgets import QApplication
    from PySide6.QtQuickControls2 import QQuickStyle

    application = QApplication.instance()
    if application is None:
        return {"ok": False, "message": "No QApplication instance."}

    QQuickStyle.setStyle("Basic")

    from http_server_plugin.server_backend import ServerBackend

    backend = ServerBackend()
    engine = QQmlApplicationEngine()

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.setInitialProperties({"backend": backend})
    engine.load(QUrl.fromLocalFile(str(qml_dir / "ServerWindow.qml")))

    if not engine.rootObjects():
        return {"ok": False, "message": "QML failed to load."}

    # Prevent garbage collection of backend and engine.
    engine._backend = backend
    _active_engines.append(engine)

    # Stop server gracefully on app exit.
    application.aboutToQuit.connect(backend.stop)

    return {"ok": True, "message": "HTTP Server panel launched."}
```

- [ ] **Step 2: Commit**

```powershell
git add plugins/http_server_plugin/__init__.py
git commit -m "feat(http_server): add plugin entry point with describe and launch_ui

Follows existing ai_chat_plugin pattern: QQmlApplicationEngine with
setInitialProperties for the backend QObject. Registers aboutToQuit
to stop the server on application exit.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3e: Run All Tests and Final Verification

- [ ] **Step 1: Run all http_server_plugin tests**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\http_server_plugin\tests -v
```

Expected: all tests pass.

- [ ] **Step 2: Run ai_chat_plugin tests (regression check)**

```powershell
.\pyvenv\Scripts\python.exe -m pytest plugins\ai_chat_plugin\tests -q
```

Expected: all tests pass.

- [ ] **Step 3: Run C++ test suite (regression check)**

```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 4: Final commit if any fixes were needed**

If any test fixes were needed, commit them. Otherwise, skip this step.
