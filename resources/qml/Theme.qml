pragma Singleton
import QtQuick

QtObject {
    id: theme

    // =========================================================
    // Mode
    // =========================================================
    readonly property int light: 0
    readonly property int dark: 1

    property int mode: light
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

    readonly property color blueLight: '#2c7aa6ff'
    readonly property color blueMid: '#8b7aa6ff'
    readonly property color blueDark: '#b57aa6ff'

    readonly property color redLight: "#D32F2F"
    readonly property color redDark: "#821212"

    // =========================================================
    // Semantic Tokens
    // =========================================================

    // Text
    readonly property color textPrimary: isDark ? white : black

    readonly property color textSecondary: isDark ? gray400 : gray600

    readonly property color textDisabled: isDark ? gray600 : gray400

    // Surface
    readonly property color surface: isDark ? gray900 : white

    readonly property color surfaceAlt: isDark ? gray800 : gray100

    // Border
    readonly property color border: isDark ? gray700 : gray400

    // Accent
    readonly property color accent: isDark ? blueDark : blueLight

    // Status
    readonly property color danger: isDark ? redDark : redLight

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
