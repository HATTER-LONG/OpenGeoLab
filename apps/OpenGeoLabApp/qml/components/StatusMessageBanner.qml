pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Rectangle {
    id: banner

    required property AppTheme theme
    property string message: ""
    property string badgeText: ""
    property color accentColor: theme.accentD

    visible: message.length > 0
    radius: theme.radiusMedium
    color: theme.tint(accentColor, theme.darkMode ? 0.16 : 0.1)
    border.width: 1
    border.color: theme.tint(accentColor, theme.darkMode ? 0.54 : 0.3)
    implicitHeight: contentColumn.implicitHeight + 18

    Column {
        id: contentColumn

        anchors.fill: parent
        anchors.margins: 9
        spacing: 6

        Rectangle {
            visible: banner.badgeText.length > 0
            radius: 8
            color: banner.theme.tint(banner.accentColor, banner.theme.darkMode ? 0.28 : 0.16)
            border.width: 1
            border.color: banner.theme.tint(banner.accentColor, banner.theme.darkMode ? 0.66 : 0.38)
            width: badgeLabel.implicitWidth + 14
            height: 22

            Text {
                id: badgeLabel

                anchors.centerIn: parent
                text: banner.badgeText
                color: banner.theme.textPrimary
                font.pixelSize: 11
                font.bold: true
                font.family: banner.theme.bodyFontFamily
            }
        }

        Text {
            width: parent.width
            text: banner.message
            color: banner.theme.textPrimary
            font.pixelSize: 12
            font.bold: true
            font.family: banner.theme.bodyFontFamily
            wrapMode: Text.Wrap
        }
    }
}
