/**
 * @file DocumentSidebar.qml
 * @brief Left sidebar displaying document part information
 *
 * Shows a tree-like view of parts in the current document with
 * entity counts for vertices, edges, faces, and solids.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: sidebar

    /// Whether the sidebar is expanded
    property bool expanded: true

    /// Reference to DocumentService (injected from parent)
    required property var documentService

    /// Reference to RenderService for change notifications
    required property var renderService

    /// Collapsed width
    readonly property int collapsedWidth: 32

    /// Expanded width
    readonly property int expandedWidth: 240

    width: sidebar.expanded ? sidebar.expandedWidth : sidebar.collapsedWidth
    color: Theme.surfaceAlt

    Behavior on width {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    // Toggle button
    Rectangle {
        id: toggleButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        width: 24
        height: 24
        radius: 4
        color: toggleArea.containsMouse ? Theme.hovered : "transparent"

        ThemedIcon {
            anchors.centerIn: parent
            source: sidebar.expanded ? "qrc:/opengeolab/resources/icons/collapse.svg" : "qrc:/opengeolab/resources/icons/expand.svg"
            size: 16
        }

        MouseArea {
            id: toggleArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: sidebar.expanded = !sidebar.expanded
        }

        ToolTip.visible: toggleArea.containsMouse
        ToolTip.text: sidebar.expanded ? qsTr("Collapse sidebar") : qsTr("Expand sidebar")
        ToolTip.delay: 500
    }

    // Header
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: toggleButton.left
        height: 32
        color: "transparent"
        visible: sidebar.expanded

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Document")
            font.pixelSize: 13
            font.bold: true
            color: Theme.textPrimary
        }
    }

    // Separator
    Rectangle {
        id: separator
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
        visible: sidebar.expanded
    }

    // Part list
    ListView {
        id: partListView
        anchors.top: separator.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 4
        clip: true
        visible: sidebar.expanded
        spacing: 2

        model: sidebar.documentService.partListModel

        delegate: Rectangle {
            id: partDelegate
            width: partListView.width
            height: partColumn.implicitHeight + 12
            radius: 4
            color: partMouseArea.containsMouse ? Theme.hovered : "transparent"

            required property int index
            required property string name
            required property int vertexCount
            required property int edgeCount
            required property int wireCount
            required property int faceCount
            required property int shellCount
            required property int solidCount

            MouseArea {
                id: partMouseArea
                anchors.fill: parent
                hoverEnabled: true
            }

            ColumnLayout {
                id: partColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 8
                spacing: 4

                // Part name
                Label {
                    text: partDelegate.name
                    font.pixelSize: 12
                    font.bold: true
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Entity counts grid
                GridLayout {
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 2
                    Layout.fillWidth: true

                    // Solids
                    Label {
                        text: qsTr("Solids:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Label {
                        text: partDelegate.solidCount
                        font.pixelSize: 11
                        color: Theme.textPrimary
                    }

                    // Shells
                    Label {
                        text: qsTr("Shells:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Label {
                        text: partDelegate.shellCount
                        font.pixelSize: 11
                        color: Theme.textPrimary
                    }

                    // Faces
                    Label {
                        text: qsTr("Faces:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Label {
                        text: partDelegate.faceCount
                        font.pixelSize: 11
                        color: Theme.textPrimary
                    }

                    // Wires
                    Label {
                        text: qsTr("Wires:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Label {
                        text: partDelegate.wireCount
                        font.pixelSize: 11
                        color: Theme.textPrimary
                    }

                    // Edges
                    Label {
                        text: qsTr("Edges:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Label {
                        text: partDelegate.edgeCount
                        font.pixelSize: 11
                        color: Theme.textPrimary
                    }

                    // Vertices
                    Label {
                        text: qsTr("Vertices:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Label {
                        text: partDelegate.vertexCount
                        font.pixelSize: 11
                        color: Theme.textPrimary
                    }
                }
            }
        }

        // Empty state
        Label {
            anchors.centerIn: parent
            text: qsTr("No geometry loaded")
            font.pixelSize: 12
            color: Theme.textSecondary
            visible: partListView.count === 0
        }
    }

    // Collapsed icon strip
    Column {
        anchors.top: toggleButton.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 8
        visible: !sidebar.expanded

        ThemedIcon {
            source: "qrc:/opengeolab/resources/icons/box.svg"
            size: 20
            color: Theme.textSecondary

            ToolTip.visible: iconMouseArea.containsMouse
            ToolTip.text: qsTr("Parts: %1").arg(sidebar.documentService.partCount)
            ToolTip.delay: 300

            MouseArea {
                id: iconMouseArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }

    // Respond to document changes
    Connections {
        target: sidebar.renderService
        function onGeometryChanged() {
            sidebar.documentService.refresh();
        }
    }
}
