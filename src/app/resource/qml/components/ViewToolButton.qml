pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "../theme"

/**
 * @brief Styled icon button for the view toolbar.
 *
 * Uses AbstractButton + AppIcon for theme-aware icon rendering with
 * ColorOverlay tinting.  40×40 px with hover deepening, scale, and tooltip.
 */
AbstractButton {
    id: button

    required property AppTheme theme
    property string iconKind: ""
    property string toolTipText: ""

    width: 40
    height: 40
    padding: 0

    scale: button.pressed ? 0.92 : (button.hovered ? 1.06 : 1.0)

    Behavior on scale {
        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
    }

    AppIcon {
        anchors.centerIn: parent
        theme: button.theme
        iconKind: button.iconKind
        useThemeContrast: true
        width: 26; height: 26
    }

    background: Rectangle {
        radius: 6
        color: button.pressed
            ? Qt.rgba(button.theme.accentA.r,
                      button.theme.accentA.g,
                      button.theme.accentA.b, 0.28)
            : (button.hovered
                ? Qt.rgba(button.theme.textPrimary.r,
                          button.theme.textPrimary.g,
                          button.theme.textPrimary.b, 0.15)
                : "transparent")

        border.width: 1
        border.color: button.hovered
            ? Qt.rgba(button.theme.borderDefault.r,
                      button.theme.borderDefault.g,
                      button.theme.borderDefault.b, 0.9)
            : "transparent"

        Behavior on color {
            ColorAnimation { duration: 120; easing.type: Easing.OutQuad }
        }
        Behavior on border.color {
            ColorAnimation { duration: 120; easing.type: Easing.OutQuad }
        }
    }

    Timer {
        id: tipDelay
        interval: 400
        onTriggered: tipItem.shown = true
    }

    onHoveredChanged: {
        if (hovered && toolTipText !== "") {
            tipDelay.restart()
        } else {
            tipDelay.stop()
            tipItem.shown = false
        }
    }

    ViewToolTip {
        id: tipItem
        theme: button.theme
        text: button.toolTipText
        anchors.horizontalCenter: button.horizontalCenter
        anchors.top: button.bottom
        anchors.topMargin: 4
    }
}
