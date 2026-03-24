pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import OpenGeoLab.Services
import "theme"
import "sections"

Window {
    id: root

    property bool darkMode: false
    property bool menuOpen: false
    property int selectedRibbonTab: 0
    property var pluginList: []
    property bool pluginListLoaded: false
    property string statusNote: qsTr("Viewport is active. Ribbon commands stay connected to the same controller pipeline.")

    ListModel {
        id: boxListModel
    }

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

        function onResponseReady(requestId, responseJson) {
            try {
                const resp = JSON.parse(responseJson);
                if (resp.module === "plugins" && resp.action === "list" && resp.ok) {
                    root.pluginList = resp.result.plugins || [];
                    root.pluginListLoaded = true;
                    root.statusNote = qsTr("Found %1 plugin(s).").arg(root.pluginList.length);
                } else if (resp.module === "geometry" && resp.action === "list_boxes" && resp.ok) {
                    boxListModel.clear();
                    const boxes = resp.result.boxes || [];
                    for (let i = 0; i < boxes.length; ++i) {
                        const b = boxes[i];
                        boxListModel.append({
                            boxId: b.id ?? 0,
                            label: b.label ?? qsTr("Box"),
                            center: Array.isArray(b.center) && b.center.length === 3 ? b.center : [0, 0, 0],
                            size: Array.isArray(b.size) && b.size.length === 3 ? b.size : [0, 0, 0],
                            vertexCount: b.vertexCount ?? 0
                        });
                    }
                } else if (resp.module === "geometry" && resp.ok) {
                    root.statusNote = qsTr("Geometry: %1").arg(resp.summary ?? "done");
                }
            } catch (e) {
                console.warn("[Main] Failed to parse response:", e);
            }
        }

        function onErrorOccurred(requestId, errorMessage) {
            root.statusNote = qsTr("Error: %1").arg(errorMessage);
        }
    }

    Connections {
        target: NotificationService

        function onNotificationReceived(channel, payload) {
            try {
                if (channel === "geometry.data_changed") {
                    RequestService.submitAsync(JSON.stringify({
                        module: "geometry",
                        action: "list_boxes",
                        param: {},
                        mute: true
                    }));
                } else if (channel === "geometry.status" || channel === "geometry.progress") {
                    const data = JSON.parse(payload);
                    if (data.event === "started") {
                        root.statusNote = qsTr("Creating box…");
                    } else if (data.event === "progress") {
                        root.statusNote = data.message ?? qsTr("Processing…");
                    } else if (data.event === "completed") {
                        root.statusNote = qsTr("Completed: %1").arg(data.label ?? "");
                    }
                }
            } catch (e) {
                console.warn("[Main] Failed to parse notification:", e);
            }
        }
    }

    Component.onCompleted: {
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
        if (actionKey === "toggleTheme") {
            root.toggleTheme();
            return;
        }
        if (actionKey === "switchLanguage") {
            TranslationManager.switchLanguage(
                TranslationManager.currentLanguage === "zh_CN" ? "en_US" : "zh_CN");
            root.statusNote = TranslationManager.currentLanguage === "zh_CN"
                ? qsTr("Switched to Chinese.") : qsTr("Switched to English.");
            root.menuOpen = false;
            return;
        }

        // Geometry actions dispatched to C++ geometry module via Python runtime
        if (actionKey === "addBox") {
            RequestService.submitAsync(JSON.stringify({
                module: "geometry",
                action: "create_box",
                param: { vertexCount: 100, center: [0, 0, 0], size: [1, 1, 1] }
            }));
            root.statusNote = qsTr("Creating box…");
            return;
        }

        if (actionKey.startsWith("pluginUI_")) {
            const pluginName = actionKey.substring(9);
            RequestService.executeOnMainThread(JSON.stringify({
                module: "plugins",
                action: "invoke_ui",
                param: { pluginName: pluginName },
                mute: true
            }));
            root.statusNote = qsTr("Launching plugin UI: %1").arg(pluginName);
            return;
        }

        if (actionKey.startsWith("plugin_")) {
            const pluginName = actionKey.substring(7);
            RequestService.submitAsync(JSON.stringify({
                module: "plugins",
                action: "execute",
                param: { pluginName: pluginName },
                mute: true
            }));
            root.statusNote = qsTr("Executing plugin: %1").arg(pluginName);
            return;
        }

        root.statusNote = qsTr("Action: %1").arg(actionKey);
        root.menuOpen = false;
        console.log("[Main] openActionPage:", actionKey);
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
                recordedCommandCount: 0
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

                    SidebarPanel {
                        Layout.preferredWidth: 280
                        Layout.fillHeight: true
                        theme: appTheme
                        boxListModel: boxListModel
                    }

                    ViewportPanel {
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
            }
        }
    }
}
