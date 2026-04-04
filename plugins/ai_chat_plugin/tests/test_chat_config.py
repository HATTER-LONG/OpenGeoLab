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

    def test_instances_do_not_share_default_state(self, tmp_path: Path) -> None:
        path_a = tmp_path / "a" / "config.json"
        path_b = tmp_path / "b" / "config.json"

        cfg_a = ChatConfig(config_path=path_a)
        cfg_a.byokProvider = "anthropic"

        cfg_b = ChatConfig(config_path=path_b)
        assert cfg_b.byokProvider == "openai"  # must NOT be "anthropic"
