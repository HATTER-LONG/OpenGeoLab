"""Shared QML engine setup: attach JsonHighlighters to TextArea documents.

Called after the QQmlApplicationEngine has loaded the QML root object.
Finds named TextArea components and attaches syntax highlighters.
"""
from __future__ import annotations

from PySide6.QtCore import QObject
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtQuick import QQuickTextDocument

from _shared.plugin_theme import DARK, LIGHT, ThemeColors
from ai_chat_plugin.json_highlighter import JsonHighlighter


def _find_child_by_name(root: QObject, name: str) -> QObject | None:
    """Recursively find a child QObject by objectName."""
    for child in root.children():
        if child.objectName() == name:
            return child
        result = _find_child_by_name(child, name)
        if result is not None:
            return result
    return None


def _get_theme(backend) -> ThemeColors:
    """Return the current theme based on backend dark-mode state."""
    return DARK if backend.isDark else LIGHT


def setup_engine(engine: QQmlApplicationEngine, backend) -> None:
    """Attach JsonHighlighters to all named TextArea documents.

    Must be called after engine.load() has completed successfully.

    QML TextArea.textDocument is a QQuickTextDocument.  We extract
    the underlying QTextDocument to pass to QSyntaxHighlighter.
    """
    root = engine.rootObjects()[0]

    highlighters: list[JsonHighlighter] = []

    for obj_name in ("detailTextArea", "requestTextArea", "responseTextArea"):
        area = _find_child_by_name(root, obj_name)
        if area is None:
            continue
        quick_doc = area.property("textDocument")
        if not isinstance(quick_doc, QQuickTextDocument):
            continue
        text_doc = quick_doc.textDocument()
        hl = JsonHighlighter(text_doc, theme=_get_theme(backend))
        highlighters.append(hl)

    # Update all highlighters when theme changes.
    def on_theme_changed() -> None:
        theme = _get_theme(backend)
        for hl in highlighters:
            hl.set_theme(theme)

    backend.isDarkChanged.connect(on_theme_changed)

    # Prevent garbage collection.
    engine._highlighters = highlighters
