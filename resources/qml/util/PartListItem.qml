/**
 * @file PartListItem.qml
 * @brief Individual part item component for the document sidebar
 *
 * Displays a single part with its color indicator, name, and entity counts.
 * Supports expanding to show detailed entity information.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: root

    /// Part data object from BackendService
    property var partData: ({})

    /// Index of this part in the list
    property int partIndex: 0

    /// Whether the item is expanded to show details
    property bool isExpanded: false

    height: isExpanded ? expandedHeight : collapsedHeight
    radius: 4
    color: itemArea.containsMouse ? Theme.hovered : "transparent"

    readonly property int collapsedHeight: 36
    readonly property int expandedHeight: collapsedHeight + detailsColumn.implicitHeight + 8

    Behavior on height {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    MouseArea {
        id: itemArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.isExpanded = !root.isExpanded
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Main row with color and name
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.collapsedHeight - 8
            spacing: 8

            // Color indicator
            Rectangle {
                width: 16
                height: 16
                radius: 3
                color: root.partData.color || "#808080"
                border.width: 1
                border.color: Theme.border
            }

            // Part name
            Label {
                text: root.partData.name || qsTr("Part %1").arg(root.partIndex + 1)
                font.pixelSize: 12
                color: Theme.textPrimary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            // Entity count badge
            Rectangle {
                width: countLabel.implicitWidth + 8
                height: 18
                radius: 9
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.border

                Label {
                    id: countLabel
                    anchors.centerIn: parent
                    text: root.partData.entity_counts ? root.partData.entity_counts.total : "0"
                    font.pixelSize: 10
                    color: Theme.textSecondary
                }
            }

            // Expand indicator
            ThemedIcon {
                source: root.isExpanded ? "qrc:/opengeolab/resources/icons/chevron_down.svg" : "qrc:/opengeolab/resources/icons/chevron_right.svg"
                size: 12
                color: Theme.textSecondary
            }
        }

        // Details section (visible when expanded)
        ColumnLayout {
            id: detailsColumn
            Layout.fillWidth: true
            Layout.leftMargin: 24
            spacing: 2
            visible: root.isExpanded
            opacity: root.isExpanded ? 1.0 : 0.0

            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            // Entity type counts
            EntityCountRow {
                label: qsTr("Faces")
                count: root.partData.entity_counts ? root.partData.entity_counts.faces : 0
                iconSource: "qrc:/opengeolab/resources/icons/face.svg"
            }

            EntityCountRow {
                label: qsTr("Edges")
                count: root.partData.entity_counts ? root.partData.entity_counts.edges : 0
                iconSource: "qrc:/opengeolab/resources/icons/edge.svg"
            }

            EntityCountRow {
                label: qsTr("Vertices")
                count: root.partData.entity_counts ? root.partData.entity_counts.vertices : 0
                iconSource: "qrc:/opengeolab/resources/icons/vertex.svg"
            }

            EntityCountRow {
                label: qsTr("Solids")
                count: root.partData.entity_counts ? root.partData.entity_counts.solids : 0
                iconSource: "qrc:/opengeolab/resources/icons/solid.svg"
                visible: (root.partData.entity_counts ? root.partData.entity_counts.solids : 0) > 0
            }

            EntityCountRow {
                label: qsTr("Shells")
                count: root.partData.entity_counts ? root.partData.entity_counts.shells : 0
                iconSource: "qrc:/opengeolab/resources/icons/shell.svg"
                visible: (root.partData.entity_counts ? root.partData.entity_counts.shells : 0) > 0
            }

            // Part ID info
            Label {
                text: qsTr("ID: %1").arg(root.partData.uid || "N/A")
                font.pixelSize: 10
                color: Theme.textSecondary
            }
        }
    }

    // Bottom separator
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        height: 1
        color: Theme.border
        opacity: 0.5
    }
}
