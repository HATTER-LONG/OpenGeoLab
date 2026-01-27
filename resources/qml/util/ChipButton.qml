pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import ".."

/**
 * @file ChipButton.qml
 * @brief Pill-style small button (used for quick prompts/tags).
 */
AbstractButton {
    id: control

    implicitHeight: 24
    implicitWidth: chipLabel.implicitWidth + 16
    hoverEnabled: true

    background: Rectangle {
        radius: height / 2
        color: control.pressed ? Theme.clicked : control.hovered ? Theme.hovered : Theme.surfaceAlt
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Label {
        id: chipLabel
        text: control.text
        font.pixelSize: 10
        color: Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
