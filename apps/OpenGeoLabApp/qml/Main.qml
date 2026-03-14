pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root

    required property var appController
    property bool darkMode: false
    property bool menuOpen: false
    property int selectedRibbonTab: 0
    property string statusNote: appController.lastSummary.length > 0 ? appController.lastSummary : "Viewport placeholder is active. Ribbon commands stay connected to the same controller pipeline."

    readonly property var ribbonTabs: ["Geometry", "Mesh", "AI"]
    readonly property var ribbonGroupsModel: [[
            {
                "title": "Create",
                "actions": [
                    {
                        "key": "addBox",
                        "title": "Box",
                        "icon": "box",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    },
                    {
                        "key": "addCylinder",
                        "title": "Cylinder",
                        "icon": "cylinder",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    },
                    {
                        "key": "addSphere",
                        "title": "Sphere",
                        "icon": "sphere",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    },
                    {
                        "key": "addTorus",
                        "title": "Torus",
                        "icon": "torus",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    }
                ]
            },
            {
                "title": "Modify",
                "actions": [
                    {
                        "key": "trim",
                        "title": "Trim",
                        "icon": "trim",
                        "accentOne": "accentD",
                        "accentTwo": "accentC"
                    },
                    {
                        "key": "offset",
                        "title": "Offset",
                        "icon": "offset",
                        "accentOne": "accentD",
                        "accentTwo": "accentD"
                    }
                ]
            },
            {
                "title": "Inspect",
                "actions": [
                    {
                        "key": "queryGeometry",
                        "title": "Query",
                        "icon": "query",
                        "accentOne": "accentE",
                        "accentTwo": "accentB"
                    }
                ]
            }
        ], [
            {
                "title": "Mesh",
                "actions": [
                    {
                        "key": "generateMesh",
                        "title": "Generate",
                        "icon": "mesh",
                        "accentOne": "accentB",
                        "accentTwo": "accentA"
                    },
                    {
                        "key": "smoothMesh",
                        "title": "Smooth",
                        "icon": "smoothMesh",
                        "accentOne": "accentB",
                        "accentTwo": "accentA"
                    }
                ]
            },
            {
                "title": "Inspect",
                "actions": [
                    {
                        "key": "queryMesh",
                        "title": "Query",
                        "icon": "query",
                        "accentOne": "accentC",
                        "accentTwo": "accentA"
                    }
                ]
            }
        ], [
            {
                "title": "Assist",
                "actions": [
                    {
                        "key": "aiSuggest",
                        "title": "Suggest",
                        "icon": "aiSuggest",
                        "accentOne": "accentE",
                        "accentTwo": "accentA"
                    },
                    {
                        "key": "aiChat",
                        "title": "Chat",
                        "icon": "aiChat",
                        "accentOne": "accentE",
                        "accentTwo": "accentA"
                    }
                ]
            }
        ]]

    width: 1500
    height: 900
    minimumWidth: 1280
    minimumHeight: 800
    visible: true
    title: "OpenGeoLab"
    color: appTheme.bg0

    AppTheme {
        id: appTheme

        darkMode: root.darkMode
    }

    function parsedResponse() {
        if (!root.appController.lastPayload || root.appController.lastPayload.length === 0) {
            return {};
        }

        try {
            return JSON.parse(root.appController.lastPayload);
        } catch (error) {
            return {};
        }
    }

    function viewportSummary() {
        const response = root.parsedResponse();
        const payload = response.payload || {};
        if (payload.selectionResult && payload.selectionResult.summary) {
            return payload.selectionResult.summary;
        }
        if (payload.renderFrame && payload.renderFrame.summary) {
            return payload.renderFrame.summary;
        }
        return root.statusNote;
    }

    function applyTheme(nextDarkMode) {
        root.darkMode = nextDarkMode;
        root.menuOpen = false;
    }

    function triggerHeaderAction(actionKey) {
        if (actionKey === "importModel") {
            root.statusNote = "Import model is a ribbon placeholder action.";
        } else if (actionKey === "exportModel") {
            root.statusNote = "Export model is a ribbon placeholder action.";
        } else if (actionKey === "toggleTheme") {
            root.applyTheme(!root.darkMode);
        } else if (actionKey === "recordSelection") {
            root.appController.recordSelectionSmokeTest();
            root.statusNote = root.appController.lastSummary;
        } else if (actionKey === "replayCommands") {
            root.appController.replayRecordedCommands();
            root.statusNote = root.appController.lastSummary;
        } else if (actionKey === "clearRecordedCommands") {
            root.appController.clearRecordedCommands();
            root.statusNote = root.appController.lastSummary;
        } else if (actionKey === "exportScript") {
            exportScriptDialog.open();
        } else if (actionKey === "focusViewport") {
            root.statusNote = root.viewportSummary();
        } else if (actionKey === "inspectPayload") {
            root.statusNote = root.appController.lastPayload;
        } else if (actionKey === "addBox") {
            root.statusNote = "Geometry create placeholder: Box.";
        } else if (actionKey === "addCylinder") {
            root.statusNote = "Geometry create placeholder: Cylinder.";
        } else if (actionKey === "addSphere") {
            root.statusNote = "Geometry create placeholder: Sphere.";
        } else if (actionKey === "addTorus") {
            root.statusNote = "Geometry create placeholder: Torus.";
        } else if (actionKey === "trim") {
            root.statusNote = "Geometry modify placeholder: Trim.";
        } else if (actionKey === "offset") {
            root.statusNote = "Geometry modify placeholder: Offset.";
        } else if (actionKey === "queryGeometry") {
            root.statusNote = "Geometry inspect placeholder: Query.";
        } else if (actionKey === "generateMesh") {
            root.statusNote = "Mesh placeholder: Generate.";
        } else if (actionKey === "smoothMesh") {
            root.statusNote = "Mesh placeholder: Smooth.";
        } else if (actionKey === "queryMesh") {
            root.statusNote = "Mesh inspect placeholder: Query.";
        } else if (actionKey === "aiSuggest") {
            root.statusNote = "AI assist placeholder: Suggest.";
        } else if (actionKey === "aiChat") {
            root.statusNote = "AI assist placeholder: Chat.";
        }

        root.menuOpen = false;
    }

    FileDialog {
        id: exportScriptDialog

        title: "Export Recorded Script"
        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        currentFile: "opengeolab_recorded.py"
        nameFilters: ["Python files (*.py)", "All files (*)"]

        onAccepted: {
            if (root.appController.exportRecordedScript(selectedFile)) {
                root.statusNote = root.appController.lastSummary;
            } else {
                root.statusNote = root.appController.lastPythonOutput;
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: appTheme.bg0
            }
            GradientStop {
                position: 0.55
                color: appTheme.bg1
            }
            GradientStop {
                position: 1.0
                color: appTheme.bg2
            }
        }
    }

    Rectangle {
        width: 360
        height: 360
        radius: 96
        anchors.right: parent.right
        anchors.rightMargin: -90
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -120
        color: appTheme.tint(appTheme.accentD, appTheme.darkMode ? 0.08 : 0.05)
        border.width: 1
        border.color: appTheme.tint(appTheme.accentD, 0.12)
        rotation: 14
    }

    Rectangle {
        anchors.fill: parent
        // anchors.margins: appTheme.shellMargin
        radius: 20
        color: "transparent"
        border.width: 0
        border.color: appTheme.shellBorder

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: appTheme.shellPadding
            spacing: appTheme.gap

            AppHeader {
                Layout.fillWidth: true
                Layout.preferredHeight: 136
                theme: appTheme
                darkMode: root.darkMode
                menuOpen: root.menuOpen
                selectedTab: root.selectedRibbonTab
                recordedCommandCount: root.appController.recordedCommandCount
                ribbonTabs: root.ribbonTabs
                ribbonGroups: root.ribbonGroupsModel[root.selectedRibbonTab]
                onToggleMenu: root.menuOpen = !root.menuOpen
                onSelectTab: function (tabIndex) {
                    root.selectedRibbonTab = tabIndex;
                }
                onTriggerAction: function (actionKey) {
                    root.triggerHeaderAction(actionKey);
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: appTheme.gap

                SidebarPanel {
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true
                    theme: appTheme
                }

                ViewportPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    theme: appTheme
                    summaryText: root.viewportSummary()
                    recordedCommandCount: root.appController.recordedCommandCount
                    onRequestViewPage: root.selectedRibbonTab = 2
                }
            }
        }
    }
}
