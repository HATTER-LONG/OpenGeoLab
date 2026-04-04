"""Copilot SDK tool definitions for OpenGeoLab command integration.

Provides ``build_tools()`` which returns a list of SDK ``Tool`` objects
based on whether the plugin is running in hosted mode (pywrapper available)
or standalone mode (no tools).

Tool handlers are also exposed as module-level functions for testability.
"""
from __future__ import annotations

import json
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

    return [describe_module, describe_action, execute_action]
