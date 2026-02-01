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

    /// Emitted when button is clicked
    signal clicked

    width: 28
    height: 28
    radius: 4
    color: mouseArea.containsPress ? Theme.clicked : (mouseArea.containsMouse ? Theme.hovered : "transparent")

    ThemedIcon {
        anchors.centerIn: parent
        source: button.iconSource
        size: 18
        color: Theme.textPrimary
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: button.clicked()
    }

    ToolTip.visible: mouseArea.containsMouse && button.toolTipText !== ""
    ToolTip.text: button.toolTipText
    ToolTip.delay: 500
}
