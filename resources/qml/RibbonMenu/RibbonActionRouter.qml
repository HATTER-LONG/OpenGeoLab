pragma ComponentBehavior: Bound
import QtQuick

import "Actions" as Actions

QtObject {
    id: root

    signal exitApp

    // Injected by the app host (e.g., Main.qml) so actions can open dialogs.
    property QtObject dialogHost: null

    // Action modules keep RibbonActionRouter small and maintainable.
    property list<QtObject> modules: [
        Actions.AppActions {
            router: root
        },
        Actions.GeometryActions {
            dialogHost: root.dialogHost
        },
        Actions.MeshActions {
            dialogHost: root.dialogHost
        },
        Actions.AIActions {
            dialogHost: root.dialogHost
        }
    ]

    function handle(actionId, payload): void {
        for (let i = 0; i < root.modules.length; i++) {
            const m = root.modules[i];
            if (!m || !m.canHandle || !m.handle)
                continue;

            if (m.canHandle(actionId)) {
                m.handle(actionId, payload);
                return;
            }
        }

        console.warn("[RibbonActionRouter] Unhandled action:", actionId, "payload:", payload);
    }
}
