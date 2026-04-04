import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Tab-merged plugin window: Chat + Action Debugger.
 * Default tab is Chat (index 0).
 */
ApplicationWindow {
    id: window

    width: 1000
    height: 700
    title: qsTr("AI Chat — OpenGeoLab")
    color: PluginTheme.bg
    visible: true

    required property var backend
    required property var chatBackend

    Binding {
        target: PluginTheme
        property: "darkMode"
        value: window.backend.isDark
    }

    Binding {
        target: window.chatBackend
        property: "darkMode"
        value: window.backend.isDark
    }

    header: ToolBar {
        background: Rectangle {
            color: PluginTheme.surface
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: PluginTheme.borderSubtle
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: PluginTheme.gap
            anchors.rightMargin: PluginTheme.gap

            TabBar {
                id: tabBar
                Layout.fillWidth: false
                background: Item {}

                TabButton {
                    text: qsTr("💬 Chat")
                    width: implicitWidth
                    font.pixelSize: 13

                    background: Rectangle {
                        color: tabBar.currentIndex === 0
                               ? PluginTheme.surfaceStrong
                               : "transparent"
                        radius: PluginTheme.radiusSmall
                        Behavior on color {
                            ColorAnimation { duration: PluginTheme.animFast }
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: tabBar.currentIndex === 0
                               ? PluginTheme.textPrimary
                               : PluginTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TabButton {
                    text: qsTr("🔧 Debugger")
                    width: implicitWidth
                    font.pixelSize: 13

                    background: Rectangle {
                        color: tabBar.currentIndex === 1
                               ? PluginTheme.surfaceStrong
                               : "transparent"
                        radius: PluginTheme.radiusSmall
                        Behavior on color {
                            ColorAnimation { duration: PluginTheme.animFast }
                        }
                    }

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: tabBar.currentIndex === 1
                               ? PluginTheme.textPrimary
                               : PluginTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                visible: tabBar.currentIndex === 0
                icon.source: window.backend.iconPath + "/new_session.svg"
                icon.color: PluginTheme.textSecondary
                icon.width: 20
                icon.height: 20
                onClicked: window.chatBackend.newSession()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("New session")
                ToolTip.delay: 500

                background: Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: PluginTheme.radiusSmall / 2
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : parent.hovered
                             ? PluginTheme.surfaceMuted
                             : "transparent"
                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }

            ToolButton {
                icon.source: window.backend.iconPath + (
                    window.backend.isDark
                        ? "/darkTheme.svg"
                        : "/lightTheme.svg"
                )
                icon.color: PluginTheme.textSecondary
                icon.width: 20
                icon.height: 20
                onClicked: window.backend.toggleTheme()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Toggle theme")
                ToolTip.delay: 500

                background: Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: PluginTheme.radiusSmall / 2
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : parent.hovered
                             ? PluginTheme.surfaceMuted
                             : "transparent"
                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        anchors.margins: PluginTheme.gap
        currentIndex: tabBar.currentIndex

        ChatPage {
            chatBackend: window.chatBackend
        }

        ActionDebuggerPage {
            backend: window.backend
        }
    }
}
