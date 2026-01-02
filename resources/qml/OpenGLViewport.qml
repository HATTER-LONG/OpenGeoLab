pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0 as OGL

/**
 * @brief OpenGL viewport for 3D model rendering
 *
 * Provides:
 * - 3D geometry rendering using OpenGL
 * - Mouse interaction for view manipulation
 * - Selection support for geometry picking
 */
Item {
    id: root

    // Selection mode for geometry picking
    property alias selectionMode: viewport.selectionMode
    property alias selectedId: viewport.selectedId
    property alias hasGeometry: viewport.hasGeometry

    // Signals for selection events
    signal selectionChanged(int id, int type)

    // Background for the viewport
    Rectangle {
        anchors.fill: parent
        color: Theme.mode === Theme.dark ? "#2D3139" : "#F0F4F8"
        border.width: 1
        border.color: Theme.borderColor
    }

    // OpenGL viewport item
    OGL.ViewportItem {
        id: viewport
        anchors.fill: parent

        onSelectionChanged: (id, type) => root.selectionChanged(id, type)
    }

    // Placeholder when no geometry is loaded
    Column {
        anchors.centerIn: parent
        spacing: 16
        visible: !viewport.hasGeometry

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "OpenGL Viewport"
            font.pixelSize: 18
            font.bold: true
            color: Theme.textPrimaryColor
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("3D rendering area")
            font.pixelSize: 13
            color: Theme.textSecondaryColor
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 200
            height: 120
            color: "transparent"
            border.width: 2
            border.color: Theme.borderColor
            radius: 8

            Label {
                anchors.centerIn: parent
                text: "🎨"
                font.pixelSize: 48
            }
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Import a model or create geometry to begin")
            font.pixelSize: 11
            color: Theme.textSecondaryColor
            opacity: 0.7
        }
    }

    // Toolbar overlay
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        spacing: 4
        visible: viewport.hasGeometry

        ToolButton {
            icon.source: "qrc:/opengeolab/resources/icons/home.svg"
            icon.color: Theme.textPrimaryColor
            ToolTip.text: qsTr("Fit to view")
            ToolTip.visible: hovered
            onClicked: viewport.fitToView()
        }

        ToolButton {
            icon.source: "qrc:/opengeolab/resources/icons/refresh.svg"
            icon.color: Theme.textPrimaryColor
            ToolTip.text: qsTr("Reset view")
            ToolTip.visible: hovered
            onClicked: viewport.resetView()
        }
    }

    // Help text for controls
    Label {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 8
        text: qsTr("Ctrl+Drag: Rotate | Shift+Drag: Pan | Ctrl+Wheel: Zoom")
        font.pixelSize: 10
        color: Theme.textSecondaryColor
        opacity: 0.6
        visible: viewport.hasGeometry
    }

    // Public methods
    function fitToView() {
        viewport.fitToView();
    }

    function resetView() {
        viewport.resetView();
    }

    function clearSelection() {
        viewport.clearSelection();
    }
}
