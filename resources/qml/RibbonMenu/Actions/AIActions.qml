pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    property QtObject dialogHost: null

    function canHandle(actionId): bool {
        return actionId === "aiSuggest" || actionId === "aiChat";
    }

    function handle(actionId, payload): void {
        if (!dialogHost || !dialogHost.open) {
            console.error("[AIActions] dialogHost is null or missing open()");
            return;
        }

        const titles = {
            aiSuggest: qsTr("AI Suggest"),
            aiChat: qsTr("AI Chat")
        };

        dialogHost.open("qrc:/opengeolab/resources/qml/RibbonMenu/Dialogs/ActionParamsDialog.qml", {
            actionId: actionId,
            title: titles[actionId] || actionId,
            initialParams: payload || ({})
        });
    }
}
