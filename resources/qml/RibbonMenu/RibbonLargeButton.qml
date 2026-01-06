pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0

AbstractButton {
    id: root

    property string iconSource: ""
    property string tooltipText: ""

    implicitWidth: 52
    implicitHeight: 60

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Rectangle {
        radius: 3
        border.width: root.hovered || root.activeFocus ? 1 : 0
        border.color: Theme.ribbonHoverColor

        color: root.pressed ? Theme.clicked : (root.hovered ? Theme.ribbonHoverColor : "transparent")
    }

    contentItem: Column {
        anchors.fill: parent
        spacing: 2

        ThemedIcon {
            id: icon
            source: root.iconSource
            width: 32
            height: 32
            color: root.hovered ? Theme.palette.highlight : Theme.textPrimary
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            id: label
            text: root.text
            font.pixelSize: 12
            color: root.hovered ? Theme.palette.highlight : Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
    ToolTip.visible: root.hovered && root.tooltipText.length > 0
    ToolTip.text: root.tooltipText
    ToolTip.delay: 500

    Accessible.name: root.text
    Accessible.description: root.tooltipText
}
