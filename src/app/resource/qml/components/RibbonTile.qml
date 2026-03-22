pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: tile

    required property AppTheme theme
    required property var actionHandler
    property string actionKey: ""
    property string title: ""
    property string iconKind: "menu"
    property color accentOne: theme.accentA
    property color accentTwo: theme.accentB
    readonly property color iconPrimaryColor: theme.textPrimary

    implicitWidth: 68
    implicitHeight: implicitWidth
    radius: 14
    color: mouseArea.pressed ? theme.ribbonTile.pressed : (mouseArea.containsMouse ? theme.ribbonTile.hovered : theme.ribbonTile.normal)
    border.width: 1
    border.color: mouseArea.containsMouse ? theme.tint(tile.accentOne, theme.darkMode ? 0.62 : 0.34) : theme.ribbonTile.borderNormal
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
        id: accentPlate

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 0
        height: 50
        radius: 0
        topLeftRadius: 14
        topRightRadius: 14
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.width: 1
        border.color: tile.theme.tint(tile.accentTwo, mouseArea.containsMouse ? (tile.theme.darkMode ? 0.4 : 0.24) : (tile.theme.darkMode ? 0.24 : 0.14))
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: tile.theme.tint(tile.accentOne, mouseArea.containsMouse ? (tile.theme.darkMode ? 0.22 : 0.16) : (tile.theme.darkMode ? 0.14 : 0.1))
            }
            GradientStop {
                position: 1.0
                color: tile.theme.tint(tile.accentTwo, mouseArea.containsMouse ? (tile.theme.darkMode ? 0.18 : 0.12) : (tile.theme.darkMode ? 0.1 : 0.07))
            }
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 6

        Column {
            anchors.fill: parent
            spacing: 4

            Item {
                width: parent.width
                height: 42

                Rectangle {
                    width: 42
                    height: 42
                    radius: 12
                    anchors.centerIn: parent
                    color: tile.theme.ribbonTile.iconBg
                    border.width: 1
                    border.color: tile.theme.tint(tile.accentOne, tile.theme.darkMode ? 0.44 : 0.24)

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: 11
                        color: "transparent"
                        border.width: mouseArea.containsMouse ? 1 : 0
                        border.color: tile.theme.tint(tile.accentTwo, tile.theme.darkMode ? 0.3 : 0.16)
                    }

                    AppIcon {
                        anchors.centerIn: parent
                        theme: tile.theme
                        iconKind: tile.iconKind
                        primaryColor: tile.iconPrimaryColor
                        width: 26
                        height: 26
                    }
                }
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
                text: tile.title
                color: tile.theme.textPrimary
                font.pixelSize: 10
                font.bold: true
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
        onClicked: tile.actionHandler(tile.actionKey)
    }
}
