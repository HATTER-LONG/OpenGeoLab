pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    signal exitApp

    function handle(actionId, payload): void {
        const fn = root._handlers[actionId];
        if (fn) {
            fn(payload);
            return;
        }
        console.warn("[RibbonActionRouter] Unhandled action:", actionId, "payload:", payload);
    }

    // Maps UI action ids to app-level intents.
    property var _handlers: ({
            "exitApp": function (_payload) {
                root.exitApp();
            },
            "toggleTheme": function (_payload) {
                Theme.mode = (Theme.mode === Theme.dark) ? Theme.light : Theme.dark;
            }
            // "importModel": function(payload) { root.importModelRequested(payload); }
        })
}
