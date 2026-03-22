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
    border.color: mouseArea.containsMouse ? theme.tint(tile.accentOne, theme.darkMode ? 0.62 : 0.34) : theme.tint(theme.borderSubtle, theme.darkMode ? 0.88 : 0.72)
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
                    width: 38
                    height: 38
                    radius: 12
                    anchors.centerIn: parent
                    color: tile.theme.tint(tile.theme.surface, tile.theme.darkMode ? 0.82 : 0.95)
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
                        width: 21
                        height: 21
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
