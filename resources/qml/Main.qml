pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @file Main.qml
 * @brief Main application window with 3D viewport
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1920
    height: 1080
    title: qsTr("OpenGeoLab")

    palette: Theme.palette

    header: RibbonMenu.RibbonToolBar {
        id: ribbonToolBar
        onActionTriggered: (actionId, payload) => {
            MainPages.handleAction(actionId, payload);
        }
    }

    // Main content area with viewport
    Rectangle {
        anchors.fill: parent
        color: Theme.surface

        // 3D Viewport
        ViewportItem {
            id: viewport
            anchors.fill: parent
            showFaces: true
            showEdges: true
            backgroundColor: Theme.isDark ? "#1a1a24" : "#e0e0e8"

            // Fit all when geometry is loaded
            Connections {
                target: BackendService
                function onOperationFinished(moduleName, result) {
                    if (moduleName === "ModelReader") {
                        viewport.fitAll();
                    }
                }
            }
        }

        // Viewport controls overlay
        RowLayout {
            anchors {
                top: parent.top
                right: parent.right
                margins: 8
            }
            spacing: 4

            Button {
                text: qsTr("Fit All")
                onClicked: viewport.fitAll()
            }

            Button {
                text: qsTr("Reset View")
                onClicked: viewport.resetView()
            }

            CheckBox {
                text: qsTr("Faces")
                checked: viewport.showFaces
                onCheckedChanged: viewport.showFaces = checked
            }

            CheckBox {
                text: qsTr("Edges")
                checked: viewport.showEdges
                onCheckedChanged: viewport.showEdges = checked
            }
        }
    }

    Pages.CornerOverlay {
        id: cornerOverlay
        logService: LogService
    }
}
