pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import "../theme"

/**
 * A reusable context menu popup with a single "Copy" action.
 *
 * Usage:
 *   CopyContextMenu {
 *       id: contextMenu
 *       theme: root.theme
 *   }
 *   // To show:
 *   contextMenu.showAt(mouseX, mouseY, textToCopy)
 */
Popup {
    id: popup

    required property AppTheme theme
    property string copyText: ""

    width: copyRow.implicitWidth + 24
    height: copyRow.implicitHeight + 16
    padding: 0
    modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    function showAt(px: real, py: real, text: string): void {
        popup.copyText = text;
        popup.x = px;
        popup.y = py;
        popup.open();
    }

    TextEdit {
        id: clipboardHelper
        visible: false
    }

    background: Rectangle {
        radius: popup.theme.radiusMedium
        color: popup.theme.surfaceStrong
        border.width: 1
        border.color: popup.theme.tint(popup.theme.borderSubtle,
                                       popup.theme.darkMode ? 0.8 : 0.5)
    }

    contentItem: Rectangle {
        color: copyMouseArea.containsMouse
            ? popup.theme.tint(popup.theme.accentA,
                               popup.theme.darkMode ? 0.18 : 0.08)
            : popup.theme.surfaceStrong
        radius: popup.theme.radiusSmall

        Row {
            id: copyRow
            anchors.centerIn: parent
            spacing: 6

            AppIcon {
                theme: popup.theme
                iconKind: "copyOutline"
                useThemeContrast: false
                primaryColor: popup.theme.textPrimary
                width: 14
                height: 14
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: qsTr("Copy")
                color: popup.theme.textPrimary
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: copyMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                clipboardHelper.text = popup.copyText;
                clipboardHelper.selectAll();
                clipboardHelper.copy();
                popup.close();
            }
        }
    }
}
