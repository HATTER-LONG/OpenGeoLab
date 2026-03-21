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


def _recording_response(
    request: dict, action: str, ok: bool, summary: str, result: dict | None = None
) -> dict:
    return {
        "protocolVersion": PROTOCOL_VERSION,
        "requestId": request.get("requestId"),
        "ok": ok,
        "action": action,
        "summary": summary,
        "result": result or {},
        "errors": [] if ok else [{"message": summary}],
        "diagnostics": {"runtime": _python_capabilities()},
    }


def _generate_script(requests: list[str]) -> str:
    from datetime import datetime, timezone

    lines: list[str] = []
    lines.append("#!/usr/bin/env python3")
    lines.append('"""OpenGeoLab recorded session.')
    lines.append("")
    lines.append(f'Recorded: {datetime.now(tz=timezone.utc).strftime("%Y-%m-%dT%H:%M:%S")}')
    lines.append(f"Requests: {len(requests)}")
    lines.append("")
    lines.append("Usage:")
    lines.append("    Requires opengeolab_pywrapper module in sys.path.")
    lines.append("    Typically run from the OpenGeoLab application build/bin directory.")
    lines.append('"""')
    lines.append("import json")
    lines.append("")
    lines.append("import opengeolab_pywrapper")
    lines.append("")
    lines.append("")
    lines.append("def main():")
    lines.append("    process = opengeolab_pywrapper.process")

    for index, request_json in enumerate(requests, start=1):
        try:
            request = json.loads(request_json)
        except json.JSONDecodeError:
            request = {}

        module_name = request.get("module", "unknown")
        action_name = request.get("action", "unknown")
        clean_request = {
            key: value
            for key, value in request.items()
            if key not in ("requestId", "protocolVersion")
        }

        lines.append("")
        lines.append(f"    # [{index}] {module_name} / {action_name}")
        formatted_json = json.dumps(clean_request, indent=4, ensure_ascii=False)
        lines.append('    result = json.loads(process("""')
        for json_line in formatted_json.splitlines():
            lines.append(f"        {json_line}")
        lines.append('    """))')
        lines.append(
            f"    print(f\"[{index}] {module_name}.{action_name}: ok={{result.get('ok')}}\")"
        )

    lines.append("")
    lines.append("")
    lines.append('if __name__ == "__main__":')
    lines.append("    main()")
    lines.append("")

    return "\n".join(lines)


def _handle_recording_start(request: dict) -> dict:
    wrapper = _import_backend_wrapper()
    if wrapper.is_recording():
        return _recording_response(
            request,
            "recording.start",
            False,
            "Already recording. Stop first or call recording.clear.",
        )

    wrapper.clear_recording()
    wrapper.start_recording()
    return _recording_response(request, "recording.start", True, "Recording started.")


def _handle_recording_stop(request: dict) -> dict:
    wrapper = _import_backend_wrapper()
    wrapper.stop_recording()
    count = len(wrapper.get_recorded_requests())
    return _recording_response(
        request,
        "recording.stop",
        True,
        f"Recording stopped. {count} request(s) captured.",
        {"count": count},
    )


def _handle_recording_status(request: dict) -> dict:
    wrapper = _import_backend_wrapper()
    recording = wrapper.is_recording()
    count = len(wrapper.get_recorded_requests())
    return _recording_response(
        request,
        "recording.status",
        True,
        f"Recording: {'active' if recording else 'inactive'}, {count} request(s).",
        {"recording": recording, "count": count},
    )


def _handle_recording_export(request: dict) -> dict:
    wrapper = _import_backend_wrapper()
    requests = wrapper.get_recorded_requests()
    payload = request.get("payload", {})
    raw_path = payload.get("path", "recorded_session.py")

    output_path = Path(raw_path).resolve()
    try:
        output_path.relative_to(Path.cwd().resolve())
    except ValueError:
        return _recording_response(
            request,
            "recording.export",
            False,
            f"Path must be within working directory: {Path.cwd()}",
        )

    script = _generate_script(requests)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(script, encoding="utf-8")

    return _recording_response(
        request,
        "recording.export",
        True,
        f"Exported {len(requests)} requests to {output_path}",
        {"path": str(output_path), "count": len(requests)},
    )


def _handle_recording_replay(request: dict) -> dict:
    payload = request.get("payload", {})
    script_path = Path(payload.get("path", "")).resolve()

    if not script_path.is_file():
        return _recording_response(
            request,
            "recording.replay",
            False,
            f"Script not found: {script_path}",
        )

    try:
        script_path.relative_to(Path.cwd().resolve())
    except ValueError:
        return _recording_response(
            request,
            "recording.replay",
            False,
            f"Path must be within working directory: {Path.cwd()}",
        )

    wrapper = _import_backend_wrapper()
    was_recording = wrapper.is_recording()
    if was_recording:
        wrapper.stop_recording()

    try:
        code = compile(script_path.read_text("utf-8"), str(script_path), "exec")
        namespace = {"__builtins__": __builtins__, "__name__": "__main__"}
        exec(code, namespace, namespace)
        return _recording_response(
            request,
            "recording.replay",
            True,
            f"Replayed {script_path.name}",
        )
    except Exception as exc:
        return _recording_response(request, "recording.replay", False, str(exc))
    # Note: recording is NOT auto-resumed after replay.
    # User must explicitly call recording.start if needed.


def _handle_recording(action: str, request: dict) -> dict:
    handlers = {
        "recording.start": _handle_recording_start,
        "recording.stop": _handle_recording_stop,
        "recording.status": _handle_recording_status,
        "recording.export": _handle_recording_export,
        "recording.replay": _handle_recording_replay,
    }
    handler = handlers.get(action)
    if handler is None:
        return _recording_response(request, action, False, f"Unknown recording action: {action}")

    return handler(request)


def process(request_json: str) -> str:
    request = json.loads(request_json)
    action = request.get("action", "")

    if action == "capabilities.query":
        response = _capabilities_response(request)
    elif action == "plugins.list":
        response = _plugins_response(request)
    elif action == "plugins.invoke_ui":
        response = _launch_plugin_ui(request)
    elif action.startswith("recording."):
        response = _handle_recording(action, request)
    else:
        # Ensure the request has a "module" field for ModuleDispatcher.
        # DEPRECATED backward compatibility: split "geometry.bounding_box" into
        # module="geometry", action="bounding_box". All callers should use the
        # explicit module+action format directly.
        if "module" not in request and "." in action:
            module, _, short_action = action.partition(".")
            request["module"] = module
            request["action"] = short_action

        wrapper = _import_backend_wrapper()
        response = json.loads(wrapper.process(json.dumps(request)))
        response.setdefault("diagnostics", {})
        response["diagnostics"]["runtime"] = _python_capabilities()

    return json.dumps(response, indent=2, ensure_ascii=False)
