pragma Singleton
import QtQuick

QtObject {
    id: mainPages

    property var pageCache: ({})
    function getPage(actionId) {
        if (!pageCache[actionId]) {
            const componentMap = {
                "importModel": "Pages/ImportModel.qml"
            };

            const path = componentMap[actionId];
            if (path) {
                const component = Qt.createComponent(path);
                if (component.status === Component.Ready) {
                    pageCache[actionId] = component.createObject(mainPages);
                }
            }
        }
        return pageCache[actionId];
    }

    function handleAction(actionId, payload) {
        const page = getPage(actionId);
        if (page && typeof page.open === "function") {
            page.open(payload);
        }
    }
}
