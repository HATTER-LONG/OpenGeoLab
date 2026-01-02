pragma Singleton
import QtQuick

/**
 * @file Theme.qml
 * @brief Application theme singleton for consistent styling
 *
 * Provides centralized color definitions and theme tokens for light/dark modes.
 * All UI components should reference these properties for consistent theming.
 *
 * @note This is a QML singleton - use Theme.propertyName to access values.
 */
QtObject {
    id: theme

    // =========================================================================
    // Theme Mode Constants
    // =========================================================================

    /**
     * @brief Light theme mode identifier
     */
    readonly property int light: 0

    /**
     * @brief Dark theme mode identifier
     */
    readonly property int dark: 1

    /**
     * @brief Current theme mode (light or dark)
     */
    property int mode: light

    // =========================================================================
    // Core Colors
    // =========================================================================

    /**
     * @brief Application-level background color
     */
    // Dark theme is intentionally neutral (avoid deep-blue cast).
    readonly property color backgroundColor: mode === dark ? "#0D0F14" : "#F5F8FF"

    /**
     * @brief Surface color for panels, cards, and containers
     */
    readonly property color surfaceColor: mode === dark ? "#151821" : "#FFFFFF"

    /**
     * @brief Alternative surface color for visual hierarchy
     */
    readonly property color surfaceAltColor: mode === dark ? "#1B1F2A" : "#EEF4FF"

    /**
     * @brief Primary text color for main content
     */
    readonly property color textPrimaryColor: mode === dark ? "#E7EAF0" : "#0B1220"

    /**
     * @brief Secondary text color for labels and hints
     */
    readonly property color textSecondaryColor: mode === dark ? "#A3ACBD" : "#44546A"

    /**
     * @brief Border color for UI elements
     */
    readonly property color borderColor: mode === dark ? "#303646" : "#D7E1F2"

    /**
     * @brief Primary brand/theme color
     */
    readonly property color primaryColor: mode === dark ? "#3D7FF0" : "#1D66FF"

    /**
     * @brief Accent color for emphasis and highlights
     */
    readonly property color accentColor: mode === dark ? "#22D3EE" : "#00D4FF"

    /**
     * @brief Highlight color for selection states
     */
    readonly property color highlightColor: mode === dark ? "#6AA6FF" : "#7AA7FF"

    /**
     * @brief Error/danger indicator color
     */
    readonly property color errorColor: "#EF5350"

    // =========================================================================
    // Input Tokens (TextField / TextArea / SpinBox / ComboBox)
    // =========================================================================

    /**
     * @brief Input background color
     */
    readonly property color inputBackgroundColor: mode === dark ? "#11141B" : "#FFFFFF"

    /**
     * @brief Input background color when disabled
     */
    readonly property color inputDisabledBackgroundColor: mode === dark ? "#0F1218" : "#EEF4FF"

    /**
     * @brief Input border color
     */
    readonly property color inputBorderColor: borderColor

    /**
     * @brief Input border color when focused
     */
    readonly property color inputBorderFocusColor: primaryColor

    /**
     * @brief Input text color
     */
    readonly property color inputTextColor: textPrimaryColor

    /**
     * @brief Input placeholder text color
     */
    readonly property color inputPlaceholderColor: textSecondaryColor

    // =========================================================================
    // Button Tokens
    // =========================================================================

    /**
     * @brief Button text color (typically white)
     */
    readonly property color buttonTextColor: "#FFFFFF"

    /**
     * @brief Button background color (uses primary)
     */
    readonly property color buttonBackgroundColor: primaryColor

    /**
     * @brief Button hover state background
     */
    readonly property color buttonHoverColor: mode === dark ? "#4F93FF" : "#2D75FF"

    /**
     * @brief Button pressed state background
     */
    readonly property color buttonPressedColor: mode === dark ? "#2563EB" : "#1553D6"

    /**
     * @brief Disabled button background
     */
    readonly property color buttonDisabledBackgroundColor: mode === dark ? "#2B3445" : "#AAB8D6"

    /**
     * @brief Disabled button text color
     */
    readonly property color buttonDisabledTextColor: mode === dark ? "#6C7A92" : "#F2F4F8"

    /**
     * @brief Button border color
     */
    readonly property color buttonBorderColor: mode === dark ? "#2A3A55" : "#B8C7E6"

    // =========================================================================
    // Ribbon Colors
    // =========================================================================

    /**
     * @brief Ribbon toolbar background
     */
    readonly property color ribbonBackgroundColor: surfaceColor

    /**
     * @brief Ribbon separator line color
     */
    readonly property color ribbonSeparatorColor: borderColor

    /**
     * @brief Ribbon tab bar background
     */
    readonly property color ribbonTabBarColor: mode === dark ? "#1E2127" : surfaceAltColor

    /**
     * @brief Ribbon content area background
     */
    readonly property color ribbonContentColor: mode === dark ? "#252830" : surfaceColor

    /**
     * @brief Ribbon border color
     */
    readonly property color ribbonBorderColor: mode === dark ? "#363B44" : borderColor

    /**
     * @brief Ribbon text color
     */
    readonly property color ribbonTextColor: textPrimaryColor

    /**
     * @brief Ribbon dimmed text color
     */
    readonly property color ribbonTextDimColor: textSecondaryColor

    /**
     * @brief Ribbon icon color
     */
    readonly property color ribbonIconColor: mode === dark ? "#E1E1E1" : textPrimaryColor

    /**
     * @brief Ribbon accent/highlight color
     */
    readonly property color ribbonAccentColor: highlightColor

    /**
     * @brief Selected ribbon tab background
     */
    readonly property color ribbonSelectedTabColor: mode === dark ? "#323842" : "#E7EFFD"

    /**
     * @brief Ribbon hover state background
     */
    readonly property color ribbonHoverColor: mode === dark ? "#3A3F4B" : "#DCE8FF"

    /**
     * @brief Ribbon pressed state background
     */
    readonly property color ribbonPressedColor: mode === dark ? "#4A5568" : "#C9DAFF"

    /**
     * @brief Ribbon file menu left panel color
     */
    readonly property color ribbonFileMenuLeftColor: mode === dark ? "#1A1D24" : "#EEF4FF"

    /**
     * @brief Ribbon popup background color
     */
    readonly property color ribbonPopupBackgroundColor: mode === dark ? '#191d28' : '#ebf5ff'

    // =========================================================================
    // Dialog Colors
    // =========================================================================

    /**
     * @brief Success indicator color
     */
    readonly property color dialogSuccessColor: "#4CAF50"

    /**
     * @brief Success hover state color
     */
    readonly property color dialogSuccessHoverColor: "#43A047"

    /**
     * @brief Success pressed state color
     */
    readonly property color dialogSuccessPressedColor: "#388E3C"

    /**
     * @brief Error indicator color
     */
    readonly property color dialogErrorColor: "#EF5350"

    /**
     * @brief Error hover state color
     */
    readonly property color dialogErrorHoverColor: "#E53935"

    /**
     * @brief Error pressed state color
     */
    readonly property color dialogErrorPressedColor: "#C62828"

    /**
     * @brief Dialog details panel background
     */
    readonly property color dialogDetailsBgColor: mode === dark ? "#2A2A2A" : "#F5F5F5"
}
