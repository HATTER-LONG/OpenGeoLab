pragma ComponentBehavior: Bound
import QtQuick
import "../Pages" as Pages

/**
 * @brief Simplified action router for Ribbon toolbar actions.
 *
 * Routes action IDs to their corresponding dialogs or handlers.
 * Business logic is delegated to the individual dialog pages.
 */
QtObject {
    id: root

    signal exitApp

    // Dialog host for opening modal dialogs.
    property var dialogHost: null

    // File dialog for importing models.
    property var importModelDialog: null

    /**
     * @brief Routes an action to its handler or dialog.
     * @param actionId Unique action identifier.
     * @param payload Optional parameters for the action.
     */
    function handle(actionId, payload): void {
        // Handle global actions first.
        switch (actionId) {
        case "exitApp":
            root.exitApp();
            return;
        case "toggleTheme":
            Theme.mode = (Theme.mode === Theme.dark) ? Theme.light : Theme.dark;
            return;
        case "importModel":
            if (importModelDialog && importModelDialog.open)
                importModelDialog.open();
            return;
        }

        // Open dialog for the action.
        _openDialog(actionId, payload);
    }

    function _openDialog(actionId, payload): void {
        if (!dialogHost || !dialogHost.openDialog) {
            console.error("[ActionRouter] dialogHost not configured");
            return;
        }

        const comp = _getDialogComponent(actionId);
        if (!comp) {
            console.warn("[ActionRouter] No dialog for action:", actionId);
            return;
        }

        dialogHost.openDialog(comp, {
            initialParams: payload || ({})
        });
    }

    function _getDialogComponent(actionId) {
        switch (actionId) {
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

    // Dialog components.
    readonly property Component addPointDialog: Component {
        Pages.AddPointDialog {}
    }
    readonly property Component addLineDialog: Component {
        Pages.AddLineDialog {}
    }
    readonly property Component addBoxDialog: Component {
        Pages.AddBoxDialog {}
    }
    readonly property Component trimDialog: Component {
        Pages.TrimDialog {}
    }
    readonly property Component offsetDialog: Component {
        Pages.OffsetDialog {}
    }
    readonly property Component generateMeshDialog: Component {
        Pages.GenerateMeshDialog {}
    }
    readonly property Component smoothMeshDialog: Component {
        Pages.SmoothMeshDialog {}
    }
    readonly property Component aiSuggestDialog: Component {
        Pages.AISuggestDialog {}
    }
    readonly property Component aiChatDialog: Component {
        Pages.AIChatDialog {}
    }
}
