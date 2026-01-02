pragma ComponentBehavior: Bound
import QtQuick
import "." as Pages

/**
 * @file ToolDialogHost.qml
 * @brief Host component for managing non-modal tool dialogs
 *
 * Manages the display of tool dialogs with mutual exclusivity:
 * - Only one dialog can be open at a time
 * - Opening a new dialog closes the previous one
 * - Positioned to the right of the model tree sidebar
 *
 * @note This component should be placed in the main content area,
 *       typically as a sibling to the viewport.
 */
Item {
    id: root

    /**
     * @brief Reference to the viewport for selection integration
     */
    property var viewport: null

    /**
     * @brief Currently active dialog type (empty string if none)
     */
    property string activeDialogType: ""

    /**
     * @brief Emitted when a dialog is opened
     */
    signal dialogOpened(string dialogType)

    /**
     * @brief Emitted when a dialog is closed
     */
    signal dialogClosed(string dialogType)

    anchors.left: parent.left
    anchors.top: parent.top
    anchors.margins: 16
    width: 360
    height: parent.height - 32

    // =========================================================================
    // Dialog Instances
    // =========================================================================

    Pages.AddPointToolDialog {
        id: addPointDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("addPoint")
    }

    Pages.AddLineToolDialog {
        id: addLineDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("addLine")
    }

    Pages.AddBoxToolDialog {
        id: addBoxDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("AddBox")
    }

    Pages.TrimToolDialog {
        id: trimDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("trim")
    }

    Pages.OffsetToolDialog {
        id: offsetDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("offset")
    }

    Pages.GenerateMeshToolDialog {
        id: generateMeshDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("generateMesh")
    }

    Pages.SmoothMeshToolDialog {
        id: smoothMeshDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("smoothMesh")
    }

    Pages.AISuggestToolDialog {
        id: aiSuggestDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("aiSuggest")
    }

    Pages.AIChatToolDialog {
        id: aiChatDialog
        anchors.top: parent.top
        viewport: root.viewport
        onCloseRequested: root.closeDialog("aiChat")
    }

    /**
     * @brief Open a specific dialog by type
     * @param dialogType Type identifier for the dialog
     * @param params Optional initial parameters
     */
    function openDialog(dialogType: string, params: var): void {
        // Close any currently open dialog
        closeAllDialogs();

        // Open the requested dialog
        var dialog = getDialogByType(dialogType);
        if (dialog) {
            if (params) {
                dialog.initialParams = params;
            }
            dialog.show();
            root.activeDialogType = dialogType;
            root.dialogOpened(dialogType);
        } else {
            console.warn("[ToolDialogHost] Unknown dialog type:", dialogType);
        }
    }

    /**
     * @brief Close a specific dialog
     * @param dialogType Type identifier for the dialog
     */
    function closeDialog(dialogType: string): void {
        var dialog = getDialogByType(dialogType);
        if (dialog) {
            dialog.hide();
            if (root.activeDialogType === dialogType) {
                root.activeDialogType = "";
            }
            root.dialogClosed(dialogType);
        }
    }

    /**
     * @brief Close all open dialogs
     */
    function closeAllDialogs(): void {
        addPointDialog.hide();
        addLineDialog.hide();
        addBoxDialog.hide();
        trimDialog.hide();
        offsetDialog.hide();
        generateMeshDialog.hide();
        smoothMeshDialog.hide();
        aiSuggestDialog.hide();
        aiChatDialog.hide();
        root.activeDialogType = "";
    }

    /**
     * @brief Check if any dialog is currently open
     * @return True if a dialog is open
     */
    function hasOpenDialog(): bool {
        return root.activeDialogType !== "";
    }

    /**
     * @brief Get dialog instance by type
     * @param dialogType Type identifier
     * @return Dialog instance or null
     */
    function getDialogByType(dialogType: string): var {
        switch (dialogType) {
        case "addPoint":
            return addPointDialog;
        case "addLine":
            return addLineDialog;
        case "AddBox":
            return addBoxDialog;
        case "trim":
            return trimDialog;
        case "offset":
            return offsetDialog;
        case "generateMesh":
            return generateMeshDialog;
        case "smoothMesh":
            return smoothMeshDialog;
        case "aiSuggest":
            return aiSuggestDialog;
        case "aiChat":
            return aiChatDialog;
        default:
            return null;
        }
    }
}
