pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Rectangle {
    id: tabs

    required property AppTheme theme
    property var items: []
    property int currentIndex: 0
    property color accentColor: theme.accentA
    signal tabSelected(int index)

    radius: theme.radiusMedium
    color: theme.surfaceMuted
    border.width: 1
    border.color: theme.borderSubtle
    implicitHeight: tabRow.implicitHeight + 12

    RowLayout {
        id: tabRow

        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        Repeater {
            model: tabs.items

            delegate: Rectangle {
                required property var modelData

                readonly property bool selected: tabs.currentIndex === index

                Layout.fillWidth: true
                Layout.preferredHeight: 54
                radius: tabs.theme.radiusSmall
                color: selected
                    ? tabs.theme.tint(tabs.accentColor, tabs.theme.darkMode ? 0.24 : 0.12)
                    : tabs.theme.tint(tabs.theme.surface, tabs.theme.darkMode ? 0.58 : 0.94)
                border.width: 1
                border.color: selected
                    ? tabs.theme.tint(tabs.accentColor, tabs.theme.darkMode ? 0.6 : 0.34)
                    : tabs.theme.tint(tabs.theme.borderSubtle, tabs.theme.darkMode ? 0.58 : 0.3)

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: parent.parent.modelData.title
                        color: tabs.theme.textPrimary
                        font.pixelSize: 12
                        font.bold: true
                        font.family: tabs.theme.bodyFontFamily
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: parent.parent.modelData.subtitle || ""
                        color: tabs.theme.textSecondary
                        font.pixelSize: 10
                        font.family: tabs.theme.bodyFontFamily
                        visible: text.length > 0
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: tabs.tabSelected(index)
                }
            }
        }
    }
}
