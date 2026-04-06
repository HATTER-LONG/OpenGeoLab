"""Tests for server_core — HTTP server thread and request handling."""
from __future__ import annotations

import json
import urllib.error
import urllib.request
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


def _options(url: str) -> tuple[int, dict[str, str]]:
    """Send an OPTIONS request and return the status and response headers."""
    req = urllib.request.Request(url, method="OPTIONS")
    with urllib.request.urlopen(req) as resp:
        return resp.status, dict(resp.headers.items())


class TestHealthEndpoint:
    """GET /api/v1/health returns status info."""

    def test_health_returns_running(self, server):
        core, base_url = server
        status, body = _get_json(f"{base_url}/api/v1/health")
        assert status == 200
        assert body["status"] == "running"
        assert body["version"] == "1.0"

    def test_health_includes_cors_headers(self, server):
        core, base_url = server
        req = urllib.request.Request(f"{base_url}/api/v1/health", method="GET")
        with urllib.request.urlopen(req) as resp:
            assert resp.headers["Access-Control-Allow-Origin"] == "*"
            assert "GET" in resp.headers["Access-Control-Allow-Methods"]
            assert "Content-Type" in resp.headers["Access-Control-Allow-Headers"]


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

    def test_non_object_json_returns_400(self, server):
        core, base_url = server
        data = b"null"
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

    def test_missing_action_returns_400(self, server):
        core, base_url = server
        status, body = _post_json(f"{base_url}/api/v1/action", {
            "module": "geometry",
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

    def test_options_preflight_returns_204_with_cors_headers(self, server):
        core, base_url = server
        status, headers = _options(f"{base_url}/api/v1/action")
        assert status == 204
        assert headers["Access-Control-Allow-Origin"] == "*"
        assert "POST" in headers["Access-Control-Allow-Methods"]
        assert "Content-Type" in headers["Access-Control-Allow-Headers"]


class TestServerLifecycle:
    """Start/stop and state management."""

    def test_start_and_stop(self, mock_process):
        from http_server_plugin.server_core import ServerCore

        core = ServerCore()
        assert not core.is_running()
        core.start("127.0.0.1", 0)
        assert core.is_running()
        assert core.port > 0
        assert core._thread is not None
        assert core._thread.daemon is True
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
        core.stop()


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
        for _i in range(105):
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
