/**
 * @file ThemedIcon.qml
 * @brief Theme-aware icon component with color overlay support
 *
 * Renders SVG/image icons with dynamic color tinting based on theme.
 * Handles high-DPI scaling automatically.
 */
import QtQuick
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import OpenGeoLab 1.0

Item {
    id: root

    /// Icon source URL (SVG recommended)
    property url source
    /// Tint color applied via ColorOverlay
    property color color: Theme.palette.text
    /// Base icon size in logical pixels
    property int size: 18

    implicitWidth: size
    implicitHeight: size

    width: implicitWidth
    height: implicitHeight

    readonly property int _pixelWidth: Math.max(1, Math.round(width * Screen.devicePixelRatio))
    readonly property int _pixelHeight: Math.max(1, Math.round(height * Screen.devicePixelRatio))

    Image {
        id: img
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectFit
        visible: false
        smooth: true
        antialiasing: true
        cache: true

        sourceSize.width: root._pixelWidth
        sourceSize.height: root._pixelHeight
    }

    ColorOverlay {
        anchors.fill: img
        source: img
        color: root.color
    }
}
