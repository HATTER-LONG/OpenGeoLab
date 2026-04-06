"""HTTP server core — daemon-thread HTTP server for OpenGeoLab actions.

Provides a lightweight HTTP API that transparently forwards JSON requests
to the embedded Python runtime's ``process()`` function.
"""
from __future__ import annotations

import json
import threading
import time
from collections import deque
from http.server import BaseHTTPRequestHandler, HTTPServer
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

    def log_message(self, format: str, *args: Any) -> None:  # noqa: A002
        """Silence the default HTTP request logging."""
        return

    def do_OPTIONS(self) -> None:
        """Respond to CORS preflight requests."""
        self._send_cors_headers(204)
        self.end_headers()

    def do_GET(self) -> None:
        """Handle GET requests."""
        start = time.monotonic()
        if self.path == "/api/v1/health":
            body = {"status": "running", "version": "1.0"}
            self._send_json(200, body)
            self._record_log("GET", self.path, 200, True, time.monotonic() - start)
            return

        body = {"ok": False, "error": "Not found"}
        self._send_json(404, body)
        self._record_log("GET", self.path, 404, False, time.monotonic() - start)

    def do_POST(self) -> None:
        """Handle POST requests."""
        start = time.monotonic()
        if self.path != "/api/v1/action":
            body = {"ok": False, "error": "Not found"}
            self._send_json(404, body)
            self._record_log("POST", self.path, 404, False, time.monotonic() - start)
            return

        content_length = int(self.headers.get("Content-Length", 0))
        raw_body = self.rfile.read(content_length)

        try:
            request_body = json.loads(raw_body)
        except (json.JSONDecodeError, UnicodeDecodeError) as exc:
            body = {"ok": False, "error": f"Invalid JSON: {exc}"}
            self._send_json(400, body)
            self._record_log("POST", self.path, 400, False, time.monotonic() - start)
            return

        if not isinstance(request_body, dict):
            body = {"ok": False, "error": "JSON body must be an object"}
            self._send_json(400, body)
            self._record_log(
                "POST",
                self.path,
                400,
                False,
                time.monotonic() - start,
                response_body=body,
            )
            return

        if "module" not in request_body or "action" not in request_body:
            body = {"ok": False, "error": "Missing required fields: module, action"}
            self._send_json(400, body)
            self._record_log(
                "POST",
                self.path,
                400,
                False,
                time.monotonic() - start,
                request_body=request_body,
                response_body=body,
            )
            return

        try:
            process_fn = _get_process_fn()
            response_str = process_fn(json.dumps(request_body), None)
            response_body = json.loads(response_str)
            self._send_json(200, response_body)
            self._record_log(
                "POST",
                self.path,
                200,
                response_body.get("ok", False),
                time.monotonic() - start,
                module=request_body.get("module", ""),
                action=request_body.get("action", ""),
                request_body=request_body,
                response_body=response_body,
            )
        except Exception as exc:  # pragma: no cover - exercised by tests through HTTP.
            body = {"ok": False, "error": f"Internal server error: {exc}"}
            self._send_json(500, body)
            self._record_log(
                "POST",
                self.path,
                500,
                False,
                time.monotonic() - start,
                module=request_body.get("module", ""),
                action=request_body.get("action", ""),
                request_body=request_body,
                response_body=body,
            )

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
    """Manages an HTTP server running on a daemon thread."""

    def __init__(self) -> None:
        self._server: HTTPServer | None = None
        self._thread: threading.Thread | None = None
        self._log: deque[dict[str, Any]] = deque(maxlen=_LOG_MAX_ENTRIES)
        self._lock = threading.Lock()
        self._port = 0

    @property
    def port(self) -> int:
        """Return the actual port the server is listening on."""
        return self._port

    def is_running(self) -> bool:
        """Return whether the HTTP server thread is active."""
        return self._thread is not None and self._thread.is_alive()

    def start(self, host: str, port: int) -> None:
        """Start the HTTP server on *host*:*port* in a daemon thread."""
        if self.is_running():
            raise RuntimeError("Server is already running")

        self._server = HTTPServer((host, port), _RequestHandler)
        self._server.core = self  # type: ignore[attr-defined]
        self._port = self._server.server_address[1]
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
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
        """Return a copy of the request log."""
        with self._lock:
            return list(self._log)

    def _append_log(self, entry: dict[str, Any]) -> None:
        with self._lock:
            self._log.append(entry)
