pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"

/** @brief Expandable list item displaying a single box's metadata. */
Item {
    id: root

    required property AppTheme theme
    required property string label
    required property int boxId
    required property var center
    required property var size
    required property int vertexCount

    property bool expanded: false

    implicitWidth: parent ? parent.width : 260
    implicitHeight: contentColumn.implicitHeight + 16
    clip: true

    Behavior on implicitHeight {
        NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusSmall
        color: itemArea.containsMouse
            ? root.theme.surfaceStrong
            : root.theme.surfaceMuted
        border.width: root.expanded ? 1 : 0
        border.color: root.theme.tint(root.theme.borderSubtle, 0.5)
    }

    MouseArea {
        id: itemArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.expanded = !root.expanded
    }

    ColumnLayout {
        id: contentColumn

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppIcon {
                theme: root.theme
                iconKind: "cubeOutline"
                width: 16
                height: 16
            }

            Text {
                Layout.fillWidth: true
                text: root.label
                font.pixelSize: 13
                font.weight: Font.Medium
                color: root.theme.textPrimary
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            visible: root.expanded
            Layout.fillWidth: true
            Layout.leftMargin: 24
            spacing: 2

            Text {
                text: qsTr("Center: (%1, %2, %3)").arg(
                    root.center[0]?.toFixed(2) ?? "0").arg(
                    root.center[1]?.toFixed(2) ?? "0").arg(
                    root.center[2]?.toFixed(2) ?? "0")
                font.pixelSize: 11
                color: root.theme.textSecondary
            }

            Text {
                text: qsTr("Size: (%1, %2, %3)").arg(
                    root.size[0]?.toFixed(2) ?? "0").arg(
                    root.size[1]?.toFixed(2) ?? "0").arg(
                    root.size[2]?.toFixed(2) ?? "0")
                font.pixelSize: 11
                color: root.theme.textSecondary
            }

            Text {
                text: qsTr("Vertices: %1").arg(root.vertexCount)
                font.pixelSize: 11
                color: root.theme.textSecondary
            }
        }
    }
}
