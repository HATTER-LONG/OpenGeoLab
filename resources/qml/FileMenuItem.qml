pragma ComponentBehavior: Bound
import QtQuick

/**
 * @file FileMenuItem.qml
 * @brief Menu item component for File menu in Ribbon toolbar
 */
Rectangle {
    id: menuItem

    property string iconText: ""
    property string text: ""
    property string shortcut: ""
    property bool hasSubmenu: false

    signal clicked

    width: parent ? parent.width : 200
    height: 40
    color: itemMouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

    Row {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        spacing: 12

        // Icon
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: menuItem.iconText
            font.pixelSize: 16
            color: "white"
            width: 24
            horizontalAlignment: Text.AlignHCenter
        }

        // Label
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: menuItem.text
            font.pixelSize: 13
            color: "white"
            width: parent.width - 80
        }

        // Shortcut or arrow
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: menuItem.hasSubmenu ? "▶" : menuItem.shortcut
            font.pixelSize: menuItem.hasSubmenu ? 10 : 11
            color: Qt.rgba(1, 1, 1, 0.7)
            horizontalAlignment: Text.AlignRight
        }
    }

    MouseArea {
        id: itemMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: menuItem.clicked()
    }
}
