from __future__ import annotations

import importlib
import json
import os
import pkgutil
import sys
import traceback
from pathlib import Path

# ---------------------------------------------------------------------------
# On Windows, register the host application directory as a DLL search path
# BEFORE any PySide6 import.  This ensures that the Qt DLLs already loaded
# by the C++ host take priority over PySide6's bundled copies, preventing
# duplicate Qt singletons (two QCoreApplication statics).
# FreeCAD uses the same technique in FreeCADInit.py.
# ---------------------------------------------------------------------------
if sys.platform == "win32":
    _app_root = os.environ.get("OPENGEOLAB_APPLICATION_ROOT", "")
    if _app_root and os.path.isdir(_app_root):
        os.add_dll_directory(_app_root)

PROTOCOL_VERSION = "1.0"


def _plugin_root() -> Path:
    configured_root = os.environ.get("OPENGEOLAB_PLUGIN_ROOT")
    if configured_root:
        return Path(configured_root)

    return Path(__file__).resolve().parent.parent / "plugins" / "python"


def _python_capabilities() -> dict:
    pyside_info = {"available": False}
    try:
        import PySide6  # type: ignore
    except ModuleNotFoundError:
        pass
    else:
        pyside_info = {"available": True, "version": getattr(PySide6, "__version__", "unknown")}

    return {
        "protocolVersion": PROTOCOL_VERSION,
        "embeddedPython": True,
        "pluginRoot": str(_plugin_root()),
        "pyside6": pyside_info,
    }


def _discover_plugins() -> list[dict]:
    root = _plugin_root()
    if not root.exists():
        return []

    discovered: list[dict] = []
    for module_info in pkgutil.iter_modules([str(root)]):
        if module_info.name.startswith("_"):
            continue

        try:
            module = importlib.import_module(module_info.name)
            describe = getattr(module, "describe_plugin", None)
            metadata = describe() if callable(describe) else {"name": module_info.name}
        except Exception:
            metadata = {
                "name": module_info.name,
                "status": "failed",
                "traceback": traceback.format_exc(),
            }

        discovered.append(metadata)

    return discovered


def _import_backend_wrapper():
    return importlib.import_module("opengeolab_pywrapper")


def _plugins_response(request: dict) -> dict:
    return {
        "protocolVersion": PROTOCOL_VERSION,
        "requestId": request.get("requestId", "generated-request"),
        "ok": True,
        "action": request.get("action", "plugins.list"),
        "summary": "Python plugins enumerated.",
        "result": {
            "plugins": _discover_plugins(),
        },
        "diagnostics": {
            "runtime": _python_capabilities(),
        },
        "errors": [],
    }


def _launch_plugin_ui(request: dict) -> dict:
    plugin_name = request.get("payload", {}).get("plugin", "demo_plugin")
    module = importlib.import_module(plugin_name)
    launch_ui = getattr(module, "launch_ui", None)
    if not callable(launch_ui):
        raise RuntimeError(f"Plugin '{plugin_name}' does not expose launch_ui()")

    result = launch_ui()
    return {
        "protocolVersion": PROTOCOL_VERSION,
        "requestId": request.get("requestId", "generated-request"),
        "ok": bool(result.get("ok", True)),
        "action": request.get("action", "plugins.invoke_ui"),
        "summary": result.get("message", "Plugin UI action executed."),
        "result": result,
        "diagnostics": {
            "runtime": _python_capabilities(),
        },
        "errors": [],
    }


def _capabilities_response(request: dict) -> dict:
    return {
        "protocolVersion": PROTOCOL_VERSION,
        "requestId": request.get("requestId", "generated-request"),
        "ok": True,
        "action": "capabilities.query",
        "summary": "Runtime capabilities reported.",
        "result": _python_capabilities(),
        "diagnostics": {"runtime": _python_capabilities()},
        "errors": [],
    }


def process(request_json: str) -> str:
    request = json.loads(request_json)
    action = request.get("action", "")

    if action == "capabilities.query":
        response = _capabilities_response(request)
    elif action == "plugins.list":
        response = _plugins_response(request)
    elif action == "plugins.invoke_ui":
        response = _launch_plugin_ui(request)
    else:
        # Ensure the request has a "module" field for ModuleDispatcher.
        # Backward compatibility: split "geometry.bounding_box" into
        # module="geometry", action="bounding_box".
        if "module" not in request and "." in action:
            module, _, short_action = action.partition(".")
            request["module"] = module
            request["action"] = short_action

        wrapper = _import_backend_wrapper()
        response = json.loads(wrapper.process(json.dumps(request)))
        response.setdefault("diagnostics", {})
        response["diagnostics"]["runtime"] = _python_capabilities()

    return json.dumps(response, indent=2, ensure_ascii=False)
