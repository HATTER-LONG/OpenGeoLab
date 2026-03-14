pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: chip

    required property AppTheme theme
    property string text: ""
    property color tintColor: theme.accentA

    implicitWidth: label.implicitWidth + 24
    implicitHeight: 28
    radius: 10
    color: theme.tint(tintColor, theme.darkMode ? 0.22 : 0.14)
    border.width: 1
    border.color: theme.tint(tintColor, theme.darkMode ? 0.4 : 0.24)

    Text {
        id: label

        anchors.centerIn: parent
        text: chip.text
        color: chip.theme.textPrimary
        font.pixelSize: 12
        font.bold: true
        font.family: chip.theme.bodyFontFamily
    }
}
