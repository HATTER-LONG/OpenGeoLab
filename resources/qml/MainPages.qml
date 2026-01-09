/**
 * @file MainPages.qml
 * @brief Singleton for lazy-loading and managing page components
 *
 * Caches page instances and routes actions to appropriate page handlers.
 */
pragma Singleton
import QtQuick

QtObject {
    id: mainPages

    /// Cache of created page component instances
    property var pageCache: ({})

    /**
     * @brief Get or create a page component by action ID
     * @param actionId The action identifier mapped to a page
     * @return Page instance or undefined if not found
     */
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
