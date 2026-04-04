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
        /* ── Base ── */
        QWidget {{
            background-color: {t.surface};
            color: {t.text_primary};
            font-family: 'Segoe UI', 'Helvetica Neue', sans-serif;
            font-size: 12px;
        }}

        /* ── Labels ── */
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

        /* ── Buttons ── */
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
        QPushButton[role="icon"] {{
            background-color: transparent;
            border: none;
            border-radius: 6px;
            padding: 4px;
            font-size: 16px;
            min-width: 28px;
            min-height: 28px;
        }}
        QPushButton[role="icon"]:hover {{
            background-color: {t.surface_muted};
        }}

        /* ── Progress bar ── */
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

        /* ── Card frame ── */
        QFrame[role="card"] {{
            background-color: {t.surface};
            border: 1px solid {t.border};
            border-radius: {t.radius}px;
        }}

        /* ── Tree view ── */
        QTreeView {{
            background-color: {t.bg};
            border: 1px solid {t.border};
            border-radius: 8px;
            outline: none;
            padding: 4px;
        }}
        QTreeView::item {{
            padding: 4px 8px;
            border-radius: 4px;
        }}
        QTreeView::item:selected {{
            background-color: {t.accent};
            color: #ffffff;
        }}
        QTreeView::item:hover:!selected {{
            background-color: {t.surface_muted};
        }}
        QTreeView::branch {{
            background: transparent;
        }}

        /* ── Text editors ── */
        QTextEdit, QTextBrowser {{
            background-color: {t.bg};
            color: {t.text_primary};
            border: 1px solid {t.border};
            border-radius: 8px;
            padding: 8px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
            selection-background-color: {t.accent};
            selection-color: #ffffff;
        }}
        QTextEdit:focus {{
            border: 2px solid {t.accent};
        }}

        /* ── Line edit ── */
        QLineEdit {{
            background-color: {t.surface_strong};
            color: {t.text_primary};
            border: 1px solid {t.border};
            border-radius: 6px;
            padding: 4px 8px;
            min-height: 24px;
        }}
        QLineEdit:focus {{
            border: 2px solid {t.accent};
        }}
        QLineEdit:disabled {{
            background-color: {t.surface_muted};
            color: {t.text_secondary};
        }}

        /* ── Spin boxes ── */
        QSpinBox, QDoubleSpinBox {{
            background-color: {t.surface_strong};
            color: {t.text_primary};
            border: 1px solid {t.border};
            border-radius: 6px;
            padding: 4px 8px;
            min-height: 24px;
        }}
        QSpinBox:focus, QDoubleSpinBox:focus {{
            border: 2px solid {t.accent};
        }}
        QSpinBox:disabled, QDoubleSpinBox:disabled {{
            background-color: {t.surface_muted};
            color: {t.text_secondary};
        }}
        QSpinBox::up-button, QSpinBox::down-button,
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {{
            background-color: {t.surface_strong};
            border: none;
            width: 16px;
        }}

        /* ── Check box ── */
        QCheckBox {{
            spacing: 6px;
            background: transparent;
        }}
        QCheckBox::indicator {{
            width: 16px;
            height: 16px;
            border: 2px solid {t.border};
            border-radius: 4px;
            background-color: {t.surface_strong};
        }}
        QCheckBox::indicator:checked {{
            background-color: {t.accent};
            border-color: {t.accent};
        }}
        QCheckBox::indicator:disabled {{
            background-color: {t.surface_muted};
            border-color: {t.surface_muted};
        }}

        /* ── Splitter ── */
        QSplitter::handle {{
            background-color: {t.border};
        }}
        QSplitter::handle:horizontal {{
            width: 2px;
            margin: 4px 1px;
        }}
        QSplitter::handle:vertical {{
            height: 2px;
            margin: 1px 4px;
        }}

        /* ── Scrollbar ── */
        QScrollBar:vertical {{
            background-color: transparent;
            width: 8px;
            margin: 0;
        }}
        QScrollBar::handle:vertical {{
            background-color: {t.surface_strong};
            border-radius: 4px;
            min-height: 20px;
        }}
        QScrollBar::handle:vertical:hover {{
            background-color: {t.text_secondary};
        }}
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
            height: 0;
        }}
        QScrollBar:horizontal {{
            background-color: transparent;
            height: 8px;
            margin: 0;
        }}
        QScrollBar::handle:horizontal {{
            background-color: {t.surface_strong};
            border-radius: 4px;
            min-width: 20px;
        }}
        QScrollBar::handle:horizontal:hover {{
            background-color: {t.text_secondary};
        }}
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
            width: 0;
        }}
        QScrollBar::add-page, QScrollBar::sub-page {{
            background: transparent;
        }}
    """
