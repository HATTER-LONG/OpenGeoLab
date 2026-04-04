"""Backwards-compatible re-export — prefer importing from _shared directly."""
from __future__ import annotations

from _shared.plugin_theme import (  # noqa: F401
    DARK,
    LIGHT,
    ThemeColors,
    build_stylesheet,
    current_theme,
    detect_dark_mode,
)
