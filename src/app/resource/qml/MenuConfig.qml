pragma ComponentBehavior: Bound

import QtQml

/**
 * @file MenuConfig.qml
 * @brief Central data source for menu panel actions, parallel to RibbonConfig.
 *
 * Each section defines a group of actions with accent, alphaScale, and hoverAccent.
 */
QtObject {
    readonly property var sections: [
        {
            "title": qsTr("Workspace"),
            "accent": "accentA",
            "actions": [
                { "key": "importModel",    "title": qsTr("Import Model"),  "icon": "import"  },
                { "key": "exportModel",    "title": qsTr("Export Model"),  "icon": "export"  },
                { "key": "toggleTheme",    "title": "",  "icon": "",  "dynamic": true,
                  "alphaScale": "muted", "hoverAccent": "accentA" },
                { "key": "switchLanguage", "title": "",  "icon": "language", "dynamic": true,
                  "accent": "accentE", "alphaScale": "muted", "hoverAccent": "accentE" },
                { "key": "exit", "title": qsTr("Exit"), "icon": "exitOutline",
                  "accent": "accentD", "alphaScale": "muted", "hoverAccent": "accentD" }
            ]
        }
    ]
}
