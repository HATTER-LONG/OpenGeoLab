/**
 * @file BaseButton.qml
 * @brief Themed button component with consistent styling
 *
 * Provides a reusable button with hover and press states,
 * following the application's theme.
 */
import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: control

    implicitWidth: 60
    implicitHeight: 22
    padding: 6
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        color: control.palette.buttonText
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    background: Rectangle {
        radius: 6
        color: control.down ? Theme.clicked : control.hovered ? Theme.hovered : Theme.surfaceAlt
        border.width: 1
        border.color: Theme.border
    }
}
