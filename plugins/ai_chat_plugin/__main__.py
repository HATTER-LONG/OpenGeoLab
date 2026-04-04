"""Standalone entry point: python -m ai_chat_plugin.

Launches the Action Debugger without requiring the C++ host application.
Scene tools are unavailable in standalone mode.
"""
from __future__ import annotations

import sys


def main() -> None:
    """Create a QApplication and show the Action Debugger window."""
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
