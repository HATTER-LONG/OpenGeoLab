pragma ComponentBehavior: Bound

import QtQml

/// @file MenuConfig.qml
/// @brief Central data source for menu panel actions, parallel to RibbonConfig.
/// Each section defines a group of actions with accent, alphaScale, and hoverAccent.
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
                  "accent": "accentE", "alphaScale": "muted", "hoverAccent": "accentE" }
            ]
        },
        {
            "title": qsTr("Script Recorder"),
            "accent": "accentB",
            "actions": [
                { "key": "recordSelection",      "title": qsTr("Start Script Record"),  "icon": "record"       },
                { "key": "replayCommands",        "title": qsTr("Replay Script"),        "icon": "replay"       },
                { "key": "exportScript",          "title": qsTr("Export Record"),         "icon": "exportRecord" },
                { "key": "clearRecordedCommands", "title": qsTr("Clear Script History"), "icon": "clear",
                  "hoverAccent": "accentD" }
            ]
        }
    ]
}
