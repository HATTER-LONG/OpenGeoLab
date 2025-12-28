pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import "."

/**
 * RibbonToolBarV2.qml
 * -------------------
 * 目标：更易移植、更易扩展的 Ribbon 容器。
 *
 * 关键优化：
 * 1) 统一出口：signal actionTriggered(actionId, payload)
 * 2) Tab 动态化：根据 config.tabs 自动生成 Tab
 * 3) 主题可注入：颜色属性开放给宿主（移植到别的程序不用改 Ribbon 内部）
 * 4) 可选配置校验：启动时 validate()
 */
Rectangle {
    id: root

    // ===== 对外统一出口（替代大量 signals + switch）=====
    signal actionTriggered(string actionId, var payload)

    // ===== 配置注入：默认使用 RibbonConfigV2（单例）=====
    RibbonConfig {
        id: defaultConfig
    }

    property var config: defaultConfig

    // 当前选中的 Tab（不含 File）
    property int currentTabIndex: 0

    // ===== 主题/颜色（默认值与你旧版接近）=====
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
        // 配置校验：移植/维护时非常建议打开
        if (root.config && root.config.validate) {
            const ok = root.config.validate();
            if (!ok)
                console.warn("[RibbonToolBarV2] Config validation failed. Please fix duplicate/missing ids.");
        }
    }

    // ===== FILE MENU（可选：先做最小版，再慢慢增强）=====
    RibbonFileMenu {
        id: fileMenu
        x: 0
        y: tabBar.height

        // FileMenu 也走统一出口
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

            // ---- File 按钮（固定）----
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

            // ---- 动态 Tab（来自 config.tabs）----
            Repeater {
                model: (root.config && root.config.tabs) ? root.config.tabs : []

                Rectangle {
                    id: tabBtn
                    required property int index
                    required property var modelData

                    Layout.preferredWidth: 86
                    Layout.preferredHeight: 24

                    // 选中态 / hover 态
                    color: root.currentTabIndex === index ? root.selectedTabColor : (tabArea.containsMouse ? root.hoverColor : "transparent")

                    border.width: root.currentTabIndex === index ? 1 : 0
                    border.color: root.currentTabIndex === index ? root.accentColor : root.borderColor

                    // 顶部选中高亮线
                    Rectangle {
                        visible: root.currentTabIndex === tabBtn.index
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 2
                        color: root.accentColor
                    }

                    // 底部遮盖线：让选中 tab 与内容区衔接更像“连在一起”
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

        // 只保留一个 TabContent：根据 currentTabIndex 切换数据
        RibbonTabContent {
            anchors.fill: parent

            // 从 config.tabs[currentTabIndex] 获取 groups
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

    // =========================
    // PUBLIC API（保持你旧版能力：Recent Files）
    // =========================
    function setRecentFiles(files): void {
        // files: string[] / var 都可
        fileMenu.setRecentFiles(files || []);
    }
}
