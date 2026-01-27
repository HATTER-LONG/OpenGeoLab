pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import ".."

/**
 * @file ViewportPickButton.qml
 * @brief Small icon button for "pick/select from viewport" actions.
 */
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
