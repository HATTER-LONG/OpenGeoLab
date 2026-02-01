pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @file Main.qml
 * @brief Main application window
 *
 * Contains the main layout with ribbon toolbar, document sidebar,
 * OpenGL viewport, and view controls.
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1920
    height: 1080
    title: qsTr("OpenGeoLab")

    palette: Theme.palette

    // Set main window reference for floating pages
    Component.onCompleted: {
        MainPages.mainWindow = root;
        initializeScene();
    }

    /**
     * @brief Initialize the 3D scene
     *
     * Refreshes the render service and creates default geometry
     * if no model is loaded.
     */
    function initializeScene() {
        RenderService.refreshScene();
    }
    header: RibbonMenu.RibbonToolBar {
        id: ribbonToolBar
        onActionTriggered: (actionId, payload) => {
            MainPages.handleAction(actionId, payload);
        }
    }

    // Main content area with sidebar and viewport
    Item {
        id: contentArea
        anchors.fill: parent

        // Document sidebar on the left
        DocumentSidebar {
            id: documentSidebar
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            z: 5
            documentService: DocumentService
            renderService: RenderService
        }

        // OpenGL 3D Viewport
        GLViewport {
            id: glViewport
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: documentSidebar.right
            anchors.right: parent.right
            renderService: RenderService

            // Viewport info overlay
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 8
                width: viewportInfoRow.implicitWidth + 16
                height: viewportInfoRow.implicitHeight + 8
                radius: 4
                color: Qt.rgba(0, 0, 0, 0.5)
                visible: true

                Row {
                    id: viewportInfoRow
                    anchors.centerIn: parent
                    spacing: 16

                    Label {
                        text: RenderService.hasGeometry ? qsTr("Geometry loaded") : qsTr("No geometry")
                        font.pixelSize: 11
                        color: "white"
                    }

                    Label {
                        text: qsTr("Ctrl+LMB: Rotate | MMB: Pan | RMB/Wheel: Zoom")
                        font.pixelSize: 11
                        color: Qt.rgba(1, 1, 1, 0.7)
                    }
                }
            }
        }

        // View toolbar for viewport controls
        ViewToolbar {
            id: viewToolbar
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 8
            z: 10

            onFitRequested: RenderService.fitToScene()
            onFrontViewRequested: RenderService.setFrontView()
            onTopViewRequested: RenderService.setTopView()
            onLeftViewRequested: RenderService.setLeftView()
            onRightViewRequested: RenderService.setRightView()
        }
    }

    // Handle geometry data changes from RenderService
    Connections {
        target: RenderService
        function onGeometryChanged() {
            // Auto-fit camera when geometry changes
            RenderService.fitToScene();
        }
    }

    Pages.CornerOverlay {
        id: cornerOverlay
        logService: LogService
    }

    // Container for floating function pages
    Item {
        id: functionPagesContainer
        anchors.fill: parent
        z: 100
    }
}
