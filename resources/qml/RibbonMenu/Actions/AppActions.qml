pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    // router is the top-level RibbonActionRouter instance
    property QtObject router: null

    function canHandle(actionId): bool {
        return actionId === "exitApp" || actionId === "toggleTheme" || actionId === "importModel";
    }

    function handle(actionId, _payload): void {
        switch (actionId) {
        case "exitApp":
            if (router && router.exitApp)
                router.exitApp();
            else
                console.warn("[AppActions] router.exitApp not available");
            break;
        case "toggleTheme":
            Theme.mode = (Theme.mode === Theme.dark) ? Theme.light : Theme.dark;
            break;
        case "importModel":
            ImportModel.open();
            break;
        default:
            console.warn("[AppActions] Unhandled action:", actionId);
            break;
        }
    }
}
