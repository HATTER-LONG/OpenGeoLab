pragma Singleton
import QtQuick

QtObject {
    id: theme

    readonly property int light: 0
    readonly property int dark: 1

    property int mode: light

    // =========================================================================
    // Core colors (Light: blue / tech; Dark: near-black / tech with cyan accents)
    // Notes:
    // - backgroundColor: app-level background
    // - surfaceColor: panels / cards / containers
    // - textPrimaryColor/textSecondaryColor: primary/secondary text
    // - primaryColor: brand/theme primary
    // - accentColor: emphasis/highlight
    // =========================================================================
    readonly property color backgroundColor: mode === dark ? "#0B0F1A" : "#F5F8FF"
    readonly property color surfaceColor: mode === dark ? "#101827" : "#FFFFFF"
    readonly property color surfaceAltColor: mode === dark ? "#0F172A" : "#EEF4FF"

    readonly property color textPrimaryColor: mode === dark ? "#E6EDF7" : "#0B1220"
    readonly property color textSecondaryColor: mode === dark ? "#9AA7BD" : "#44546A"
    readonly property color borderColor: mode === dark ? "#273246" : "#D7E1F2"

    readonly property color primaryColor: mode === dark ? "#3B82F6" : "#1D66FF"
    readonly property color accentColor: mode === dark ? "#22D3EE" : "#00D4FF"

    // Optional: softer highlight/selection color
    readonly property color highlightColor: mode === dark ? "#60A5FA" : "#7AA7FF"

    // =========================================================================
    // Button tokens (works well with Qt Quick Controls Basic)
    // =========================================================================
    readonly property color buttonTextColor: "#FFFFFF"
    readonly property color buttonBackgroundColor: primaryColor
    readonly property color buttonHoverColor: mode === dark ? "#4F93FF" : "#2D75FF"
    readonly property color buttonPressedColor: mode === dark ? "#2563EB" : "#1553D6"
    readonly property color buttonDisabledBackgroundColor: mode === dark ? "#2B3445" : "#AAB8D6"
    readonly property color buttonDisabledTextColor: mode === dark ? "#6C7A92" : "#F2F4F8"
    readonly property color buttonBorderColor: mode === dark ? "#2A3A55" : "#B8C7E6"

    // =========================================================================
    // Ribbon colors (keep legacy names to avoid touching existing QML)
    // =========================================================================
    readonly property color ribbonBackgroundColor: surfaceColor
    readonly property color ribbonSeparatorColor: borderColor

    // =========================================================================
    // Ribbon tokens (UI-specific)
    // - Used by RibbonToolBar / RibbonTabContent / RibbonFileMenu
    // - Keep all Ribbon hex values centralized here for light/dark theming
    // =========================================================================
    readonly property color ribbonTabBarColor: mode === dark ? "#1E2127" : surfaceAltColor
    readonly property color ribbonContentColor: mode === dark ? "#252830" : surfaceColor
    readonly property color ribbonBorderColor: mode === dark ? "#363B44" : borderColor

    readonly property color ribbonTextColor: textPrimaryColor
    readonly property color ribbonTextDimColor: textSecondaryColor
    readonly property color ribbonIconColor: mode === dark ? "#E1E1E1" : textPrimaryColor

    readonly property color ribbonAccentColor: highlightColor
    readonly property color ribbonSelectedTabColor: mode === dark ? "#323842" : "#E7EFFD"
    readonly property color ribbonHoverColor: mode === dark ? "#3A3F4B" : "#DCE8FF"
    readonly property color ribbonPressedColor: mode === dark ? "#4A5568" : "#C9DAFF"

    readonly property color ribbonFileMenuLeftColor: mode === dark ? "#1A1D24" : "#EEF4FF"

    // Popup surface used by menus (e.g., File menu). Intentionally different from
    // ribbonContentColor so the popup does not visually merge with the ribbon.
    readonly property color ribbonPopupBackgroundColor: surfaceAltColor
}
