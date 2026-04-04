"""Standalone entry point: python -m ai_chat_plugin.

Launches the tab-merged AI Chat window with Chat and Action Debugger tabs.
When run from ``build/bin/plugins/``, the native pywrapper module is
discovered automatically so full schema and execute features are available.
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

    if any(bin_dir.glob("opengeolab_pywrapper*.pyd")):
        bin_str = str(bin_dir)
        if bin_str not in sys.path:
            sys.path.insert(0, bin_str)
        if hasattr(os, "add_dll_directory"):
            os.add_dll_directory(bin_str)
        os.environ["PATH"] = bin_str + os.pathsep + os.environ.get("PATH", "")

        import ctypes
        cmd_dll = bin_dir / "opengeolab_command.dll"
        if cmd_dll.exists():
            ctypes.CDLL(str(cmd_dll), winmode=0)


def _create_engine():
    """Create QQmlApplicationEngine with both backends."""
    from PySide6.QtCore import QUrl
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtQuickControls2 import QQuickStyle

    from ai_chat_plugin.chat_config import ChatConfig
    from ai_chat_plugin.debugger_backend import DebuggerBackend
    from ai_chat_plugin.chat_backend import ChatBackend
    from ai_chat_plugin._qml_setup import setup_engine

    QQuickStyle.setStyle("Basic")

    backend = DebuggerBackend()
    config = ChatConfig()
    chat_backend = ChatBackend(config=config)
    engine = QQmlApplicationEngine()

    qml_dir = Path(__file__).resolve().parent / "qml"
    engine.addImportPath(str(qml_dir))

    engine.setInitialProperties({
        "backend": backend,
        "chatBackend": chat_backend,
    })
    engine.load(QUrl.fromLocalFile(str(qml_dir / "PluginWindow.qml")))

    if not engine.rootObjects():
        print("Error: QML failed to load.", file=sys.stderr)
        sys.exit(1)

    setup_engine(engine, backend)

    engine._backend = backend
    engine._config = config
    engine._chat_backend = chat_backend
    return engine


def main() -> None:
    """Create a QApplication and show the AI Chat window."""
    _setup_standalone_paths()

    from PySide6.QtWidgets import QApplication

    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)

    engine = _create_engine()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
