pragma ComponentBehavior: Bound

import QtQuick

import "../theme"
import "../components" as Components

Rectangle {
    id: sidebar

    required property AppTheme theme

    radius: theme.radiusLarge
    color: theme.surface
    border.width: 1
    border.color: theme.borderSubtle

    Flickable {
        anchors.fill: parent
        anchors.margins: 14
        contentWidth: width
        contentHeight: cardsColumn.implicitHeight
        clip: true

        Column {
            id: cardsColumn

            width: parent.width
            spacing: 0

            Components.SectionCard {
                width: parent.width
                height: 320
                theme: sidebar.theme
                title: qsTr("Sidebar")
                subtitle: qsTr("Reserved for future workbench tools, properties, and workflow panels.")
                introDelay: 40

                Column {
                    width: parent.width
                    spacing: 12

                    Rectangle {
                        width: parent.width
                        height: 220
                        radius: 18
                        color: sidebar.theme.tint(sidebar.theme.surface, sidebar.theme.darkMode ? 0.72 : 0.92)
                        border.width: 1
                        border.color: sidebar.theme.tint(sidebar.theme.borderSubtle, 0.84)

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 16
                            radius: 14
                            color: "transparent"
                            border.width: 1
                            border.color: sidebar.theme.tint(sidebar.theme.accentA, sidebar.theme.darkMode ? 0.28 : 0.18)
                        }

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 48
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            text: qsTr("Workspace panel\n\nFuture: model tree, property inspector, task pane.")
                            color: sidebar.theme.textSecondary
                            font.pixelSize: 14
                            font.family: sidebar.theme.bodyFontFamily
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 64
                        radius: 14
                        color: sidebar.theme.tint(sidebar.theme.surface, sidebar.theme.darkMode ? 0.66 : 0.88)
                        border.width: 1
                        border.color: sidebar.theme.tint(sidebar.theme.borderSubtle, 0.84)

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("No active tools")
                            color: sidebar.theme.textSecondary
                            font.pixelSize: 13
                            font.family: sidebar.theme.bodyFontFamily
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
