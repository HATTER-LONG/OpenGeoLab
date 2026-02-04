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

    // Expose key UI elements for floating pages positioning
    property alias documentSideBar: documentSideBar

    palette: Theme.palette

    // Set main window reference for floating pages
    Component.onCompleted: {
        MainPages.mainWindow = root;
        initializeScene();
        glViewport.focus = true;
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
        // Document sidebar on the left
        DocumentSideBar {
            id: documentSideBar
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            z: 10
        }
        // OpenGL 3D Viewport
        GLViewport {
            id: glViewport
            objectName: "glViewport"
            anchors.fill: parent
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
            // Refresh part list in sidebar
            documentSideBar.refreshPartList();
        }

        /**
         * @brief Handle entity pick event from viewport
         * @param entityType Type of picked entity
         * @param entityUid UID of picked entity
         */
        function onEntityPicked(entityType, entityUid) {
            console.log("[Main] Entity picked: type=" + entityType + ", uid=" + entityUid);
            // Forward to MainPages for handling by active page
            MainPages.handleEntityPicked(entityType, entityUid);
        }

        /**
         * @brief Handle pick cancelled event
         */
        function onPickCancelled() {
            console.log("[Main] Pick cancelled");
            MainPages.handlePickCancelled();
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
