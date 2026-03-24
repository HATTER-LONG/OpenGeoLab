"""Reusable theme constants and styled widgets for PySide6 plugin windows.

Mirrors the main QML AppTheme so plugin UIs look consistent with the host app.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ThemeColors:
    """Color palette matching AppTheme.qml."""

    bg: str
    surface: str
    surface_muted: str
    surface_strong: str
    text_primary: str
    text_secondary: str
    accent: str
    accent_light: str
    success: str
    danger: str
    warning: str
    border: str
    radius: int = 12


DARK = ThemeColors(
    bg="#0a0d11",
    surface="#10151b",
    surface_muted="#151c24",
    surface_strong="#1d2630",
    text_primary="#f4f7fb",
    text_secondary="#a0acb9",
    accent="#5aa2ff",
    accent_light="#85c0ff",
    success="#6fe3b0",
    danger="#ff8d7d",
    warning="#ffd071",
    border="#27313c",
)

LIGHT = ThemeColors(
    bg="#f9fbfd",
    surface="#ffffff",
    surface_muted="#eef3f8",
    surface_strong="#dfe9f4",
    text_primary="#16283c",
    text_secondary="#60748b",
    accent="#1473e6",
    accent_light="#14ae8a",
    success="#1f9d68",
    danger="#d9534f",
    warning="#d89209",
    border="#d6e0eb",
)


def detect_dark_mode() -> bool:
    """Heuristic: check the host app's palette brightness."""
    try:
        from PySide6.QtWidgets import QApplication

        app = QApplication.instance()
        if app is not None:
            bg = app.palette().color(app.palette().Window)  # type: ignore[arg-type]
            return bg.lightnessF() < 0.5
    except Exception:
        pass
    return True


def current_theme() -> ThemeColors:
    """Return the theme matching the host app's dark/light mode."""
    return DARK if detect_dark_mode() else LIGHT


def build_stylesheet(t: ThemeColors) -> str:
    """Generate a Qt stylesheet matching the AppTheme colors."""
    return f"""
        QWidget {{
            background-color: {t.surface};
            color: {t.text_primary};
            font-family: 'Segoe UI', 'Helvetica Neue', sans-serif;
            font-size: 12px;
        }}
        QLabel {{
            background: transparent;
        }}
        QLabel[role="title"] {{
            font-size: 14px;
            font-weight: 600;
        }}
        QLabel[role="secondary"] {{
            color: {t.text_secondary};
            font-size: 11px;
        }}
        QPushButton {{
            background-color: {t.accent};
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
            font-size: 12px;
        }}
        QPushButton:hover {{
            background-color: {t.accent_light};
        }}
        QPushButton:pressed {{
            background-color: {t.accent};
        }}
        QPushButton:disabled {{
            background-color: {t.surface_strong};
            color: {t.text_secondary};
        }}
        QPushButton[role="secondary"] {{
            background-color: {t.surface_strong};
            color: {t.text_primary};
        }}
        QPushButton[role="secondary"]:hover {{
            background-color: {t.surface_muted};
        }}
        QProgressBar {{
            background-color: {t.surface_strong};
            border: none;
            border-radius: 2px;
            height: 4px;
            text-align: center;
        }}
        QProgressBar::chunk {{
            background-color: {t.accent};
            border-radius: 2px;
        }}
        QFrame[role="card"] {{
            background-color: {t.surface};
            border: 1px solid {t.border};
            border-radius: {t.radius}px;
        }}
    """
