pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Rectangle {
    id: chip

    required property AppTheme theme
    property string label: ""
    property color accentColor: theme.accentA
    property bool selected: false
    signal clicked

    width: 74
    height: 30
    radius: 10
    color: selected ? theme.tint(accentColor, theme.darkMode ? 0.38 : 0.18)
                    : theme.tint(accentColor, theme.darkMode ? 0.12 : 0.08)
    border.width: 1
    border.color: selected ? theme.tint(accentColor, theme.darkMode ? 0.92 : 0.58)
                           : theme.tint(accentColor, theme.darkMode ? 0.42 : 0.24)

    Row {
        anchors.centerIn: parent
        spacing: 6

        Rectangle {
            width: 8
            height: 8
            radius: 4
            anchors.verticalCenter: parent.verticalCenter
            color: chip.accentColor
            border.width: 1
            border.color: chip.theme.tint(chip.accentColor, chip.theme.darkMode ? 0.9 : 0.52)
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: chip.label
            color: chip.theme.textPrimary
            font.pixelSize: 11
            font.bold: chip.selected
            font.family: chip.theme.bodyFontFamily
        }
    }

    Rectangle {
        visible: chip.selected
        anchors.fill: parent
        anchors.margins: 1
        radius: 9
        color: "transparent"
        border.width: 1
        border.color: chip.theme.tint(chip.accentColor, chip.theme.darkMode ? 0.76 : 0.34)
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: chip.clicked()
    }
}
