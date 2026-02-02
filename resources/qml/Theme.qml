/**
 * @file Theme.qml
 * @brief Global theme singleton providing color tokens and palette
 *
 * Supports light/dark mode switching with semantic color definitions
 * for consistent styling across the application.
 */
pragma Singleton
import QtQuick

QtObject {
    id: theme

    // =========================================================
    // Mode
    // =========================================================
    /// Light mode constant
    readonly property int light: 0
    /// Dark mode constant
    readonly property int dark: 1

    /// Current theme mode (light or dark)
    property int mode: light
    /// Convenience property for dark mode check
    readonly property bool isDark: mode === dark

    // =========================================================
    // Raw Tokens
    // =========================================================
    readonly property color black: "#000000"
    readonly property color white: "#ffffff"

    readonly property color gray100: "#f5f5f5"
    readonly property color gray200: "#eeeeee"
    readonly property color gray300: "#e0e0e0"
    readonly property color gray400: "#bdbdbd"
    readonly property color gray500: "#9e9e9e"
    readonly property color gray600: "#757575"
    readonly property color gray700: "#616161"
    readonly property color gray800: "#424242"
    readonly property color gray900: "#212121"
    readonly property color bluegray: "#1E2127"
    readonly property color bluegrayMid: '#252830'
    readonly property color bluegrayDark: '#1e2025'
    readonly property color bluegraySur: '#222429'

    readonly property color blue: '#578fff'
    readonly property color blueLight: '#3a6e9dfb'
    readonly property color blueMid: '#657aa6ff'
    readonly property color blueDark: '#b57aa6ff'

    readonly property color redLight: "#D32F2F"
    readonly property color redDark: "#821212"
    readonly property color greenLight: "#2e7d32"
    readonly property color greenDark: '#107610'

    // =========================================================
    // Semantic Tokens
    // =========================================================

    // Text
    readonly property color textPrimary: isDark ? white : black

    readonly property color textSecondary: isDark ? gray400 : gray600

    readonly property color textDisabled: isDark ? gray600 : gray400

    // Surface
    readonly property color surface: isDark ? bluegraySur : white

    readonly property color surfaceAlt: isDark ? bluegray : gray100

    // Border
    readonly property color border: isDark ? gray700 : gray400

    // Accent
    readonly property color accent: isDark ? blueDark : blue

    // Status
    readonly property color danger: isDark ? redDark : redLight
    readonly property color success: isDark ? greenDark : greenLight
    readonly property color hovered: isDark ? gray800 : gray300
    readonly property color clicked: isDark ? gray600 : gray400

    // Ribbon
    readonly property color ribbonBackground: isDark ? bluegrayMid : gray100
    readonly property color ribbonTabBackground: isDark ? bluegray : blueLight
    readonly property color ribbonFileMenuBackground: isDark ? bluegrayDark : blueMid
    readonly property color ribbonHoverColor: isDark ? gray800 : blueMid
    // =========================================================
    // Palette
    // =========================================================

    readonly property Palette palette: Palette {

        // ================= ACTIVE =================
        window: theme.surface
        windowText: theme.textPrimary

        base: theme.surfaceAlt
        alternateBase: theme.gray200
        text: theme.textPrimary
        placeholderText: theme.textSecondary

        button: theme.surfaceAlt
        buttonText: theme.textPrimary

        highlight: theme.accent
        highlightedText: theme.white

        brightText: theme.danger

        light: theme.gray300
        midlight: theme.gray400
        mid: theme.border
        dark: theme.gray700
        shadow: theme.black

        link: theme.accent
        linkVisited: theme.accent

        toolTipBase: theme.surfaceAlt
        toolTipText: theme.textPrimary

        // ================= DISABLED =================

        disabled.window: theme.palette.window
        disabled.windowText: theme.textDisabled

        disabled.base: theme.palette.base
        disabled.alternateBase: theme.palette.alternateBase
        disabled.text: theme.textDisabled
        disabled.placeholderText: theme.textDisabled

        disabled.button: theme.palette.button
        disabled.buttonText: theme.textDisabled

        disabled.highlight: theme.gray500
        disabled.highlightedText: theme.textDisabled
        disabled.brightText: theme.textDisabled

        disabled.light: theme.gray500
        disabled.midlight: theme.gray600
        disabled.mid: theme.gray600
        disabled.dark: theme.gray700
        disabled.shadow: theme.black

        disabled.link: theme.textDisabled
        disabled.linkVisited: theme.textDisabled
        disabled.toolTipBase: theme.palette.toolTipBase
        disabled.toolTipText: theme.textDisabled

        // ================= INACTIVE =================

        inactive.window: theme.palette.active.window
        inactive.windowText: theme.textSecondary

        inactive.base: theme.palette.active.base
        inactive.alternateBase: theme.palette.active.alternateBase
        inactive.text: theme.textSecondary
        inactive.placeholderText: theme.textSecondary

        inactive.button: theme.palette.active.button
        inactive.buttonText: theme.textSecondary

        inactive.highlight: theme.palette.active.highlight
        inactive.highlightedText: theme.palette.active.highlightedText
        inactive.brightText: theme.palette.active.brightText

        inactive.light: theme.palette.active.light
        inactive.midlight: theme.palette.active.midlight
        inactive.mid: theme.palette.active.mid
        inactive.dark: theme.palette.active.dark
        inactive.shadow: theme.palette.active.shadow

        inactive.link: theme.palette.active.link
        inactive.linkVisited: theme.palette.active.linkVisited
        inactive.toolTipBase: theme.palette.active.toolTipBase
        inactive.toolTipText: theme.textSecondary
    }

    // =========================================================
    // API
    // =========================================================
    function toggleMode() {
        mode = isDark ? light : dark;
    }

    function setMode(nextMode) {
        if (nextMode === light || nextMode === dark)
            mode = nextMode;
    }
}
