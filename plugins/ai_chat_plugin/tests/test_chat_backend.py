"""Tests for ChatBackend — model listing and availableModels property."""
from __future__ import annotations

import json
import sys
from unittest.mock import MagicMock, patch

import pytest

_app = None


def _ensure_app():
    global _app
    if _app is None:
        from PySide6.QtWidgets import QApplication

        _app = QApplication.instance() or QApplication(sys.argv)
    return _app


@pytest.fixture(autouse=True)
def qt_app():
    _ensure_app()


def _make_backend(tmp_path):
    """Create a ChatBackend with worker auto-start disabled.

    Patches CopilotWorker so the real QThread never starts, while keeping
    the Qt Signal.connect() calls working via MagicMock.
    """
    from ai_chat_plugin.chat_config import ChatConfig

    config_path = tmp_path / "cfg" / "ai_chat_config.json"
    config = ChatConfig(config_path=config_path)

    with patch(
        "ai_chat_plugin.chat_backend.CopilotWorker",
    ) as MockWorker:
        instance = MockWorker.return_value
        # Prevent actual thread start
        instance.start = MagicMock()
        instance.isRunning.return_value = False

        from ai_chat_plugin.chat_backend import ChatBackend

        backend = ChatBackend(config=config)
    return backend


class TestAvailableModels:
    """Tests for SDK model listing integration."""

    def test_initial_models_empty(self, tmp_path) -> None:
        backend = _make_backend(tmp_path)
        assert backend.availableModels == []

    def test_on_models_loaded_updates_property(self, tmp_path) -> None:
        backend = _make_backend(tmp_path)
        models = [
            {"id": "gpt-4o", "name": "GPT-4o", "state": "enabled"},
            {"id": "claude-sonnet-4.5", "name": "Claude Sonnet", "state": "enabled"},
        ]
        backend._on_models_loaded(json.dumps(models))
        assert len(backend.availableModels) == 2
        assert backend.availableModels[0]["id"] == "gpt-4o"

    def test_on_models_loaded_emits_signal(self, tmp_path) -> None:
        backend = _make_backend(tmp_path)
        received = []
        backend.availableModelsChanged.connect(lambda: received.append(True))
        backend._on_models_loaded(json.dumps([{"id": "test-model", "name": "Test"}]))
        assert len(received) == 1

    def test_models_replaced_on_reconnect(self, tmp_path) -> None:
        backend = _make_backend(tmp_path)
        backend._on_models_loaded(json.dumps([{"id": "old-model", "name": "Old"}]))
        backend._on_models_loaded(json.dumps([{"id": "new-model", "name": "New"}]))
        assert len(backend.availableModels) == 1
        assert backend.availableModels[0]["id"] == "new-model"

    def test_invalid_json_yields_empty(self, tmp_path) -> None:
        backend = _make_backend(tmp_path)
        backend._on_models_loaded("not valid json {{{")
        assert backend.availableModels == []
