"""Reusable JSON syntax highlighter for QTextBrowser / QTextEdit.

Uses theme colors from _shared.plugin_theme. Highlights:
- Keys (accent color)
- Strings (green / success)
- Numbers (orange / warning)
- Booleans and null (purple / accent_light)
- Braces, brackets, colons, commas (muted / text_secondary)
"""
from __future__ import annotations

import re

from PySide6.QtGui import QColor, QFont, QSyntaxHighlighter, QTextCharFormat

from _shared.plugin_theme import ThemeColors


class JsonHighlighter(QSyntaxHighlighter):
    """QSyntaxHighlighter subclass for JSON content."""

    def __init__(self, parent=None, *, theme: ThemeColors | None = None) -> None:
        super().__init__(parent)
        self._build_formats(theme)

    def set_theme(self, theme: ThemeColors) -> None:
        """Update highlighting colors for a new theme."""
        self._build_formats(theme)
        self.rehighlight()

    def _build_formats(self, theme: ThemeColors | None) -> None:
        """Construct text formats from theme colors."""
        if theme is None:
            from _shared.plugin_theme import DARK

            theme = DARK

        self._key_fmt = self._make_format(theme.accent, bold=True)
        self._string_fmt = self._make_format(theme.success)
        self._number_fmt = self._make_format(theme.warning)
        self._bool_null_fmt = self._make_format(theme.accent_light, bold=True)
        self._brace_fmt = self._make_format(theme.text_secondary)

        # Patterns applied in order; later matches do NOT override earlier ones
        # because we skip already-formatted regions.
        self._rules: list[tuple[re.Pattern[str], QTextCharFormat]] = [
            # Keys: "key": (captured as the quoted part before a colon)
            (re.compile(r'"[^"\\]*(?:\\.[^"\\]*)*"\s*(?=:)'), self._key_fmt),
            # Strings: "value" (any double-quoted string not followed by colon)
            (re.compile(r'"[^"\\]*(?:\\.[^"\\]*)*"'), self._string_fmt),
            # Numbers: integers and floats (including negative and scientific)
            (re.compile(r'\b-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?\b'), self._number_fmt),
            # Booleans and null
            (re.compile(r'\b(?:true|false|null)\b'), self._bool_null_fmt),
            # Structural characters
            (re.compile(r'[{}\[\]:,]'), self._brace_fmt),
        ]

    @staticmethod
    def _make_format(color_hex: str, bold: bool = False) -> QTextCharFormat:
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color_hex))
        if bold:
            fmt.setFontWeight(QFont.Weight.Bold)
        return fmt

    def highlightBlock(self, text: str) -> None:
        """Apply highlighting rules to a single block of text."""
        formatted = [False] * len(text)

        for pattern, fmt in self._rules:
            for match in pattern.finditer(text):
                start, end = match.start(), match.end()
                if any(formatted[start:end]):
                    continue
                self.setFormat(start, end - start, fmt)
                for i in range(start, end):
                    formatted[i] = True
