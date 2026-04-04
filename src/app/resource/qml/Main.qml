pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import OpenGeoLab.Services 1.0
import "theme"
import "sections"

Window {
    id: root

    property bool darkMode: false
    property bool menuOpen: false
    property int selectedRibbonTab: 0
    property var pluginList: []
    property string statusNote: qsTr("Viewport is active. Ribbon commands stay connected to the same controller pipeline.")

    width: 1500
    height: 1000
    minimumWidth: 1280
    minimumHeight: 860
    visible: true
    title: "OpenGeoLab"
    color: appTheme.bg0

    AppTheme {
        id: appTheme

        darkMode: root.darkMode
    }

    RibbonConfig {
        id: ribbonConfig
    }

    Connections {
        target: RequestService

        function onResponseReady(responseJson, muted) {
            const resp = JSON.parse(responseJson);
            if (resp.module === "plugins" && resp.action === "list" && resp.ok) {
                root.pluginList = resp.result.plugins || [];
                root.statusNote = qsTr("Found %1 plugin(s).").arg(root.pluginList.length);
            }
        }

        function onErrorOccurred(errorMessage, muted) {
            root.statusNote = qsTr("Error: %1").arg(errorMessage);
            console.warn("[Main] Request error:", errorMessage);
        }
    }

    Component.onCompleted: {
        MainPages.mainWindow = root;
        MainPages.theme = appTheme;
        MainPages.pagesContainer = functionPagesContainer;
        RequestService.submitAsync(JSON.stringify({
            module: "plugins",
            action: "list",
            param: {},
            mute: true
        }));
    }

    function toggleTheme() {
        root.darkMode = !root.darkMode;
        root.statusNote = root.darkMode ? qsTr("Switched to dark theme.") : qsTr("Switched to light theme.");
        root.menuOpen = false;
    }

    function openActionPage(actionKey) {
        if (actionKey === "exit") {
            Qt.quit();
            return;
        }
        if (actionKey === "toggleTheme") {
            root.toggleTheme();
            return;
        }
        if (actionKey === "switchLanguage") {
            TranslationManager.switchLanguage(TranslationManager.currentLanguage === "zh_CN" ? "en_US" : "zh_CN");
            root.statusNote = TranslationManager.currentLanguage === "zh_CN" ? qsTr("Switched to Chinese.") : qsTr("Switched to English.");
            root.menuOpen = false;
            return;
        }

        if (actionKey === "newModel") {
            root.statusNote = qsTr("Resetting workspace…");
            root.menuOpen = false;
            MainPages.closeAll();
            RequestService.submitAsync(JSON.stringify({
                module: "scene",
                action: "new_model",
                param: {}
            }));
            return;
        }

        // AI Chat — launch via plugin system.
        if (actionKey === "aiChat") {
            root.statusNote = qsTr("Launching AI Chat…");
            root.menuOpen = false;
            RequestService.executeOnMainThread(JSON.stringify({
                module: "plugins",
                action: "invoke_ui",
                param: {
                    pluginName: "ai_chat_plugin"
                },
                mute: true
            }));
            return;
        }

        // PySide6 UI plugins — must execute on main thread.
        if (actionKey.startsWith("pluginUI_")) {
            const pluginName = actionKey.substring(9);
            root.statusNote = qsTr("Launching plugin UI: %1").arg(pluginName);
            root.menuOpen = false;
            RequestService.executeOnMainThread(JSON.stringify({
                module: "plugins",
                action: "invoke_ui",
                param: {
                    pluginName: pluginName
                },
                mute: true
            }));
            return;
        }

        // Script-only plugins — execute asynchronously.
        if (actionKey.startsWith("plugin_")) {
            const pluginName = actionKey.substring(7);
            root.statusNote = qsTr("Executing plugin: %1").arg(pluginName);
            root.menuOpen = false;
            RequestService.submitAsync(JSON.stringify({
                module: "plugins",
                action: "execute",
                param: {
                    pluginName: pluginName
                },
                mute: false
            }));
            return;
        }

        if (MainPages.hasPage(actionKey)) {
            MainPages.handleAction(actionKey);
            root.menuOpen = false;
            return;
        }

        root.statusNote = qsTr("Action: %1 (not yet implemented)").arg(actionKey);
        root.menuOpen = false;
        console.log("[Main] Action not implemented:", actionKey);
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
                ribbonTabs: ribbonConfig.tabs
                ribbonGroups: ribbonConfig.groupsForTab(root.selectedRibbonTab)
                pluginList: root.pluginList
                actionHandler: root.openActionPage
                onToggleMenu: root.menuOpen = !root.menuOpen
                onSelectTab: function (tabIndex) {
                    root.selectedRibbonTab = tabIndex;
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                MouseArea {
                    anchors.fill: parent
                    visible: root.menuOpen
                    z: 10
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.ArrowCursor
                    onClicked: root.menuOpen = false
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: appTheme.gap

                    ColumnLayout {
                        Layout.preferredWidth: 280
                        Layout.maximumWidth: 280
                        Layout.fillHeight: true
                        spacing: appTheme.gap

                        SidebarPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: appTheme
                        }

                        ProgressCard {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            theme: appTheme
                        }
                    }

                    ViewportPanel {
                        id: viewportPanel
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        theme: appTheme
                    }
                }

                ActivityOverlay {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: appTheme.gap
                    anchors.bottomMargin: appTheme.gap
                    theme: appTheme
                }

                Item {
                    id: functionPagesContainer
                    anchors.fill: parent
                    z: 100
                    visible: MainPages.currentOpenPage.length > 0
                }
            }
        }
    }
}
