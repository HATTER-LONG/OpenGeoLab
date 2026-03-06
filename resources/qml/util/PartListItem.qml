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

    readonly property bool geometryVisible: root.partData.geometry_visible !== false
    readonly property bool meshVisible: root.partData.mesh_visible !== false
    readonly property bool dimmed: !root.geometryVisible && !root.meshVisible

    signal toggleGeometryRequested(var partUid, bool visible)
    signal toggleMeshRequested(var partUid, bool visible)

    readonly property color activeToggleFill: Theme.isDark ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.2) : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14)
    readonly property color inactiveToggleFill: Theme.isDark ? Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.82) : Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.92)

    height: isExpanded ? expandedHeight : collapsedHeight
    radius: 8
    color: itemArea.containsMouse ? Qt.rgba(Theme.hovered.r, Theme.hovered.g, Theme.hovered.b, 0.85) : "transparent"
    border.width: root.dimmed ? 1 : 0
    border.color: root.dimmed ? Theme.border : "transparent"

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
        onClicked: function (mouse) {
            if (geometryArea.containsMouse || meshArea.containsMouse)
                return;
            root.isExpanded = !root.isExpanded;
        }
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
            opacity: root.dimmed ? 0.72 : 1.0

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

            RowLayout {
                spacing: 4

                Rectangle {
                    id: geometryToggle
                    implicitWidth: 24
                    implicitHeight: 24
                    radius: 7
                    border.width: 1
                    border.color: root.geometryVisible ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isDark ? 0.9 : 0.55) : Theme.border
                    color: geometryArea.pressed ? (root.geometryVisible ? Qt.darker(root.activeToggleFill, 1.08) : Theme.clicked) : (geometryArea.containsMouse ? (root.geometryVisible ? Qt.lighter(root.activeToggleFill, 1.08) : Theme.hovered) : (root.geometryVisible ? root.activeToggleFill : root.inactiveToggleFill))

                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "qrc:/opengeolab/resources/icons/face.svg"
                        size: 14
                        color: root.geometryVisible ? Theme.accent : Theme.textSecondary
                        opacity: root.geometryVisible ? 1.0 : 0.82
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 2
                        height: 14
                        radius: 1
                        rotation: 45
                        color: Theme.danger
                        opacity: root.geometryVisible ? 0.0 : 0.92
                    }

                    MouseArea {
                        id: geometryArea
                        anchors.fill: parent
                        hoverEnabled: true
                        preventStealing: true
                        onPressed: function (mouse) {
                            mouse.accepted = true;
                        }
                        onClicked: function (mouse) {
                            mouse.accepted = true;
                            root.toggleGeometryRequested(root.partData.uid, !root.geometryVisible);
                        }
                    }

                    ToolTip.visible: geometryArea.containsMouse
                    ToolTip.text: root.geometryVisible ? qsTr("Hide geometry") : qsTr("Show geometry")
                    ToolTip.delay: 400
                }

                Rectangle {
                    id: meshToggle
                    implicitWidth: 24
                    implicitHeight: 24
                    radius: 7
                    border.width: 1
                    border.color: root.meshVisible ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isDark ? 0.9 : 0.55) : Theme.border
                    color: meshArea.pressed ? (root.meshVisible ? Qt.darker(root.activeToggleFill, 1.08) : Theme.clicked) : (meshArea.containsMouse ? (root.meshVisible ? Qt.lighter(root.activeToggleFill, 1.08) : Theme.hovered) : (root.meshVisible ? root.activeToggleFill : root.inactiveToggleFill))

                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "qrc:/opengeolab/resources/icons/mesh.svg"
                        size: 14
                        color: root.meshVisible ? Theme.accent : Theme.textSecondary
                        opacity: root.meshVisible ? 1.0 : 0.82
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 2
                        height: 14
                        radius: 1
                        rotation: 45
                        color: Theme.danger
                        opacity: root.meshVisible ? 0.0 : 0.92
                    }

                    MouseArea {
                        id: meshArea
                        anchors.fill: parent
                        hoverEnabled: true
                        preventStealing: true
                        onPressed: function (mouse) {
                            mouse.accepted = true;
                        }
                        onClicked: function (mouse) {
                            mouse.accepted = true;
                            root.toggleMeshRequested(root.partData.uid, !root.meshVisible);
                        }
                    }

                    ToolTip.visible: meshArea.containsMouse
                    ToolTip.text: root.meshVisible ? qsTr("Hide mesh") : qsTr("Show mesh")
                    ToolTip.delay: 400
                }
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
                label: qsTr("Wires")
                count: root.partData.entity_counts ? root.partData.entity_counts.wires : 0
                iconSource: "qrc:/opengeolab/resources/icons/wire.svg"
                visible: (root.partData.entity_counts ? root.partData.entity_counts.wires : 0) > 0
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
                text: qsTr("ID: %1  |  Geo %2  |  Mesh %3").arg(root.partData.uid || "N/A").arg(root.geometryVisible ? qsTr("On") : qsTr("Off")).arg(root.meshVisible ? qsTr("On") : qsTr("Off"))
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
