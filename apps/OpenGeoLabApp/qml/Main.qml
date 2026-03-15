pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root

    required property var appController
    required property var uiSettingsController
    property bool darkMode: false
    property bool menuOpen: false
    property int selectedRibbonTab: 0
    property string statusNote: appController.lastSummary.length > 0 ? appController.lastSummary : qsTr("Viewport is active. Ribbon commands stay connected to the same controller pipeline.")

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

    ActionRegistry {
        id: actionRegistry
    }

    RibbonConfig {
        id: ribbonConfig
    }

    ActionFeaturePage {
        id: actionFeaturePage

        anchors.fill: parent
        theme: appTheme
        preferredPanelX: appTheme.shellPadding + 280 + appTheme.gap
        preferredPanelY: appTheme.shellPadding + 136 + appTheme.gap
    }

    GeometryCreateFeaturePage {
        id: geometryCreateFeaturePage

        anchors.fill: parent
        theme: appTheme
        appController: root.appController
        preferredPanelX: appTheme.shellPadding + 280 + appTheme.gap
        preferredPanelY: appTheme.shellPadding + 136 + appTheme.gap
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
        if (payload.summary) {
            return payload.summary;
        }
        return root.statusNote;
    }

    function toggleTheme() {
        root.darkMode = !root.darkMode;
        root.statusNote = root.darkMode ? qsTr("Switched to dark theme.") : qsTr("Switched to light theme.");
        root.menuOpen = false;
    }

    function toggleLanguage() {
        if (!root.uiSettingsController.toggleLanguage()) {
            root.statusNote = qsTr("Failed to switch UI language.");
            root.menuOpen = false;
            return;
        }

        if (actionFeaturePage.actionDefinition && actionFeaturePage.actionDefinition.key) {
            actionFeaturePage.refreshAction(actionRegistry.action(actionFeaturePage.actionDefinition.key));
        }
        if (geometryCreateFeaturePage.actionDefinition && geometryCreateFeaturePage.actionDefinition.key) {
            geometryCreateFeaturePage.refreshAction(actionRegistry.action(geometryCreateFeaturePage.actionDefinition.key));
        }

        root.statusNote = root.uiSettingsController.currentLanguage === "zh_CN"
            ? qsTr("Switched to Chinese.")
            : qsTr("Switched to English.");
        root.menuOpen = false;
    }

    function openActionPage(actionKey) {
        if (actionKey === "toggleTheme") {
            root.toggleTheme();
            return;
        }

        const actionDefinition = actionRegistry.action(actionKey);
        if (!actionDefinition) {
            root.statusNote = qsTr("Action page is not configured for: %1.").arg(actionKey);
            root.menuOpen = false;
            return;
        }

        actionFeaturePage.open = false;
        geometryCreateFeaturePage.open = false;

        if (actionDefinition.workflowKind === "geometryCreate") {
            geometryCreateFeaturePage.presentAction(actionDefinition);
        } else {
            actionFeaturePage.presentAction(actionDefinition);
        }

        root.statusNote = qsTr("Opened feature page: %1.").arg(actionDefinition.pageTitle);
        root.menuOpen = false;
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
                currentLanguage: root.uiSettingsController.currentLanguage
                selectedTab: root.selectedRibbonTab
                recordedCommandCount: root.appController.recordedCommandCount
                ribbonTabs: ribbonConfig.tabs
                ribbonGroups: ribbonConfig.groupsForTab(root.selectedRibbonTab)
                onToggleMenu: root.menuOpen = !root.menuOpen
                onRequestThemeToggle: root.toggleTheme()
                onRequestLanguageToggle: root.toggleLanguage()
                onSelectTab: function (tabIndex) {
                    root.selectedRibbonTab = tabIndex;
                }
                onTriggerAction: function (actionKey) {
                    root.openActionPage(actionKey);
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
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ViewportPanel {
                            anchors.fill: parent
                            theme: appTheme
                            summaryText: root.viewportSummary()
                            recordedCommandCount: root.appController.recordedCommandCount
                            onRequestViewPage: root.selectedRibbonTab = 2
                        }

                        OperationCornerOverlay {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 18
                            theme: appTheme
                            appController: root.appController
                        }
                    }
                }
            }
        }
    }
}
