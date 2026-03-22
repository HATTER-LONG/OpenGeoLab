pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Rectangle {
    id: control

    required property AppTheme theme
    required property var actionHandler
    property string actionKey: ""
    property string buttonText: ""
    property string iconKind: ""
    property bool leftAligned: false
    property var colorSet: ({})
    property color hoverBorderOverride: "transparent"
    property color labelColor: theme.textPrimary
    property color iconPrimaryColor: theme.textPrimary
    property int labelSize: 13
    property bool quiet: false

    implicitWidth: 132
    implicitHeight: 40
    radius: theme.radiusSmall
    color: mouseArea.pressed ? colorSet.pressed : (mouseArea.containsMouse ? theme.tint(colorSet.normal, quiet ? 0.92 : 1.0) : colorSet.normal)
    border.width: 1
    border.color: mouseArea.containsMouse ? (hoverBorderOverride.a > 0 ? hoverBorderOverride : theme.tint(theme.textPrimary, theme.darkMode ? 0.52 : 0.3)) : (quiet ? theme.tint(theme.borderSubtle, 0.45) : theme.borderSubtle)
    scale: mouseArea.pressed ? 0.98 : (mouseArea.containsMouse ? 1.01 : 1.0)

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
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: control.actionHandler(control.actionKey)
    }
}
