pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @file Main.qml
 * @brief Main application window
 *
 * Provides the primary UI structure with:
 * - Ribbon toolbar for commands
 * - Model tree sidebar
 * - 3D viewport placeholder (pending render refactoring)
 * - Non-modal tool dialogs positioned beside the model tree
 *
 * @note The render module is temporarily disabled for restructuring.
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: qsTr("OpenGeoLab")

    // File dialog for importing models
    Pages.ImportModel {
        id: importModelDialog
    }

    // Result dialog for backend operation outcomes
    RibbonMenu.ResultDialog {
        id: resultDialog
    }

    /**
     * @brief Handle ribbon action routing
     * @param actionId Action identifier from ribbon
     * @param payload Optional action parameters
     */
    function handleAction(actionId: string, payload: var): void {
        // Handle global actions first
        switch (actionId) {
        case "exitApp":
            Qt.quit();
            return;
        case "toggleTheme":
            Theme.mode = (Theme.mode === Theme.dark) ? Theme.light : Theme.dark;
            return;
        case "importModel":
            if (importModelDialog && importModelDialog.open) {
                importModelDialog.open();
            }
            return;
        }

        // Open tool dialog for the action
        toolDialogHost.openDialog(actionId, payload || {});
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Ribbon Menu
        RibbonMenu.RibbonToolBar {
            id: ribbon
            Layout.fillWidth: true
            onActionTriggered: (actionId, payload) => root.handleAction(actionId, payload)
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

            // Right area - Viewport with tool dialogs overlay
            Item {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 400

                // Viewport placeholder
                OpenGLViewport {
                    id: glViewport
                    anchors.fill: parent
                }

                // Tool dialog host - positioned at top-left of viewport
                Pages.ToolDialogHost {
                    id: toolDialogHost
                    viewport: glViewport
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
