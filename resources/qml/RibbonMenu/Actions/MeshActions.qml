pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: root

    property QtObject dialogHost: null

    function canHandle(actionId): bool {
        return actionId === "generateMesh" || actionId === "smoothMesh";
    }

    function handle(actionId, payload): void {
        if (!dialogHost || !dialogHost.open) {
            console.error("[MeshActions] dialogHost is null or missing open()");
            return;
        }

        const titles = {
            generateMesh: qsTr("Generate Mesh"),
            smoothMesh: qsTr("Smooth Mesh")
        };

        dialogHost.open("qrc:/opengeolab/resources/qml/RibbonMenu/Dialogs/ActionParamsDialog.qml", {
            actionId: actionId,
            title: titles[actionId] || actionId,
            initialParams: payload || ({})
        });
    }
}
