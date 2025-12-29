pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @brief Ribbon container with dynamic tabs and a single action output.
 *
 * Design goals:
 * - A single outbound signal: actionTriggered(actionId, payload)
 * - Tabs generated from config.tabs
 * - Theme injected via Theme singleton tokens
 * - Optional config validation on startup
 */
Rectangle {
    id: root

    // Single outbound signal (keeps host logic centralized).
    signal actionTriggered(string actionId, var payload)

    // Default config instance.
    RibbonConfig {
        id: defaultConfig
    }

    property var config: defaultConfig

    // Current tab index (does not include File).
    property int currentTabIndex: 0

    // Theme tokens.
    property color accentColor: Theme.ribbonAccentColor
    property color hoverColor: Theme.ribbonHoverColor
    property color selectedTabColor: Theme.ribbonSelectedTabColor
    property color borderColor: Theme.ribbonBorderColor
    property color tabBackgroundColor: Theme.ribbonTabBarColor
    property color contentBackgroundColor: Theme.ribbonContentColor
    property color textColor: Theme.ribbonTextColor
    property color textColorDim: Theme.ribbonTextDimColor
    property color iconColor: Theme.ribbonIconColor

    height: 120
    color: tabBackgroundColor

    Component.onCompleted: {
        // Optional config validation; useful during maintenance.
        if (root.config && root.config.validate) {
            const ok = root.config.validate();
            if (!ok)
                console.warn("[RibbonToolBarV2] Config validation failed. Please fix duplicate/missing ids.");
        }
    }

    // File menu popup.
    RibbonFileMenu {
        id: fileMenu
        x: 0
        y: tabBar.height

        // Route actions through the single outbound signal.
        onActionTriggered: (actionId, payload) => root.actionTriggered(actionId, payload)
    }

    // =========================
    // TAB BAR
    // =========================
    Rectangle {
        id: tabBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 28
        color: root.tabBackgroundColor

        RowLayout {
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            // File button (fixed).
            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 24
                radius: 2
                color: fileMenu.visible ? root.accentColor : (fileArea.containsMouse ? root.hoverColor : "transparent")

                Text {
                    anchors.centerIn: parent
                    text: qsTr("File")
                    color: root.textColor
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: fileArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: fileMenu.open()
                }
            }

            // Dynamic tabs from config.tabs.
            Repeater {
                model: (root.config && root.config.tabs) ? root.config.tabs : []

                Rectangle {
                    id: tabBtn
                    required property int index
                    required property var modelData

                    Layout.preferredWidth: 86
                    Layout.preferredHeight: 24

                    // Selected / hover state.
                    color: root.currentTabIndex === index ? root.selectedTabColor : (tabArea.containsMouse ? root.hoverColor : "transparent")

                    border.width: root.currentTabIndex === index ? 1 : 0
                    border.color: root.currentTabIndex === index ? root.accentColor : root.borderColor

                    // Top indicator line.
                    Rectangle {
                        visible: root.currentTabIndex === tabBtn.index
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 2
                        color: root.accentColor
                    }

                    // Bottom cover line to visually connect tab and content.
                    Rectangle {
                        visible: root.currentTabIndex === tabBtn.index
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 1
                        anchors.rightMargin: 1
                        height: 2
                        color: root.contentBackgroundColor
                    }

                    Text {
                        anchors.centerIn: parent
                        text: tabBtn.modelData.title || qsTr("Tab")
                        color: root.textColor
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: tabArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.currentTabIndex = tabBtn.index
                    }
                }
            }
        }
    }

    // =========================
    // CONTENT AREA
    // =========================
    Rectangle {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom
        color: root.contentBackgroundColor
        border.width: 1
        border.color: root.borderColor

        // Single TabContent; data changes with currentTabIndex.
        RibbonTabContent {
            anchors.fill: parent

            // Resolve groups from config.tabs[currentTabIndex].
            groups: {
                const tabs = (root.config && root.config.tabs) ? root.config.tabs : [];
                const tab = tabs[root.currentTabIndex];
                return tab ? (tab.groups || []) : [];
            }

            iconColor: root.iconColor
            textColor: root.textColor
            textColorDim: root.textColorDim
            hoverColor: root.hoverColor
            pressedColor: Theme.ribbonPressedColor
            separatorColor: root.borderColor

            onActionTriggered: (actionId, payload) => root.actionTriggered(actionId, payload)
        }
    }
}
