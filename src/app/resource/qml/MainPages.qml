pragma Singleton

import QtQuick

QtObject {
    id: mainPages

    property var theme: null
    property var mainWindow: null
    property var pagesContainer: null
    property string currentOpenPage: ""
    property var pageCache: ({})

    readonly property var componentMap: ({
        "addBox":      { path: "components/pages/CreateBoxPage.qml" },
        "addCylinder": { path: "components/pages/CreateCylinderPage.qml" },
        "addSphere":   { path: "components/pages/CreateSpherePage.qml" },
        "addTorus":    { path: "components/pages/CreateTorusPage.qml" },
        "importModel": { path: "components/pages/ImportModelPage.qml" }
    })

    function hasPage(actionId) {
        return actionId in componentMap;
    }

    function getPage(actionId) {
        if (!pageCache[actionId]) {
            const config = componentMap[actionId];
            if (!config) {
                console.warn("[MainPages] Unknown action:", actionId);
                return undefined;
            }
            const component = Qt.createComponent(config.path);
            if (component.status === Component.Ready) {
                const parent = pagesContainer ? pagesContainer : mainPages;
                pageCache[actionId] = component.createObject(parent);
                if (pageCache[actionId]) {
                    console.log("[MainPages] Created page for:", actionId);
                }
            } else if (component.status === Component.Error) {
                console.error("[MainPages] Failed:", config.path, component.errorString());
            }
        }
        return pageCache[actionId];
    }

    function handleAction(actionId, payload) {
        const config = componentMap[actionId];
        if (config && currentOpenPage && currentOpenPage !== actionId) {
            const currentPage = pageCache[currentOpenPage];
            if (currentPage && typeof currentPage.close === "function") {
                currentPage.close();
            }
        }
        const page = getPage(actionId);
        if (page && typeof page.open === "function") {
            page.open(payload);
            currentOpenPage = actionId;
        } else if (!page) {
            console.warn("[MainPages] No page handler for:", actionId);
        }
    }

    function closeAll() {
        for (const actionId in pageCache) {
            const page = pageCache[actionId];
            if (page && typeof page.close === "function") {
                page.close();
            }
        }
        currentOpenPage = "";
    }
}
