"""Copilot SDK tool definitions for OpenGeoLab command integration.

Provides ``build_tools()`` which returns a list of SDK ``Tool`` objects
based on whether the plugin is running in hosted mode (pywrapper available)
or standalone mode (no tools).

Tool handlers are also exposed as module-level functions for testability.
"""
from __future__ import annotations

import json
import threading
import traceback

from pydantic import BaseModel, Field

from copilot import define_tool

from ai_chat_plugin import scene_tools

_MAX_RESULT_BYTES = 50 * 1024  # 50 KB


class _DescribeModuleParams(BaseModel):
    module_name: str = Field(
        description="Module name: geometry, mesh, scene, or io"
    )


class _DescribeActionParams(BaseModel):
    module_name: str = Field(
        description="Module name: geometry, mesh, scene, or io"
    )
    action_name: str = Field(description="Action name within the module")


class _ExecuteActionParams(BaseModel):
    module: str = Field(
        description="Module name: geometry, mesh, scene, or io"
    )
    action: str = Field(description="Action name within the module")
    params: dict = Field(default_factory=dict, description="Action parameters")


class _CaptureViewportParams(BaseModel):
    width: int = Field(default=1024, description="Screenshot width in pixels (default 1024)")
    height: int = Field(default=768, description="Screenshot height in pixels (default 768)")
    format: str = Field(
        default="png",
        description="MIME-type hint for the attached image: png or jpeg (default png). "
        "The actual capture is always PNG; this only affects the attachment label.",
    )
    capture_image: bool = Field(
        default=True,
        description="Whether to return a base64 image attachment (default true). "
        "Set false when only saving to output_path to skip the attachment.",
    )
    output_path: str | None = Field(
        default=None,
        description="Absolute file path to save the captured PNG. "
        "The renderer writes directly to this path. "
        "When set, savedPath (or savedPathError) is returned.",
    )


def _truncate(text: str) -> str:
    """Truncate text to _MAX_RESULT_BYTES with a suffix."""
    if len(text.encode("utf-8", errors="replace")) <= _MAX_RESULT_BYTES:
        return text
    truncated = text[:_MAX_RESULT_BYTES]
    return truncated + '...(truncated)"'


def _list_modules_handler() -> str:
    """Handler for the list_modules tool."""
    modules = scene_tools.list_modules()
    return json.dumps(modules, default=str)


def _describe_module_handler(module_name: str) -> str:
    """Handler for the describe_module tool."""
    result = scene_tools.describe_module(module_name)
    if result is None:
        return json.dumps({
            "ok": False,
            "_request": {"module_name": module_name},
            "error": f"Module '{module_name}' not found",
        })
    return json.dumps({"_request": {"module_name": module_name}, **result}, default=str)


def _describe_action_handler(module_name: str, action_name: str) -> str:
    """Handler for the describe_action tool — returns full param/return schema."""
    query = {"module_name": module_name, "action_name": action_name}
    result = scene_tools.describe_action(module_name, action_name)
    if result is None:
        return json.dumps({
            "ok": False,
            "_request": query,
            "error": f"Action '{action_name}' not found in module '{module_name}'",
        })
    return json.dumps({"_request": query, **result}, default=str)


def _execute_action_handler(module: str, action: str, params: dict) -> str:
    """Handler for the execute_action tool."""
    query = {"module": module, "action": action, "params": params}
    try:
        result = scene_tools.execute_action(module, action, params)
        merged = {"_request": query, **result}
        text = json.dumps(merged, default=str)
        return _truncate(text)
    except Exception:
        return json.dumps(
            {
                "ok": False,
                "_request": query,
                "error": traceback.format_exc(limit=3),
            }
        )


# Thread-local storage for pending attachments (set by tool, read by worker)
_pending_attachments_local = threading.local()


def get_pending_attachments() -> list[dict]:
    """Retrieve and clear pending attachments set by tool handlers."""
    attachments = getattr(_pending_attachments_local, "items", [])
    _pending_attachments_local.items = []
    return attachments


