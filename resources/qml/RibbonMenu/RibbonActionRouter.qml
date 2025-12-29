pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    signal exitApp

    // Injected by the app host (e.g., Main.qml) so actions can open dialogs.
    property var dialogHost: null

    // Injected by the app host; normal FileDialog instance for importing models.
    property var importModelDialog: null

    // Central mapping: actionId -> dialog component
    property RibbonDialogRegistry dialogRegistry: RibbonDialogRegistry {}

    function handle(actionId, payload): void {
        // Global / non-dialog actions
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
            else
                console.warn("[RibbonActionRouter] importModelDialog is null or missing open()");
            return;
        default:
            break;
        }

        // Dialog actions
        if (!dialogHost || !dialogHost.openComponent) {
            console.error("[RibbonActionRouter] dialogHost is null or missing openComponent()");
            return;
        }

        const comp = dialogRegistry.componentFor(actionId);
        if (!comp) {
            console.warn("[RibbonActionRouter] Unhandled action:", actionId, "payload:", payload);
            return;
        }

        dialogHost.openComponent(comp, {
            actionId: actionId,
            title: dialogRegistry.titleFor(actionId) || actionId,
            initialParams: payload || ({})
        });
    }
}
