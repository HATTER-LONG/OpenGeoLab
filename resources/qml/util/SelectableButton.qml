/**
 * @file SelectableButton.qml
 * @brief AbstractButton with selectable state and themed background
 *
 * Provides a toggle-style button that maintains a selected state
 * with visual feedback following the application's theme.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import ".."

AbstractButton {
    id: control

    property bool selected: false
    property color accentColor: Theme.accent

    implicitHeight: 28
    hoverEnabled: true

    background: Rectangle {
        radius: 4
        color: control.selected ? control.accentColor : control.pressed ? Theme.clicked : control.hovered ? Theme.hovered : Theme.surface
        border.width: 1
        border.color: control.selected ? control.accentColor : Theme.border
    }

    // Default content; pages can override contentItem.
    contentItem: Label {
        visible: control.text.length > 0
        text: control.text
        font.pixelSize: 11
        font.bold: control.selected
        color: control.selected ? Theme.white : Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
