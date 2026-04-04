pragma Singleton

import QtQuick

QtObject {
    id: root

    property bool darkMode: true

    // ── Backgrounds ────────────────────────────────────────────────────
    readonly property color bg: darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color surface: darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted: darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong: darkMode ? "#1d2630" : "#dfe9f4"

    // ── Text ───────────────────────────────────────────────────────────
    readonly property color textPrimary: darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary: darkMode ? "#a0acb9" : "#60748b"
    readonly property color textTertiary: darkMode ? "#7d8997" : "#8397ac"
    readonly property color textOnAccent: "#ffffff"

    // ── Accents ────────────────────────────────────────────────────────
    readonly property color accentA: darkMode ? "#5aa2ff" : "#1473e6"
    readonly property color accentB: darkMode ? "#85c0ff" : "#14ae8a"

    // ── Semantic ───────────────────────────────────────────────────────
    readonly property color success: darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color warning: darkMode ? "#ffd071" : "#d89209"
    readonly property color danger: darkMode ? "#ff8d7d" : "#d9534f"

    // ── Border ─────────────────────────────────────────────────────────
    readonly property color borderSubtle: darkMode ? "#27313c" : "#d6e0eb"

    // ── Spacing ────────────────────────────────────────────────────────
    readonly property int gapTight: 8
    readonly property int gap: 12
    readonly property int gapWide: 16

    // ── Radii ──────────────────────────────────────────────────────────
    readonly property int radiusSmall: 12
    readonly property int radiusMedium: 18

    // ── Typography ─────────────────────────────────────────────────────
    readonly property string monoFont: "Consolas"

    // ── Animation ──────────────────────────────────────────────────────
    readonly property int animFast: 120
    readonly property int animNormal: 140

    /// Return @p c with alpha replaced by @p a.
    function tint(c: color, a: real): color {
        return Qt.rgba(c.r, c.g, c.b, a);
    }
}
