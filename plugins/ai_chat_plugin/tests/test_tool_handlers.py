"""Tests for tool_handlers — Copilot SDK tool definitions."""
from __future__ import annotations

import json
from unittest.mock import patch, MagicMock


def test_build_tools_hosted_mode():
    """In hosted mode, build_tools returns 4 tool objects."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        from ai_chat_plugin.tool_handlers import build_tools

        tools = build_tools()
    assert len(tools) == 4
    names = {t.name for t in tools}
    assert names == {"describe_module", "describe_action", "execute_action", "capture_viewport"}


def test_build_tools_standalone_mode():
    """In standalone mode, build_tools returns an empty list."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = False
        from ai_chat_plugin.tool_handlers import build_tools

        tools = build_tools()
    assert len(tools) == 0


def test_list_modules_handler():
    """list_modules tool handler returns JSON string of modules."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.list_modules.return_value = [
            {"name": "geometry", "description": "Shapes"}
        ]
        from ai_chat_plugin.tool_handlers import _list_modules_handler

        result = _list_modules_handler()
    parsed = json.loads(result)
    assert len(parsed) == 1
    assert parsed[0]["name"] == "geometry"


def test_describe_module_handler():
    """describe_module tool handler returns module details."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.describe_module.return_value = {
            "module": "geometry",
            "actions": [{"name": "create_box"}],
        }
        from ai_chat_plugin.tool_handlers import _describe_module_handler

        result = _describe_module_handler("geometry")
    parsed = json.loads(result)
    assert parsed["module"] == "geometry"


def test_describe_module_handler_not_found():
    """describe_module tool handler returns error for unknown module."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.describe_module.return_value = None
        from ai_chat_plugin.tool_handlers import _describe_module_handler

        result = _describe_module_handler("missing")
    parsed = json.loads(result)
    assert parsed["ok"] is False
    assert "missing" in parsed["error"]


def test_describe_action_handler():
    """describe_action returns full action schema."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.describe_action.return_value = {
            "name": "create_box",
            "description": "Create a box",
            "params": {"width": {"type": "number"}},
            "returns": {"ok": {"type": "bool"}},
        }
        from ai_chat_plugin.tool_handlers import _describe_action_handler

        result = _describe_action_handler("geometry", "create_box")
    parsed = json.loads(result)
    assert parsed["name"] == "create_box"
    assert "params" in parsed


def test_describe_action_handler_not_found():
    """describe_action returns error for unknown action."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.describe_action.return_value = None
        from ai_chat_plugin.tool_handlers import _describe_action_handler

        result = _describe_action_handler("geometry", "nonexistent")
    parsed = json.loads(result)
    assert parsed["ok"] is False
    assert "nonexistent" in parsed["error"]


def test_execute_action_handler():
    """execute_action tool handler returns action result with _request."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.execute_action.return_value = {"ok": True, "shapeId": "s1"}
        from ai_chat_plugin.tool_handlers import _execute_action_handler

        result = _execute_action_handler("geometry", "create_box", {"size": 1})
    parsed = json.loads(result)
    assert parsed["ok"] is True
    assert parsed["_request"]["module"] == "geometry"
    assert parsed["_request"]["action"] == "create_box"
    assert parsed["_request"]["params"] == {"size": 1}


def test_execute_action_handler_catches_exception():
    """execute_action returns error JSON when scene_tools raises."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.execute_action.side_effect = RuntimeError("boom")
        from ai_chat_plugin.tool_handlers import _execute_action_handler

        result = _execute_action_handler("geometry", "create_box", {})
    parsed = json.loads(result)
    assert parsed["ok"] is False
    assert "boom" in parsed["error"]


def test_result_truncation():
    """Results exceeding 50 KB are truncated."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.execute_action.return_value = {"ok": True, "data": "x" * 60_000}
        from ai_chat_plugin.tool_handlers import _execute_action_handler

        result = _execute_action_handler("geometry", "big_action", {})
    assert len(result) <= 51_300  # 50 KB + truncation suffix + some overhead
    assert result.endswith('...(truncated)"')


def test_result_truncation_respects_utf8_byte_limit():
    """Truncation keeps the UTF-8 encoded payload within the 50 KB limit."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.execute_action.return_value = {"ok": True, "data": "🎉" * 60_000}
        from ai_chat_plugin.tool_handlers import _execute_action_handler

        result = _execute_action_handler("geometry", "big_action", {})
    assert len(result.encode("utf-8")) <= 51_300
    assert result.endswith('...(truncated)"')
