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
 * Contains the primary application layout including:
 * - Ribbon toolbar for menu and actions
 * - OpenGL viewport for 3D geometry rendering
 * - Floating function pages for geometry operations
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

        // OpenGL 3D viewport - displays triangle when no model loaded
        OpenGLViewport {
            id: glViewport
            anchors.fill: parent

            // Mouse cursor changes based on interaction
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hasModel ? Qt.OpenHandCursor : Qt.ArrowCursor
            }
        }

        // Viewport toolbar overlay
        Rectangle {
            id: viewportToolbar
            anchors {
                top: parent.top
                right: parent.right
                margins: 10
            }
            width: viewportToolbarLayout.width + 16
            height: viewportToolbarLayout.height + 8
            radius: 4
            color: Qt.rgba(0.1, 0.1, 0.1, 0.7)

            RowLayout {
                id: viewportToolbarLayout
                anchors.centerIn: parent
                spacing: 8

                Button {
                    text: qsTr("Reset")
                    onClicked: glViewport.resetCamera()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Reset camera to default view")
                }

                Button {
                    text: qsTr("Fit")
                    onClicked: glViewport.fitToView()
                    enabled: glViewport.hasModel
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Fit view to show all geometry")
                }
            }
        }

        // Status indicator when no model is loaded
        Rectangle {
            id: noModelIndicator
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                margins: 20
            }
            width: noModelText.width + 32
            height: noModelText.height + 16
            radius: 4
            color: Qt.rgba(0.1, 0.1, 0.1, 0.7)
            visible: !glViewport.hasModel

            Label {
                id: noModelText
                anchors.centerIn: parent
                text: qsTr("No model loaded - Default triangle displayed")
                color: "#aaaaaa"
                font.pixelSize: 14
            }
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
