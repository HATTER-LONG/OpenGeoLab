"""
OpenGeoLab Runtime — Python request router.

Routes JSON requests from the C++ host to the correct handler:
plugins, system capabilities, or the C++ wrapper fallback.
"""
from __future__ import annotations

import importlib
import json
import inspect
import os
import pkgutil
import sys
import traceback
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Windows DLL directory registration
# ---------------------------------------------------------------------------
# On Windows, register the application root so PySide6 uses the host Qt DLLs
# instead of its own bundled copies (which would conflict).
if sys.platform == "win32":
    _app_root = os.environ.get("OPENGEOLAB_APPLICATION_ROOT", "")
    if _app_root and hasattr(os, "add_dll_directory"):
        try:
            os.add_dll_directory(_app_root)
        except OSError:
            pass

PROTOCOL_VERSION = "1.0"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _plugin_root() -> Path:
    """Resolve plugin directory from environment or relative path."""
    env = os.environ.get("OPENGEOLAB_PLUGIN_ROOT", "")
    if env:
        return Path(env)
    return Path(__file__).resolve().parent.parent / "plugins"


def _make_response(
    module: str,
    action: str,
    ok: bool,
    summary: str,
    result: dict[str, Any] | None = None,
    errors: list[str] | None = None,
    request_id: str = "",
) -> str:
    """Build a JSON response envelope."""
    response: dict[str, Any] = {
        "protocolVersion": PROTOCOL_VERSION,
        "ok": ok,
        "module": module,
        "action": action,
        "summary": summary,
        "result": result or {},
        "errors": errors or [],
    }
    if request_id:
        response["requestId"] = request_id
    return json.dumps(response)


def _python_capabilities() -> dict[str, Any]:
    """Return runtime capabilities (Python version, PySide6 availability, etc.)."""
    pyside6_available = False
    try:
        import PySide6  # noqa: F401

        pyside6_available = True
    except ImportError:
        pass
    return {
        "pythonVersion": sys.version,
        "protocolVersion": PROTOCOL_VERSION,
        "pyside6Available": pyside6_available,
    }


# ---------------------------------------------------------------------------
# Plugin discovery
# ---------------------------------------------------------------------------


def _discover_plugins() -> list[dict[str, Any]]:
    """Scan the plugin root for modules exposing describe_plugin()."""
    root = _plugin_root()
    if not root.is_dir():
        return []

    plugins: list[dict[str, Any]] = []
    root_str = str(root)
    if root_str not in sys.path:
        sys.path.insert(0, root_str)

    for _finder, name, _is_pkg in pkgutil.iter_modules([root_str]):
        try:
            mod = importlib.import_module(name)
            if hasattr(mod, "describe_plugin"):
                info = mod.describe_plugin()
                info["moduleName"] = name
                plugins.append(info)
        except Exception:
            plugins.append({
                "moduleName": name,
                "name": name,
                "description": f"Failed to load: {traceback.format_exc()}",
                "hasUI": False,
                "error": True,
            })
    return plugins


# ---------------------------------------------------------------------------
# Action handlers
# ---------------------------------------------------------------------------


def _plugins_response(request_id: str = "") -> str:
    """Handle plugins.list — enumerate available plugins."""
    plugins = _discover_plugins()
    return _make_response(
        "plugins",
        "list",
        True,
        "Plugins enumerated.",
        {"plugins": plugins},
        request_id=request_id,
    )


def _execute_plugin(
    request: dict[str, Any], request_id: str = "", progress_callback=None
) -> str:
    """Handle plugins.execute — run a plugin's execute() function."""
    param = request.get("param", {})
    plugin_name = param.get("pluginName", "")
    if not plugin_name:
        return _make_response(
            "plugins",
            "execute",
            False,
            "Missing pluginName in param.",
            request_id=request_id,
        )

    try:
        mod = importlib.import_module(plugin_name)
        if not hasattr(mod, "execute"):
            return _make_response(
                "plugins",
                "execute",
                False,
                f"Plugin '{plugin_name}' has no execute().",
                request_id=request_id,
            )
        sig = inspect.signature(mod.execute)
        if progress_callback and "progress_callback" in sig.parameters:
            result = mod.execute(param, progress_callback=progress_callback)
        else:
            result = mod.execute(param)
        return _make_response(
            "plugins",
            "execute",
            True,
            f"Plugin '{plugin_name}' executed.",
            result,
            request_id=request_id,
        )
    except Exception:
        return _make_response(
            "plugins",
            "execute",
            False,
            f"Plugin execution failed: {traceback.format_exc()}",
            request_id=request_id,
        )


def _launch_plugin_ui(request: dict[str, Any], request_id: str = "") -> str:
    """Handle plugins.invoke_ui — show a PySide6 non-modal window."""
    param = request.get("param", {})
    plugin_name = param.get("pluginName", "")
    if not plugin_name:
        return _make_response(
            "plugins",
            "invoke_ui",
            False,
            "Missing pluginName in param.",
            request_id=request_id,
        )

    try:
        mod = importlib.import_module(plugin_name)
        if not hasattr(mod, "launch_ui"):
            return _make_response(
                "plugins",
                "invoke_ui",
                False,
                f"Plugin '{plugin_name}' has no launch_ui().",
                request_id=request_id,
            )
        result = mod.launch_ui()
        return _make_response(
            "plugins",
            "invoke_ui",
            True,
            "Plugin UI launched.",
            result,
            request_id=request_id,
        )
    except Exception:
        return _make_response(
            "plugins",
            "invoke_ui",
            False,
            f"Plugin UI failed: {traceback.format_exc()}",
            request_id=request_id,
        )


def _capabilities_response(request_id: str = "") -> str:
    """Handle system.capabilities — return runtime info."""
    caps = _python_capabilities()
    return _make_response(
        "system",
        "capabilities",
        True,
        "Capabilities reported.",
        caps,
        request_id=request_id,
    )


def _import_backend_wrapper() -> Any:
    """Lazy-import the C++ pybind11 wrapper module."""
    try:
        import opengeolab_pywrapper

        return opengeolab_pywrapper
    except ImportError:
        return None


# ---------------------------------------------------------------------------
# Main entry point (called from C++ EmbeddedPythonRuntime)
# ---------------------------------------------------------------------------


def process(request_json: str, progress_callback=None) -> str:
    """Route a JSON request to the appropriate handler."""
    try:
        request = json.loads(request_json)
    except json.JSONDecodeError as exc:
        return _make_response("unknown", "unknown", False, f"Invalid JSON: {exc}")

    request_id = request.get("requestId", "")
    module = request.get("module", "")
    action = request.get("action", "")

    # Plugin routes.
    if module == "plugins":
        if action == "list":
            return _plugins_response(request_id)
        if action == "execute":
            return _execute_plugin(request, request_id, progress_callback)
        if action == "invoke_ui":
            return _launch_plugin_ui(request, request_id)
        return _make_response(
            module,
            action,
            False,
            f"Unknown plugins action: {action}",
            request_id=request_id,
        )

    # System routes.
    if module == "system":
        if action == "capabilities":
            return _capabilities_response(request_id)
        return _make_response(
            module,
            action,
            False,
            f"Unknown system action: {action}",
            request_id=request_id,
        )

    # Fallback to C++ wrapper.
    wrapper = _import_backend_wrapper()
    if wrapper is not None:
        return wrapper.process(request_json)

    return _make_response(
        module,
        action,
        False,
        f"No handler for module '{module}'.",
        request_id=request_id,
    )
