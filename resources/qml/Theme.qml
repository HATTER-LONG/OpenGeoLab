pragma Singleton
import QtQuick
import QtQuick.Controls.Material

QtObject {
    id: theme

    readonly property int light: 0
    readonly property int dark: 1

    // Expose the active theme; assign Theme.dark or Theme.light at runtime.
    property int mode: Material.theme === Material.Dark ? dark : light

    readonly property color accentColor: "#0d6efd"
    readonly property color ribbonBackgroundColor: mode === dark ? "#0f172a" : "#2c3e50"
}
