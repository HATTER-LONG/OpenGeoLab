pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    property QtObject dialogHost: null

    function canHandle(actionId): bool {
        return actionId === "addPoint" || actionId === "addLine" || actionId === "addBox" || actionId === "trim" || actionId === "offset";
    }

    function handle(actionId, payload): void {
        if (!dialogHost || !dialogHost.open) {
            console.error("[GeometryActions] dialogHost is null or missing open()");
            return;
        }

        const titles = {
            addPoint: qsTr("Add Point"),
            addLine: qsTr("Add Line"),
            addBox: qsTr("Add Box"),
            trim: qsTr("Trim"),
            offset: qsTr("Offset")
        };

        dialogHost.open("qrc:/opengeolab/resources/qml/RibbonMenu/Dialogs/ActionParamsDialog.qml", {
            actionId: actionId,
            title: titles[actionId] || actionId,
            initialParams: payload || ({})
        });
    }
}
