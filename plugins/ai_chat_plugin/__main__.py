"""Standalone entry point: python -m ai_chat_plugin.

Launches the Action Debugger window.  When run from ``build/bin/plugins/``,
the native pywrapper module is discovered automatically so the full schema
and execute features are available.
"""
from __future__ import annotations

import os
import sys
from pathlib import Path


def _setup_standalone_paths() -> None:
    """Add build/bin to sys.path and DLL search dirs for pywrapper access."""
    plugin_pkg = Path(__file__).resolve().parent   # ai_chat_plugin/
    plugins_dir = plugin_pkg.parent                # plugins/
    bin_dir = plugins_dir.parent                   # build/bin/

    # Look for the compiled pywrapper .pyd next to the plugins directory.
    if any(bin_dir.glob("opengeolab_pywrapper*.pyd")):
        bin_str = str(bin_dir)
        if bin_str not in sys.path:
            sys.path.insert(0, bin_str)
        # Windows: register DLL search directory for native deps.
        if hasattr(os, "add_dll_directory"):
            os.add_dll_directory(bin_str)
        os.environ["PATH"] = bin_str + os.pathsep + os.environ.get("PATH", "")

        # Preload the command library so transitive DLL deps are resolved
        # before Python attempts to import the pybind11 module.
        import ctypes

        cmd_dll = bin_dir / "opengeolab_command.dll"
        if cmd_dll.exists():
            ctypes.CDLL(str(cmd_dll), winmode=0)


def main() -> None:
    """Create a QApplication and show the Action Debugger window."""
    _setup_standalone_paths()

    from PySide6.QtWidgets import QApplication

    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)

    from ai_chat_plugin.action_debugger import ActionDebuggerWindow

    window = ActionDebuggerWindow(embedded=False)
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
