pragma ComponentBehavior: Bound

import QtQuick

QtObject {
    id: theme

    property bool darkMode: false

    readonly property int shellMargin: 14
    readonly property int shellPadding: 14
    readonly property int gapTight: 8
    readonly property int gap: 12
    readonly property int gapWide: 16
    readonly property int radiusSmall: 12
    readonly property int radiusMedium: 18
    readonly property int radiusLarge: 24
    readonly property string titleFontFamily: "Segoe UI Variable Display"
    readonly property string bodyFontFamily: "Segoe UI"
    readonly property string monoFontFamily: "Consolas"

    readonly property color bg0: darkMode ? "#030405" : "#e7eef7"
    readonly property color bg1: darkMode ? "#07090c" : "#d8e6f5"
    readonly property color bg2: darkMode ? "#0d1116" : "#f3f7fb"
    readonly property color shell: darkMode ? "#0a0d11" : "#f9fbfd"
    readonly property color shellBorder: darkMode ? "#1a212a" : "#c9d5e3"
    readonly property color surface: darkMode ? "#10151b" : "#ffffff"
    readonly property color surfaceMuted: darkMode ? "#151c24" : "#eef3f8"
    readonly property color surfaceStrong: darkMode ? "#1d2630" : "#dfe9f4"
    readonly property color textPrimary: darkMode ? "#f4f7fb" : "#16283c"
    readonly property color textSecondary: darkMode ? "#a0acb9" : "#60748b"
    readonly property color textTertiary: darkMode ? "#7d8997" : "#8397ac"
    readonly property color borderSubtle: darkMode ? "#27313c" : "#d6e0eb"
    readonly property color accentA: darkMode ? "#5aa2ff" : "#1473e6"
    readonly property color accentB: darkMode ? "#85c0ff" : "#14ae8a"
    readonly property color accentC: darkMode ? "#ffca6b" : "#f59e0b"
    readonly property color accentD: darkMode ? "#ff9273" : "#e6613f"
    readonly property color accentE: darkMode ? "#9ab2ff" : "#4263eb"
    readonly property color viewportBase: darkMode ? "#04070a" : "#f6fafe"
    readonly property color viewportGrid: darkMode ? "#25303c" : "#d2dce7"
    readonly property color success: darkMode ? "#6fe3b0" : "#1f9d68"
    readonly property color warning: darkMode ? "#ffd071" : "#d89209"
    readonly property color danger: darkMode ? "#ff8d7d" : "#d9534f"

    function tint(colorValue, alphaValue) {
        return Qt.rgba(colorValue.r, colorValue.g, colorValue.b, alphaValue);
    }
}
