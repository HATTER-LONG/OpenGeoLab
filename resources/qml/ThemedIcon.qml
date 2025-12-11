pragma ComponentBehavior: Bound
import QtQuick
import Qt5Compat.GraphicalEffects

/**
 * @brief Themed icon component that applies color overlay to SVG icons
 *
 * Uses ColorOverlay effect to colorize monochrome SVG icons based on current theme.
 * This allows black SVG icons to be visible on both dark and light backgrounds.
 */
Item {
    id: themedIcon

    property alias source: iconImage.source
    property color color: "#e1e1e1"
    property alias fillMode: iconImage.fillMode

    implicitWidth: 22
    implicitHeight: 22

    Image {
        id: iconImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: iconImage
        source: iconImage
        color: themedIcon.color
    }
}
