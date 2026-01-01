pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * Main application window.
 * Provides the primary UI structure with ribbon toolbar and content area.
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: qsTr("OpenGeoLab")

    // Dialog host for modal dialogs
    RibbonMenu.DialogHost {
        id: dialogHost
    }

    // File dialog for importing models
    Pages.ImportModel {
        id: importModelDialog
    }

    // Result dialog for backend operation outcomes
    RibbonMenu.ResultDialog {
        id: resultDialog
    }

    // Action router for ribbon toolbar
    RibbonMenu.ActionRouter {
        id: ribbonActions
        dialogHost: dialogHost
        importModelDialog: importModelDialog
        onExitApp: Qt.quit()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Ribbon Menu
        RibbonToolBar {
            id: ribbon
            Layout.fillWidth: true
            onActionTriggered: (actionId, payload) => ribbonActions.handle(actionId, payload)
        }

        // Main Content Area
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            // Left sidebar - Model tree
            ModelTreeSidebar {
                SplitView.minimumWidth: 200
                SplitView.preferredWidth: 250
                SplitView.maximumWidth: 400
            }

            // Right area - OpenGL viewport
            OpenGLViewport {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 400
            }

            // Handle styling
            handle: Rectangle {
                implicitWidth: 6
                color: SplitHandle.hovered ? Theme.accentColor : Theme.borderColor

                Behavior on color {
                    ColorAnimation {
                        duration: 150
                    }
                }
            }
        }
    }
}
