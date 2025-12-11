import QtQuick

/**
 * @brief Theme manager component for application-wide theme control
 *
 * Provides centralized theme management with support for dark and light modes.
 * Instantiate this as a singleton or shared instance in the root component.
 */
QtObject {
    id: themeManager

    // Current theme: true = dark, false = light
    property bool isDarkTheme: true

    // ========================================================================
    // Color Palette - Automatically switches based on theme
    // ========================================================================

    // Primary accent color
    readonly property color accentColor: isDarkTheme ? "#0d6efd" : "#0078d4"

    // Background colors
    readonly property color windowBackground: isDarkTheme ? "#1a1d23" : "#f3f3f3"
    readonly property color panelBackground: isDarkTheme ? "#252830" : "#ffffff"
    readonly property color tabBarBackground: isDarkTheme ? "#1e2127" : "#f0f0f0"
    readonly property color contentBackground: isDarkTheme ? "#252830" : "#ffffff"

    // Border and separator colors
    readonly property color borderColor: isDarkTheme ? "#3a3f4b" : "#d1d1d1"
    readonly property color separatorColor: isDarkTheme ? "#3a3f4b" : "#e0e0e0"

    // Interactive state colors
    readonly property color hoverColor: isDarkTheme ? "#3a3f4b" : "#e5f1fb"
    readonly property color pressedColor: isDarkTheme ? "#4a5568" : "#cce4f7"
    readonly property color selectedColor: isDarkTheme ? "#4a5568" : "#cce4f7"

    // Text colors
    readonly property color textPrimary: isDarkTheme ? "#ffffff" : "#1a1a1a"
    readonly property color textSecondary: isDarkTheme ? "#e1e1e1" : "#333333"
    readonly property color textTertiary: isDarkTheme ? "#b8b8b8" : "#666666"
    readonly property color textDisabled: isDarkTheme ? "#6c757d" : "#999999"

    // Icon color (for SVG icons using colorOverlay)
    readonly property color iconColor: isDarkTheme ? "#e1e1e1" : "#333333"
    readonly property color iconColorDim: isDarkTheme ? "#b0b0b0" : "#666666"

    // Status colors (consistent across themes)
    readonly property color successColor: "#28a745"
    readonly property color warningColor: "#ffc107"
    readonly property color errorColor: "#dc3545"
    readonly property color infoColor: "#17a2b8"

    // Selection indicator colors
    readonly property color selectionValidBackground: isDarkTheme ? "#2d4a3e" : "#d4edda"
    readonly property color selectionInvalidBackground: isDarkTheme ? "#4a4a2d" : "#fff3cd"

    // ========================================================================
    // Theme Toggle Function
    // ========================================================================

    function toggleTheme() {
        isDarkTheme = !isDarkTheme;
        console.log("Theme switched to:", isDarkTheme ? "Dark" : "Light");
    }

    function setDarkTheme() {
        isDarkTheme = true;
    }

    function setLightTheme() {
        isDarkTheme = false;
    }
}
