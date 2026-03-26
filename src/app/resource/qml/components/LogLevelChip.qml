pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: chip

    required property AppTheme theme
    property string text: ""
    property color accentColor: chip.theme.accentA
    property bool selected: false
    signal clicked

    implicitWidth: label.implicitWidth + 22
    implicitHeight: 28
    radius: 9
    color: chip.selected ? chip.theme.tint(chip.accentColor, chip.theme.darkMode ? 0.24 : 0.14)
                         : chip.theme.tint(chip.theme.surface, chip.theme.darkMode ? 0.48 : 0.94)
    border.width: 1
    border.color: chip.selected ? chip.theme.tint(chip.accentColor, chip.theme.darkMode ? 0.54 : 0.28)
                                : chip.theme.tint(chip.theme.borderSubtle, chip.theme.darkMode ? 0.72 : 0.4)

    Text {
        id: label

        anchors.centerIn: chip
        text: chip.text
        color: chip.selected ? chip.theme.textPrimary : chip.theme.textSecondary
        font.pixelSize: 11
        font.family: "Segoe UI"
        font.bold: chip.selected
    }

    MouseArea {
        anchors.fill: chip
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: chip.clicked()
    }
}
