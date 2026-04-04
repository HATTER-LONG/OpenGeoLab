import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Standalone window wrapping ActionDebuggerPage with a toolbar.
 * Used by both hosted and standalone modes.
 */
ApplicationWindow {
    id: window

    width: 1000
    height: 700
    title: qsTr("Action Debugger — AI Chat Plugin")
    color: PluginTheme.bg
    visible: true

    // Backend is injected as a context property named "backend".
    required property var backend

    // Bind theme singleton to backend dark mode state.
    Binding {
        target: PluginTheme
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

            Label {
                text: qsTr("Action Debugger")
                color: PluginTheme.textPrimary
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

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

    ActionDebuggerPage {
        anchors.fill: parent
        anchors.margins: PluginTheme.gap
        backend: window.backend
    }
}
