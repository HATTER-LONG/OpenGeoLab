pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: tile

    required property AppTheme theme
    property string title: ""
    property string iconKind: "menu"
    property color accentOne: theme.accentA
    property color accentTwo: theme.accentB
    readonly property color iconPrimaryColor: theme.textPrimary
    signal clicked

    implicitWidth: 68
    implicitHeight: implicitWidth
    radius: 14
    color: mouseArea.pressed ? theme.tint(theme.surfaceStrong, theme.darkMode ? 0.94 : 0.98) : (mouseArea.containsMouse ? theme.tint(theme.surfaceMuted, theme.darkMode ? 0.9 : 0.96) : theme.tint(theme.surface, theme.darkMode ? 0.3 : 0.66))
    border.width: 1
    border.color: mouseArea.containsMouse ? theme.tint(tile.accentOne, theme.darkMode ? 0.54 : 0.34) : theme.tint(theme.borderSubtle, 0.72)
    scale: mouseArea.pressed ? 0.97 : (mouseArea.containsMouse ? 1.014 : 1.0)

    Behavior on color {
        ColorAnimation {
            duration: 160
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: mouseArea.containsMouse ? 1 : 0
        border.color: tile.theme.tint(tile.accentTwo, tile.theme.darkMode ? 0.3 : 0.22)
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 1
        height: parent.height * 0.4
        radius: parent.radius - 1
        color: tile.theme.tint(tile.accentOne, mouseArea.containsMouse ? (tile.theme.darkMode ? 0.14 : 0.11) : (tile.theme.darkMode ? 0.07 : 0.055))
    }

    Item {
        anchors.fill: parent
        anchors.margins: 6

        Column {
            anchors.fill: parent
            spacing: 4

            Rectangle {
                width: 36
                height: 36
                radius: 11
                anchors.horizontalCenter: parent.horizontalCenter
                color: tile.theme.tint(tile.accentOne, tile.theme.darkMode ? 0.22 : 0.13)
                border.width: 1
                border.color: tile.theme.tint(tile.accentOne, tile.theme.darkMode ? 0.5 : 0.26)

                AppIcon {
                    anchors.centerIn: parent
                    theme: tile.theme
                    iconKind: tile.iconKind
                    primaryColor: tile.iconPrimaryColor
                    width: 21
                    height: 21
                }
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
                text: tile.title
                color: tile.theme.textPrimary
                font.pixelSize: 9
                font.bold: true
                font.family: tile.theme.bodyFontFamily
                maximumLineCount: 1
                elide: Text.ElideRight
            }
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.clicked()
    }
}
