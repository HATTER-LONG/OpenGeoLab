"""Tests for the capture_viewport tool handler."""
from __future__ import annotations

import json
from unittest.mock import patch


def test_capture_viewport_handler_returns_metadata():
    """The handler should return metadata JSON and store image as pending attachment."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": "iVBORw0KGgoAAAANSUhEUg==",
        "metadata": {
            "viewport": {"width": 1024, "height": 768},
            "camera": {"eye": [0, 0, 50], "target": [0, 0, 0], "up": [0, 1, 0]},
            "visibleShapes": [],
            "selections": [],
            "labels": [],
            "hover": None,
        },
    }

    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        mock_st.execute_action.return_value = mock_result
        from ai_chat_plugin.tool_handlers import (
            _capture_viewport_handler,
            get_pending_attachments,
        )

        result_json = _capture_viewport_handler(1024, 768, "png")
        result = json.loads(result_json)
        attachments = get_pending_attachments()

    assert result["viewport"]["width"] == 1024
    assert result["camera"]["eye"] == [0, 0, 50]
    assert result["imageAttached"] is True
    # image field should NOT be in the tool return — it goes to pending attachment
    assert "image" not in result

    assert len(attachments) == 1
    assert attachments[0]["mimeType"] == "image/png"
    assert attachments[0]["data"] == "iVBORw0KGgoAAAANSUhEUg=="


def test_capture_viewport_handler_no_image():
    """When C++ action returns no image, imageAttached should be false."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": None,
        "imageError": "Capture timed out",
        "metadata": {
            "viewport": {"width": 1024, "height": 768},
            "camera": {"eye": [0, 0, 50], "target": [0, 0, 0], "up": [0, 1, 0]},
            "visibleShapes": [],
            "selections": [],
            "labels": [],
            "hover": None,
        },
    }

    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        mock_st.execute_action.return_value = mock_result
        from ai_chat_plugin.tool_handlers import _capture_viewport_handler

        result_json = _capture_viewport_handler(512, 384, "png")
        result = json.loads(result_json)

    assert result["imageAttached"] is False
    assert "imageError" in result


def test_capture_viewport_handler_standalone_mode():
    """In standalone mode, the handler should return an error."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = False
        from ai_chat_plugin.tool_handlers import _capture_viewport_handler

        result_json = _capture_viewport_handler(1024, 768, "png")
        result = json.loads(result_json)

    assert result["ok"] is False
    assert "error" in result


def test_capture_viewport_handler_jpeg_format():
    """Format param controls the MIME type of the blob attachment."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": "fakebase64jpeg",
        "metadata": {"viewport": {"width": 512, "height": 384}},
    }

    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        mock_st.execute_action.return_value = mock_result
        from ai_chat_plugin.tool_handlers import (
            _capture_viewport_handler,
            get_pending_attachments,
        )

        _capture_viewport_handler(512, 384, "jpeg")
        attachments = get_pending_attachments()

    assert len(attachments) == 1
    assert attachments[0]["mimeType"] == "image/jpeg"
    assert attachments[0]["displayName"] == "viewport.jpeg"


def test_capture_viewport_handler_forwards_output_path_and_saved_path():
    """output_path should be forwarded to C++ and savedPath returned to the caller."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": "iVBORw0KGgoAAAANSUhEUg==",
        "savedPath": "C:\\temp\\viewport.png",
        "metadata": {"viewport": {"width": 512, "height": 384}},
    }

    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        mock_st.execute_action.return_value = mock_result
        from ai_chat_plugin.tool_handlers import (
            _capture_viewport_handler,
            get_pending_attachments,
        )

        result_json = _capture_viewport_handler(
            512, 384, "png", output_path="C:\\temp\\viewport.png",
        )
        result = json.loads(result_json)
        attachments = get_pending_attachments()

    mock_st.execute_action.assert_called_once_with(
        "scene",
        "capture_viewport",
        {
            "width": 512,
            "height": 384,
            "captureImage": True,
            "outputPath": "C:\\temp\\viewport.png",
        },
    )
    assert len(attachments) == 1
    assert result["savedPath"] == "C:\\temp\\viewport.png"


def test_capture_viewport_handler_output_path_only():
    """capture_image=False with output_path should skip base64 and report savedPath."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "savedPath": "C:\\temp\\out.png",
        "metadata": {"viewport": {"width": 512, "height": 384}},
    }

    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        mock_st.execute_action.return_value = mock_result
        from ai_chat_plugin.tool_handlers import (
            _capture_viewport_handler,
            get_pending_attachments,
        )

        result_json = _capture_viewport_handler(
            512, 384, "png", capture_image=False, output_path="C:\\temp\\out.png",
        )
        result = json.loads(result_json)
        attachments = get_pending_attachments()

    mock_st.execute_action.assert_called_once_with(
        "scene",
        "capture_viewport",
        {
            "width": 512,
            "height": 384,
            "captureImage": False,
            "outputPath": "C:\\temp\\out.png",
        },
    )
    assert len(attachments) == 0
    assert result["imageAttached"] is False
    assert result["savedPath"] == "C:\\temp\\out.png"
    assert "saved to" in result["note"].lower()


def test_get_pending_attachments_clears_after_read():
    """get_pending_attachments should return items and then clear them."""
    mock_result = {
        "ok": True,
        "action": "capture_viewport",
        "image": "base64data",
        "metadata": {"viewport": {}},
    }

    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        mock_st.execute_action.return_value = mock_result
        from ai_chat_plugin.tool_handlers import (
            _capture_viewport_handler,
            get_pending_attachments,
        )

        _capture_viewport_handler(1024, 768, "png")
        first = get_pending_attachments()
        second = get_pending_attachments()

    assert len(first) == 1
    assert len(second) == 0


def test_build_tools_includes_capture_viewport():
    """build_tools should now return 4 tools including capture_viewport."""
    with patch("ai_chat_plugin.tool_handlers.scene_tools") as mock_st:
        mock_st.HOSTED_MODE = True
        from ai_chat_plugin.tool_handlers import build_tools

        tools = build_tools()
    assert len(tools) == 4
    names = {t.name for t in tools}
    assert "capture_viewport" in names
