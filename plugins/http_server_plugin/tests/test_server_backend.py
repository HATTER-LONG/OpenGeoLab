"""Tests for server_backend — QObject bridge between ServerCore and QML."""
from __future__ import annotations

from unittest.mock import MagicMock, patch

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
            {
                "time": "14:00:00",
                "method": "GET",
                "path": "/api/v1/health",
                "status": 200,
                "ok": True,
                "duration_ms": 1,
            },
        ]
        be.refreshLog()
        log = be.requestLog
        assert len(log) == 1
        assert log[0]["method"] == "GET"

    def test_refresh_log_no_change_no_signal(self, backend):
        be, mock_core = backend
        spy = MagicMock()
        be.requestLogChanged.connect(spy)
        mock_core.get_request_log.return_value = []
        be.refreshLog()
        spy.assert_not_called()

    def test_port_syncs_from_core_after_start(self, backend):
        """When core assigns a different port (e.g. port=0 → random), backend syncs."""
        be, mock_core = backend
        be.port = 0
        mock_core.is_running.return_value = True
        mock_core.port = 54321
        be.start()
        assert be.port == 54321
