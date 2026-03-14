pragma ComponentBehavior: Bound

import QtQuick

import "../theme"
import "../components" as Components

Rectangle {
    id: panel

    required property AppTheme theme
    required property bool darkMode
    required property bool menuOpen
    signal triggerAction(string actionKey)

    visible: panel.menuOpen
    z: 10
    x: 0
    y: 34
    width: 248
    radius: 18
    color: panel.theme.surface
    border.width: 1
    border.color: panel.theme.borderSubtle
    opacity: panel.menuOpen ? 1 : 0
    scale: panel.menuOpen ? 1 : 0.96
    implicitHeight: menuColumn.implicitHeight + 24

    Behavior on opacity {
        NumberAnimation {
            duration: 140
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 140
            easing.type: Easing.OutCubic
        }
    }

    Column {
        id: menuColumn

        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Text {
            text: "Workspace"
            color: panel.theme.textSecondary
            font.pixelSize: 12
            font.bold: true
            font.family: panel.theme.bodyFontFamily
        }

        Rectangle {
            width: parent.width
            radius: 14
            color: panel.theme.tint(panel.theme.surfaceMuted, panel.theme.darkMode ? 0.5 : 0.74)
            border.width: 1
            border.color: panel.theme.tint(panel.theme.borderSubtle, 0.7)
            implicitHeight: workspaceMenuGroup.implicitHeight + 20

            Column {
                id: workspaceMenuGroup

                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: "Import Model"
                    iconKind: "import"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.2 : 0.11)
                    pressedColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.3 : 0.18)
                    onClicked: panel.triggerAction("importModel")
                }

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: "Export Model"
                    iconKind: "export"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.2 : 0.11)
                    pressedColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.3 : 0.18)
                    onClicked: panel.triggerAction("exportModel")
                }

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: panel.darkMode ? "Switch to Light" : "Switch to Dark"
                    iconKind: panel.darkMode ? "lightTheme" : "darkTheme"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.18 : 0.1)
                    pressedColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.28 : 0.16)
                    hoverBorderColor: panel.theme.tint(panel.theme.accentA, panel.theme.darkMode ? 0.58 : 0.34)
                    onClicked: panel.triggerAction("toggleTheme")
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: panel.theme.tint(panel.theme.borderSubtle, 0.6)
        }

        Text {
            text: "Script Recorder"
            color: panel.theme.textSecondary
            font.pixelSize: 12
            font.bold: true
            font.family: panel.theme.bodyFontFamily
        }

        Rectangle {
            width: parent.width
            radius: 14
            color: panel.theme.tint(panel.theme.surfaceMuted, panel.theme.darkMode ? 0.46 : 0.72)
            border.width: 1
            border.color: panel.theme.tint(panel.theme.borderSubtle, 0.7)
            implicitHeight: recorderMenuGroup.implicitHeight + 20

            Column {
                id: recorderMenuGroup

                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: "Start Script Record"
                    iconKind: "record"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.2 : 0.11)
                    pressedColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.3 : 0.18)
                    onClicked: panel.triggerAction("recordSelection")
                }

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: "Replay Script"
                    iconKind: "replay"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.2 : 0.11)
                    pressedColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.3 : 0.18)
                    onClicked: panel.triggerAction("replayCommands")
                }

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: "Export Record"
                    iconKind: "exportRecord"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.2 : 0.11)
                    pressedColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.3 : 0.18)
                    onClicked: panel.triggerAction("exportScript")
                }

                Components.ActionButton {
                    theme: panel.theme
                    width: parent.width
                    buttonText: "Clear Script History"
                    iconKind: "clear"
                    leftAligned: true
                    buttonColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.2 : 0.11)
                    pressedColor: panel.theme.tint(panel.theme.accentB, panel.theme.darkMode ? 0.3 : 0.18)
                    hoverBorderColor: panel.theme.tint(panel.theme.accentD, panel.theme.darkMode ? 0.58 : 0.34)
                    onClicked: panel.triggerAction("clearRecordedCommands")
                }
            }
        }
    }
}