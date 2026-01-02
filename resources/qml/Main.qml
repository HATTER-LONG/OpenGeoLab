pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @brief Main application window
 *
 * Provides the primary UI structure with:
 * - Ribbon toolbar for commands
 * - Model tree sidebar
 * - OpenGL viewport for 3D rendering
 * - Non-modal tool dialogs
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

        // Handle trim action specially for non-modal dialog
        onTrimRequested: {
            trimDialog.viewport = glViewport;
            trimDialog.show();
        }
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
            id: mainSplitView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            // Left sidebar - Model tree
            ModelTreeSidebar {
                SplitView.minimumWidth: 200
                SplitView.preferredWidth: 250
                SplitView.maximumWidth: 400
            }

            // Right area - OpenGL viewport with overlays
            Item {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 400

                // OpenGL viewport
                OpenGLViewport {
                    id: glViewport
                    anchors.fill: parent
                }

                // Non-modal Trim dialog (positioned over viewport)
                Pages.TrimDialog {
                    id: trimDialog
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    viewport: glViewport

                    onCloseRequested: {
                        hide();
                    }
                }
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
