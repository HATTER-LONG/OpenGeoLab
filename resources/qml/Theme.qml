pragma Singleton
import QtQuick

/**
 * @file Theme.qml
 * @brief Dark theme color definitions for OpenGeoLab
 *
 * Provides centralized color palette for consistent dark UI styling.
 * All colors are read-only constants optimized for the dark theme.
 */
QtObject {
    id: theme

    // ========================================================================
    // Primary accent color
    // ========================================================================
    readonly property color accentColor: "#0d6efd"

    // ========================================================================
    // Background colors
    // ========================================================================
    readonly property color windowBackground: "#1a1d23"
    readonly property color panelBackground: "#252830"
    readonly property color tabBarBackground: "#1e2127"
    readonly property color contentBackground: "#252830"
    readonly property color renderBackground: "#2d3238"

    // ========================================================================
    // Border and separator colors
    // ========================================================================
    readonly property color borderColor: "#3a3f4b"
    readonly property color separatorColor: "#3a3f4b"

    // ========================================================================
    // Interactive state colors
    // ========================================================================
    readonly property color hoverColor: "#3a3f4b"
    readonly property color pressedColor: "#4a5568"
    readonly property color selectedColor: "#4a5568"
    readonly property color selectedTabColor: "#323842"

    // ========================================================================
    // Text colors
    // ========================================================================
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#e1e1e1"
    readonly property color textTertiary: "#b8b8b8"
    readonly property color textDisabled: "#6c757d"

    // ========================================================================
    // Icon colors
    // ========================================================================
    readonly property color iconColor: "#e1e1e1"
    readonly property color iconColorDim: "#b0b0b0"

    // ========================================================================
    // Status colors
    // ========================================================================
    readonly property color successColor: "#28a745"
    readonly property color warningColor: "#ffc107"
    readonly property color errorColor: "#dc3545"
    readonly property color infoColor: "#17a2b8"

    // ========================================================================
    // Selection indicator colors
    // ========================================================================
    readonly property color selectionValidBackground: "#2d4a3e"
    readonly property color selectionInvalidBackground: "#4a4a2d"
}
