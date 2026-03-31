pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import "../theme"

/**
 * @brief A reusable context menu popup with a single "Copy" action.
 *
 * Usage:
 * @code
 *   CopyContextMenu {
 *       id: contextMenu
 *       theme: root.theme
 *   }
 *   // To show:
 *   contextMenu.showAt(mouseX, mouseY, textToCopy)
 * @endcode
 */
Popup {
    id: popup

    required property AppTheme theme
    property string copyText: ""

    padding: 10
    leftPadding: 14
    rightPadding: 14
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
        radius: popup.theme.radiusSmall
        color: copyMouseArea.containsMouse
            ? (popup.theme.darkMode ? "#232c38" : "#d0dded")
            : popup.theme.surfaceStrong
        border.width: 1
        border.color: popup.theme.darkMode ? "#2e3a48" : "#c0cfe0"

        Behavior on color {
            ColorAnimation { duration: popup.theme.animFast }
        }
    }

    contentItem: Item {
        implicitWidth: copyRow.implicitWidth
        implicitHeight: copyRow.implicitHeight

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

        Row {
            id: copyRow
            anchors.centerIn: parent
            spacing: 8

            AppIcon {
                theme: popup.theme
                iconKind: "copyOutline"
                useThemeContrast: false
                primaryColor: popup.theme.textPrimary
                width: 16
                height: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: qsTr("Copy")
                color: popup.theme.textPrimary
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
