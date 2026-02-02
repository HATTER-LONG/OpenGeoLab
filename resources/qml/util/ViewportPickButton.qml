/**
 * @file ViewportPickButton.qml
 * @brief Icon button for viewport selection actions
 *
 * A compact button that triggers pick/select operations
 * from the 3D viewport, with hover feedback and tooltip.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import ".."

AbstractButton {
    id: control

    property string iconText: "🎯"
    property string toolTipText: qsTr("Pick from viewport")

    implicitWidth: 24
    implicitHeight: 24
    hoverEnabled: true

    background: Rectangle {
        radius: 4
        color: control.hovered ? Theme.hovered : "transparent"
    }

    contentItem: Label {
        text: control.iconText
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ToolTip.visible: hovered
    ToolTip.text: control.toolTipText
    ToolTip.delay: 500
}
