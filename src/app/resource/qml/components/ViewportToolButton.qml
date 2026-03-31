pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import "../theme"

/**
 * @brief Icon button for the viewport toolbar.
 *
 * Displays an SVG icon in a 32×32 hit area with hover/press/toggled states.
 */
Rectangle {
    id: control

    required property AppTheme theme
    property string iconKind: ""
    property string tooltip: ""
    property bool toggled: false

    signal clicked

    implicitWidth: 32
    implicitHeight: 32
    radius: 6
    color: mouseArea.pressed
           ? Qt.rgba(control.theme.accentA.r, control.theme.accentA.g, control.theme.accentA.b, 0.25)
           : mouseArea.containsMouse
             ? Qt.rgba(control.theme.textPrimary.r, control.theme.textPrimary.g, control.theme.textPrimary.b, 0.12)
             : control.toggled
               ? Qt.rgba(control.theme.accentA.r, control.theme.accentA.g, control.theme.accentA.b, 0.18)
               : "transparent"
    border.width: control.toggled ? 1 : 0
    border.color: control.theme.accentA

    Behavior on color {
        ColorAnimation {
            duration: 100
        }
    }

    AppIcon {
        anchors.centerIn: parent
        width: 20
        height: 20
        theme: control.theme
        iconKind: control.iconKind
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: control.clicked()
    }

    ToolTip {
        visible: mouseArea.containsMouse && control.tooltip.length > 0
        text: control.tooltip
        delay: 600
    }
}
