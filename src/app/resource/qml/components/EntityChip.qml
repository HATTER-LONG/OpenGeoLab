import QtQuick
import QtQuick.Layouts
import "../theme"

/**
 * Removable chip displaying a selected entity's info.
 * Shows entity type abbreviation + local ID and shape ID.
 */
Rectangle {
    id: root

    required property AppTheme theme
    property int shapeId: 0
    property int entityType: 0
    property int localId: 0

    signal removeRequested(int shapeId, int entityType, int localId)

    implicitWidth: chipRow.implicitWidth + 16
    implicitHeight: 26
    radius: 13
    color: root.theme.tint(root.theme.accentE, root.theme.darkMode ? 0.18 : 0.10)
    border.width: 1
    border.color: root.theme.tint(root.theme.accentE, root.theme.darkMode ? 0.38 : 0.25)

    function entityTypeLabel(type) {
        switch (type) {
        case 0: return "V";
        case 1: return "E";
        case 2: return "W";
        case 3: return "F";
        case 4: return "S";
        case 5: return "P";
        case 10: return "N";
        case 11: return "L";
        case 12: return "Elem";
        default: return "?";
        }
    }

    RowLayout {
        id: chipRow

        anchors.centerIn: parent
        spacing: 4

        Text {
            text: qsTr("[%3]%1:%2")
                .arg(root.entityTypeLabel(root.entityType))
                .arg(root.localId)
                .arg(root.shapeId)
            color: root.theme.textPrimary
            font.pixelSize: 11
            font.family: root.theme.monoFontFamily
        }

        Rectangle {
            id: closeBtn

            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            radius: 8
            color: closeMouse.containsMouse
                ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.32 : 0.18)
                : "transparent"

            Text {
                anchors.centerIn: parent
                text: "✕"
                color: closeMouse.containsMouse ? root.theme.danger : root.theme.textSecondary
                font.pixelSize: 9
            }

            MouseArea {
                id: closeMouse

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.removeRequested(root.shapeId, root.entityType, root.localId)
            }
        }
    }
}
