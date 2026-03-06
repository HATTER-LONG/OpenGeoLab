/**
 * @file ViewToolButton.qml
 * @brief Individual button for the view toolbar
 *
 * Styled icon button with hover effects and tooltip support.
 */
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0

Rectangle {
    id: button

    /// Icon source URL
    property url iconSource
    /// Tooltip text
    property string toolTipText
    /// Toggle state for buttons that act as on/off switches (e.g. X-ray mode)
    property bool toggled: false

    /// Emitted when button is clicked
    signal clicked

    readonly property bool hovered: hoverHandler.hovered
    readonly property bool pressed: tapHandler.pressed

    width: 32
    height: 32
    radius: 6
    color: toggled ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35) : pressed ? Qt.rgba(Theme.clicked.r, Theme.clicked.g, Theme.clicked.b, 0.85) : (hovered ? Qt.rgba(Theme.hovered.r, Theme.hovered.g, Theme.hovered.b, 0.65) : Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.0))

    border.width: 1
    border.color: toggled ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.80) : hovered ? Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.90) : Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.0)

    Behavior on color {
        ColorAnimation {
            duration: 120
            easing.type: Easing.OutQuad
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: 120
            easing.type: Easing.OutQuad
        }
    }

    ThemedIcon {
        anchors.centerIn: parent
        source: button.iconSource
        size: 25
        color: Theme.textPrimary
    }

    HoverHandler {
        id: hoverHandler
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        id: tapHandler
        acceptedButtons: Qt.LeftButton
        onTapped: button.clicked()
    }

    ToolTip.visible: hovered && button.toolTipText !== ""
    ToolTip.text: button.toolTipText
    ToolTip.delay: 500
}
