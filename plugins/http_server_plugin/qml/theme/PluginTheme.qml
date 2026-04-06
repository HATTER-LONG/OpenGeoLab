pragma Singleton

import QtQuick

QtObject {
    id: root

    property bool darkMode: true

    // ── Backgrounds ────────────────────────────────────────────────
    readonly property color bg: darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color surface: darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted: darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong: darkMode ? "#1d2630" : "#dfe9f4"

    // ── Text ───────────────────────────────────────────────────────
    readonly property color textPrimary: darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary: darkMode ? "#a0acb9" : "#60748b"
    readonly property color textTertiary: darkMode ? "#7d8997" : "#8397ac"

    // ── Accents ────────────────────────────────────────────────────
    readonly property color accent: darkMode ? "#5aa2ff" : "#1473e6"

    // ── Semantic ───────────────────────────────────────────────────
    readonly property color success: darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color danger: darkMode ? "#ff8d7d" : "#d9534f"

    // ── Borders ────────────────────────────────────────────────────
    readonly property color border: darkMode ? "#293442" : "#c9d6e3"
    readonly property color borderSubtle: darkMode ? "#1d2630" : "#e0e9f2"

    // ── Spacing ────────────────────────────────────────────────────
    readonly property int gap: 12
    readonly property int radius: 8
}
