pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @file Main.qml
 * @brief Main application window
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
        BackendService.request("RenderService", JSON.stringify({
            action: "ViewPortControl",
            view_ctrl: {
                refresh: true
            },
            _meta: {
                silent: true
            }
        }));
    }
    header: RibbonMenu.RibbonToolBar {
        id: ribbonToolBar
        onActionTriggered: (actionId, payload) => {
            MainPages.handleAction(actionId, payload);
        }
    }

    // Main content area with OpenGL viewport
    Item {
        id: contentArea
        anchors.fill: parent

        // OpenGL 3D Viewport
        GLViewport {
            id: glViewport
            anchors.fill: parent

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
                        text: glViewport.hasGeometry ? qsTr("Geometry loaded") : qsTr("No geometry")
                        font.pixelSize: 11
                        color: "white"
                    }

                    Label {
                        text: qsTr("LMB: Rotate | MMB: Pan | RMB/Wheel: Zoom")
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
        }
    }

    // Handle geometry data changes from GLViewport (auto-fit)
    Connections {
        target: glViewport
        function onGeometryChanged() {
            // Auto-fit camera when geometry changes
            BackendService.request("RenderService", JSON.stringify({
                action: "ViewPortControl",
                view_ctrl: {
                    fit: true
                },
                _meta: {
                    silent: true
                }
            }));
        }
    }

    Pages.CornerOverlay {
        id: cornerOverlay
        logService: LogService // qmllint disable
    }

    // Container for floating function pages
    Item {
        id: functionPagesContainer
        anchors.fill: parent
        z: 100
    }
}
