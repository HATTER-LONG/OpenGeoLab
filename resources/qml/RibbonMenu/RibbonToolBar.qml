/**
 * @file RibbonToolBar.qml
 * @brief Main ribbon toolbar component with tabs and content area
 *
 * Provides Office-style ribbon UI with file menu, tabs, and action groups.
 * Emits actionTriggered for centralized action handling.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: ribbonToolBar
    width: parent.width
    height: 120
    color: Theme.ribbonBackground

    /// Currently selected tab index
    property int currentTabIndex: 0

    /// Emitted when any ribbon action is triggered
    signal actionTriggered(string actionId, var payload)

    RibbonFileMenu {
        id: fileMenu
        x: ribbonToolBar.x + 4
        y: ribbonToolBar.y + tabBar.height + 4

        onActionTriggered: (actionId, payload) => ribbonToolBar.actionTriggered(actionId, payload)
    }

    RibbonConfig {
        id: defaultConfig
    }
    property var config: defaultConfig
    Rectangle {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28
        color: Theme.ribbonTabBackground
        RowLayout {
            id: tabLayout
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            Layout.alignment: Qt.AlignLeft
            Rectangle {
                Layout.preferredWidth: 60
                Layout.fillHeight: true
                radius: 2
                color: "transparent"

                Text {
                    anchors.centerIn: parent
                    text: qsTr("File")
                    font.pixelSize: 12
                    font.bold: true
                    color: Theme.textPrimary
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.color = Theme.ribbonHoverColor
                    onExited: parent.color = "transparent"
                    onClicked: fileMenu.open()
                }
            }

            Repeater {
                model: ribbonToolBar.config.tabs
                Rectangle {
                    id: tabBtn
                    required property int index
                    required property var modelData

                    Layout.preferredWidth: 86
                    Layout.preferredHeight: 24
                    color: (ribbonToolBar.currentTabIndex === index) ? Theme.ribbonBackground : (tabArea.containsMouse ? Theme.ribbonHoverColor : "transparent")
                    radius: 2
                    Text {
                        anchors.centerIn: parent
                        text: tabBtn.modelData.title || qsTr("Tab")
                        color: Theme.palette.text
                        font.pixelSize: 12
                    }
                    Rectangle {
                        visible: ribbonToolBar.currentTabIndex === tabBtn.index
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        height: 2
                        color: Theme.accent
                    }
                    MouseArea {
                        id: tabArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: ribbonToolBar.currentTabIndex = tabBtn.index
                    }
                }
            }
        }
    }
    Rectangle {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom
        color: "transparent"
        border.width: 1
        border.color: Theme.border

        // Single TabContent; data changes with currentTabIndex.
        RibbonTabContent {
            anchors.fill: parent

            // Resolve groups from config.tabs[currentTabIndex].
            groups: {
                const tabs = (ribbonToolBar.config && ribbonToolBar.config.tabs) ? ribbonToolBar.config.tabs : [];
                const tab = tabs[ribbonToolBar.currentTabIndex];
                return tab ? (tab.groups || []) : [];
            }

            onActionTriggered: (actionId, payload) => ribbonToolBar.actionTriggered(actionId, payload)

            // iconColor: root.iconColor
            // textColor: root.textColor
            // textColorDim: root.textColorDim
            // hoverColor: root.hoverColor
            // pressedColor: Theme.ribbonPressedColor
            // separatorColor: root.borderColor

        }
    }
}
