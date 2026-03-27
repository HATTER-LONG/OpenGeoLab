pragma ComponentBehavior: Bound

import QtQuick

/**
 * @brief Central theme object providing colors, spacing, fonts and dark-mode toggle.
 *
 * Instantiate once in Main.qml and pass via `required property AppTheme theme`.
 */
QtObject {
    id: root

    // ── Mode & locale ──────────────────────────────────────────────────
    property bool darkMode: false
    readonly property string locale: "en_US"

    // ── Spacing ────────────────────────────────────────────────────────
    readonly property int shellMargin: 14
    readonly property int shellPadding: 14
    readonly property int gapTight: 8
    readonly property int gap: 12
    readonly property int gapWide: 16

    // ── Radii ──────────────────────────────────────────────────────────
    readonly property int radiusSmall: 12
    readonly property int radiusMedium: 18
    readonly property int radiusLarge: 24

    // ── Typography ─────────────────────────────────────────────────────
    readonly property string monoFontFamily: "Consolas"

    // ── Backgrounds ────────────────────────────────────────────────────
    readonly property color bg0: darkMode ? "#030405" : "#e7eef7"
    readonly property color bg1: darkMode ? "#07090c" : "#d8e6f5"
    readonly property color bg2: darkMode ? "#0d1116" : "#f3f7fb"

    // ── Shell ──────────────────────────────────────────────────────────
    readonly property color shell: darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color shellBorder: darkMode ? "#1a212a" : "#c9d5e3"

    // ── Surfaces ───────────────────────────────────────────────────────
    readonly property color surface: darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted: darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong: darkMode ? "#1d2630" : "#dfe9f4"

    // ── Text ───────────────────────────────────────────────────────────
    readonly property color textPrimary: darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary: darkMode ? "#a0acb9" : "#60748b"
    readonly property color textTertiary: darkMode ? "#7d8997" : "#8397ac"

    // ── Border ─────────────────────────────────────────────────────────
    readonly property color borderSubtle: darkMode ? "#27313c" : "#d6e0eb"

    // ── Accents ────────────────────────────────────────────────────────
    readonly property color accentA: darkMode ? "#5aa2ff" : "#1473e6"
    readonly property color accentB: darkMode ? "#85c0ff" : "#14ae8a"
    readonly property color accentC: darkMode ? "#ffca6b" : "#f59e0b"
    readonly property color accentD: darkMode ? "#ff9273" : "#e6613f"
    readonly property color accentE: darkMode ? "#9ab2ff" : "#4263eb"

    // ── Viewport ───────────────────────────────────────────────────────
    readonly property color viewportBase: darkMode ? "#04070a" : "#f6fafe"
    readonly property color viewportGrid: darkMode ? "#25303c" : "#d2dce7"

    // ── Semantic ───────────────────────────────────────────────────────
    readonly property color success: darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color warning: darkMode ? "#ffd071" : "#d89209"
    readonly property color danger: darkMode ? "#ff8d7d" : "#d9534f"

    /// Return @p colorValue with its alpha replaced by @p alphaValue.
    function tint(colorValue: color, alphaValue: real): color {
        return Qt.rgba(colorValue.r, colorValue.g, colorValue.b, alphaValue);
    }

    /// Return the accent color property identified by @p name, defaulting to accentA.
    function accentByName(name: string): color {
        const map = {
            "accentA": accentA,
            "accentB": accentB,
            "accentC": accentC,
            "accentD": accentD,
            "accentE": accentE
        };
        return map[name] ?? accentA;
    }

    // ── Pre-computed color sets ──────────────────────────────────────────
    readonly property QtObject ribbonTile: QtObject {
        readonly property color normal: root.tint(root.surface, root.darkMode ? 0.3 : 0.66)
        readonly property color hovered: root.tint(root.surfaceMuted, root.darkMode ? 0.9 : 0.96)
        readonly property color pressed: root.tint(root.surfaceStrong, root.darkMode ? 0.94 : 0.98)
        readonly property color borderNormal: root.tint(root.borderSubtle, root.darkMode ? 0.88 : 0.72)
        readonly property color iconBg: root.tint(root.surface, root.darkMode ? 0.82 : 0.95)
    }

    readonly property QtObject panel: QtObject {
        readonly property color normal: root.tint(root.surface, root.darkMode ? 0.7 : 1.0)
        readonly property color border: root.tint(root.borderSubtle, 0.7)
        readonly property color tabBarBorder: root.tint(root.borderSubtle, 0.78)
        readonly property color tabActiveBg: root.tint(root.accentA, root.darkMode ? 0.2 : 0.12)
        readonly property color tabActiveBorder: root.tint(root.accentA, root.darkMode ? 0.42 : 0.24)
        readonly property color tabHovered: root.tint(root.surfaceStrong, root.darkMode ? 0.54 : 0.74)
        readonly property color menuBg: root.tint(root.surfaceMuted, root.darkMode ? 0.5 : 0.74)
        readonly property color menuBorder: root.tint(root.borderSubtle, 0.7)
        readonly property color menuRecorderBg: root.tint(root.surfaceMuted, root.darkMode ? 0.46 : 0.72)
        readonly property color separator: root.tint(root.borderSubtle, 0.6)
    }

    /// Return a color set for ActionButton based on accent name and alpha scale.
    /// @param accentName  AppTheme accent name (e.g. "accentA", "accentE")
    /// @param alphaScale  "normal" or "muted" (dimmer for tool buttons); defaults to "normal"
    function actionButtonColors(accentName: string, alphaScale: string): var {
        const scale = alphaScale || "normal";
        const accent = root.accentByName(accentName);
        if (scale === "muted") {
            return {
                normal: root.tint(accent, root.darkMode ? 0.18 : 0.1),
                pressed: root.tint(accent, root.darkMode ? 0.28 : 0.16)
            };
        }
        return {
            normal: root.tint(accent, root.darkMode ? 0.2 : 0.11),
            pressed: root.tint(accent, root.darkMode ? 0.3 : 0.18)
        };
    }

    /// Return the hover border color for a specific accent.
    /// Only used by buttons that explicitly specify hoverAccent in MenuConfig.
    function accentHoverBorder(accentName: string): color {
        const accent = root.accentByName(accentName);
        return root.tint(accent, root.darkMode ? 0.58 : 0.34);
    }
}
