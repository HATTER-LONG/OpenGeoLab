"""Persistent configuration for the AI Chat Plugin.

Manages authentication method, BYOK provider settings, and model selection.
Configuration is stored as JSON at ``~/.opengeolab/ai_chat_config.json``.
"""
from __future__ import annotations

import copy
import json
from pathlib import Path

from PySide6.QtCore import Property, QObject, Signal


JsonDict = dict[str, object]

_DEFAULT_CONFIG_PATH = Path.home() / ".opengeolab" / "ai_chat_config.json"

_DEFAULTS: JsonDict = {
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


def _deep_merge(defaults: JsonDict, overrides: JsonDict) -> JsonDict:
    """Merge *overrides* into *defaults* recursively, preserving unknown keys.

    Returns a new dict — neither *defaults* nor *overrides* is mutated.
    """
    result = copy.deepcopy(defaults)
    for key, value in overrides.items():
        default_value = result.get(key)
        if isinstance(default_value, dict) and isinstance(value, dict):
            result[key] = _deep_merge(default_value, value)
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
        self._data: JsonDict = {}
        self.load()

    def load(self) -> None:
        """Load configuration from disk; use defaults for missing keys."""
        raw: JsonDict = {}
        if self._path.is_file():
            try:
                parsed = json.loads(self._path.read_text(encoding="utf-8"))
                if isinstance(parsed, dict):
                    raw = parsed
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

    def _byok(self) -> JsonDict:
        """Return the ``byok`` sub-dict, creating it if absent."""
        byok = self._data.setdefault("byok", {})
        if isinstance(byok, dict):
            return byok
        replacement: JsonDict = {}
        self._data["byok"] = replacement
        return replacement

    def _get_auth_method(self) -> str:
        return str(self._data.get("auth_method", "github"))

    def _set_auth_method(self, value: str) -> None:
        if self._data.get("auth_method") != value:
            self._data["auth_method"] = value
            self.authMethodChanged.emit()
            self.save()

    authMethod = Property(
        str, _get_auth_method, _set_auth_method, notify=authMethodChanged,
    )

    def _get_byok_provider(self) -> str:
        return str(self._byok().get("provider", "openai"))

    def _set_byok_provider(self, value: str) -> None:
        if self._byok().get("provider") != value:
            self._byok()["provider"] = value
            self.byokProviderChanged.emit()
            self.save()

    byokProvider = Property(
        str, _get_byok_provider, _set_byok_provider, notify=byokProviderChanged,
    )

    def _get_byok_base_url(self) -> str:
        return str(self._byok().get("base_url", ""))

    def _set_byok_base_url(self, value: str) -> None:
        if self._byok().get("base_url") != value:
            self._byok()["base_url"] = value
            self.byokBaseUrlChanged.emit()
            self.save()

    byokBaseUrl = Property(
        str, _get_byok_base_url, _set_byok_base_url, notify=byokBaseUrlChanged,
    )

    def _get_byok_api_key(self) -> str:
        return str(self._byok().get("api_key", ""))

    def _set_byok_api_key(self, value: str) -> None:
        if self._byok().get("api_key") != value:
            self._byok()["api_key"] = value
            self.byokApiKeyChanged.emit()
            self.save()

    byokApiKey = Property(
        str, _get_byok_api_key, _set_byok_api_key, notify=byokApiKeyChanged,
    )

    def _get_byok_model(self) -> str:
        return str(self._byok().get("model", ""))

    def _set_byok_model(self, value: str) -> None:
        if self._byok().get("model") != value:
            self._byok()["model"] = value
            self.byokModelChanged.emit()
            self.save()

    byokModel = Property(
        str, _get_byok_model, _set_byok_model, notify=byokModelChanged,
    )

    def _get_byok_wire_api(self) -> str:
        return str(self._byok().get("wire_api", "completions"))

    def _set_byok_wire_api(self, value: str) -> None:
        if self._byok().get("wire_api") != value:
            self._byok()["wire_api"] = value
            self.byokWireApiChanged.emit()
            self.save()

    byokWireApi = Property(
        str, _get_byok_wire_api, _set_byok_wire_api, notify=byokWireApiChanged,
    )

    def _get_last_model(self) -> str:
        return str(self._data.get("last_model", ""))

    def _set_last_model(self, value: str) -> None:
        if self._data.get("last_model") != value:
            self._data["last_model"] = value
            self.lastModelChanged.emit()
            self.save()

    lastModel = Property(
        str, _get_last_model, _set_last_model, notify=lastModelChanged,
    )