def _capture_viewport_handler(
    width: int,
    height: int,
    fmt: str,
    capture_image: bool = True,
    output_path: str | None = None,
) -> str:
    """Handler for the capture_viewport tool."""
    if not scene_tools.HOSTED_MODE:
        return json.dumps({"ok": False, "error": "Not in hosted mode — pywrapper unavailable"})

    params: dict[str, object] = {
        "width": width,
        "height": height,
        "captureImage": capture_image,
    }
    if output_path:
        params["outputPath"] = output_path

    try:
        result = scene_tools.execute_action(
            "scene", "capture_viewport",
            params,
        )
    except Exception:
        return json.dumps({
            "ok": False,
            "error": f"Capture failed: {traceback.format_exc(limit=3)}",
        })

    # Extract image for BlobAttachment (not returned in tool result)
    image_b64 = result.pop("image", None)
    image_attached = False

    if image_b64 and isinstance(image_b64, str):
        mime_type = "image/jpeg" if fmt == "jpeg" else "image/png"
        blob = {
            "type": "blob",
            "data": image_b64,
            "mimeType": mime_type,
            "displayName": f"viewport.{fmt}",
        }
        if not hasattr(_pending_attachments_local, "items"):
            _pending_attachments_local.items = []
        _pending_attachments_local.items.append(blob)
        image_attached = True

    # Build metadata-only return value
    metadata = result.get("metadata", {})
    tool_return = {
        **metadata,
        "imageAttached": image_attached,
    }

    if not image_attached:
        image_error = result.get("imageError")
        if image_error:
            tool_return["imageError"] = image_error
    saved_path = result.get("savedPath")
    if saved_path:
        tool_return["savedPath"] = saved_path
    saved_path_error = result.get("savedPathError")
    if saved_path_error:
        tool_return["savedPathError"] = saved_path_error

    if image_attached:
        note = "Screenshot captured. Visible shapes and their screen positions are listed above."
    elif saved_path:
        note = f"Screenshot saved to {saved_path}."
    else:
        note = "Screenshot capture failed. Only metadata is available."
    tool_return["note"] = note

    return json.dumps(tool_return, default=str)


def build_tools() -> list:
    """Build the list of SDK Tool objects based on current mode.

    Returns an empty list in standalone mode (no scene tools available).
    """
    if not scene_tools.HOSTED_MODE:
        return []

    @define_tool(
        description=(
            "List all actions in a module. Valid modules: geometry, mesh, scene, io. "
            "Usually not needed — the skill document already lists all actions."
        ),
        skip_permission=True,
    )
    def describe_module(params: _DescribeModuleParams) -> str:
        return _describe_module_handler(params.module_name)

    @define_tool(
        description=(
            "Get the full parameter and return schema for a specific action. "
            "Always call before execute_action. Modules: geometry, mesh, scene, io."
        ),
        skip_permission=True,
    )
    def describe_action(params: _DescribeActionParams) -> str:
        return _describe_action_handler(params.module_name, params.action_name)

    @define_tool(
        description=(
            "Execute an OpenGeoLab action. Modules: geometry, mesh, scene, io. "
            "Always describe_action first to get the correct parameter schema."
        ),
    )
    def execute_action(params: _ExecuteActionParams) -> str:
        return _execute_action_handler(params.module, params.action, params.params)

    @define_tool(
        description=(
            "Capture the current 3D viewport as a screenshot with scene metadata. "
            "Returns camera position, visible shapes with screen bounding boxes, "
            "selections, and labels. The image is automatically attached so you can "
            "see what the user sees. Use this to understand the current view. "
            "Optionally save the PNG to a file via output_path."
        ),
    )
    def capture_viewport(params: _CaptureViewportParams) -> str:
        return _capture_viewport_handler(
            params.width,
            params.height,
            params.format,
            params.capture_image,
            params.output_path,
        )

    return [describe_module, describe_action, execute_action, capture_viewport]
