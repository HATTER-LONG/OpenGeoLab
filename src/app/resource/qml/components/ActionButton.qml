pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: control

    required property AppTheme theme
    property string buttonText: ""
    property string iconKind: ""
    property bool leftAligned: false
    property color buttonColor: theme.surfaceMuted
    property color pressedColor: theme.surfaceStrong
    property color labelColor: theme.textPrimary
    property color iconPrimaryColor: theme.textPrimary
    property color iconSecondaryColor: theme.accentB
    property color hoverBorderColor: theme.tint(iconPrimaryColor, theme.darkMode ? 0.52 : 0.3)
    property int labelSize: 13
    property bool quiet: false
    signal clicked

    implicitWidth: 132
    implicitHeight: 40
    radius: theme.radiusSmall
    color: mouseArea.pressed ? pressedColor : (mouseArea.containsMouse ? theme.tint(buttonColor, quiet ? 0.92 : 1.0) : buttonColor)
    border.width: 1
    border.color: mouseArea.containsMouse ? hoverBorderColor : (quiet ? theme.tint(theme.borderSubtle, 0.45) : theme.borderSubtle)
    scale: mouseArea.pressed ? 0.98 : (mouseArea.containsMouse ? 1.01 : 1.0)

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: mouseArea.containsMouse ? control.theme.tint(control.iconSecondaryColor, control.theme.darkMode ? 0.08 : 0.06) : "transparent"
        border.width: 0
    }

    Behavior on color {
        ColorAnimation {
            duration: 140
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    Row {
        x: control.leftAligned ? 14 : (control.width - width) / 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: control.iconKind.length > 0 ? 10 : 0

        AppIcon {
            width: control.iconKind.length > 0 ? 18 : 0
            height: control.iconKind.length > 0 ? 18 : 0
            theme: control.theme
            iconKind: control.iconKind
            primaryColor: control.iconPrimaryColor
            visible: control.iconKind.length > 0
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: control.buttonText
            color: control.labelColor
            font.pixelSize: control.labelSize
            font.bold: true
            font.family: control.theme.bodyFontFamily
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: control.clicked()
    }
}
