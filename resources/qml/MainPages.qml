/**
 * @file MainPages.qml
 * @brief Singleton for lazy-loading and managing page components
 *
 * Caches page instances and routes actions to appropriate page handlers.
 * Supports both dialog-style pages (FileDialog) and floating function pages.
 */
pragma Singleton
import QtQuick

QtObject {
    id: mainPages

    /// Cache of created page component instances
    property var pageCache: ({})

    /// Reference to main window for floating pages (set by Main.qml)
    property var mainWindow: null

    /// Currently open floating page action ID (only one allowed at a time)
    property string currentOpenPage: ""

    /// Component path mapping for all available pages
    readonly property var componentMap: ({
            // File operations
            "importModel": {
                path: "Pages/ImportModel.qml",
                floating: false
            },

            // Geometry - Create
            "addBox": {
                path: "Pages/AddBoxPage.qml",
                floating: true
            },
            "addCylinder": {
                path: "Pages/AddCylinderPage.qml",
                floating: true
            },
            "addSphere": {
                path: "Pages/AddSpherePage.qml",
                floating: true
            },
            "addTorus": {
                path: "Pages/AddTorusPage.qml",
                floating: true
            },
            "query": {
                path: "Pages/GeoQueryPage.qml",
                floating: true
            },
            // Geometry - Modify
            "trim": {
                path: "Pages/TrimPage.qml",
                floating: true
            },
            "offset": {
                path: "Pages/OffsetPage.qml",
                floating: true
            },

            // Mesh
            "generateMesh": {
                path: "Pages/GenerateMeshPage.qml",
                floating: true
            },
            "smoothMesh": {
                path: "Pages/SmoothMeshPage.qml",
                floating: true
            },

            // AI
            "aiSuggest": {
                path: "Pages/AISuggestPage.qml",
                floating: true
            },
            "aiChat": {
                path: "Pages/AIChatPage.qml",
                floating: true
            }
        })

    /**
     * @brief Get or create a page component by action ID
     * @param actionId The action identifier mapped to a page
     * @return Page instance or undefined if not found
     */
    function getPage(actionId) {
        if (!pageCache[actionId]) {
            const config = componentMap[actionId];
            if (!config) {
                console.warn("[MainPages] Unknown action:", actionId);
                return undefined;
            }

            const component = Qt.createComponent(config.path);
            if (component.status === Component.Ready) {
                // Floating pages need a parent window for proper positioning
                const parent = config.floating && mainWindow ? mainWindow.contentItem : mainPages;
                pageCache[actionId] = component.createObject(parent);

                if (pageCache[actionId]) {
                    console.log("[MainPages] Created page for:", actionId);
                }
            } else if (component.status === Component.Error) {
                console.error("[MainPages] Failed to create component:", config.path, component.errorString());
            }
        }
        return pageCache[actionId];
    }

    /**
     * @brief Handle action triggered from ribbon or other sources
     * @param actionId The action identifier
     * @param payload Optional data payload
     */
    function handleAction(actionId, payload) {
        const config = componentMap[actionId];

        // For floating pages, close the current one first (only one allowed at a time)
        if (config && config.floating && currentOpenPage && currentOpenPage !== actionId) {
            const currentPage = pageCache[currentOpenPage];
            if (currentPage && typeof currentPage.close === "function") {
                currentPage.close();
            }
        }

        const page = getPage(actionId);
        if (page && typeof page.open === "function") {
            page.open(payload);
            // Track the currently open floating page
            if (config && config.floating) {
                currentOpenPage = actionId;
            }
        } else if (!page) {
            console.warn("[MainPages] No page handler for action:", actionId);
        }
    }

    /**
     * @brief Close all open floating pages
     */
    function closeAll() {
        for (const actionId in pageCache) {
            const page = pageCache[actionId];
            if (page && typeof page.close === "function") {
                page.close();
            }
        }
    }

    /**
     * @brief Check if a specific page is currently visible
     * @param actionId The action identifier
     * @return true if page is visible
     */
    function isPageOpen(actionId) {
        const page = pageCache[actionId];
        return page && page.visible;
    }
}
