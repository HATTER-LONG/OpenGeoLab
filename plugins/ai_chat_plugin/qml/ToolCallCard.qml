import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Collapsible card showing a tool execution.
 * Shows tool name, status icon, and expandable result.
 */
Item {
    id: root

    required property string toolName
    required property string toolStatus
    required property string toolResult

    implicitHeight: card.height

    Rectangle {
        id: card
        anchors.left: parent.left
        width: Math.min(parent.width * 0.85, 500)
        height: cardLayout.implicitHeight + 2 * PluginTheme.gapTight
        radius: PluginTheme.radiusSmall
        color: PluginTheme.surface
        border.width: 1
        border.color: PluginTheme.borderSubtle

        property bool expanded: false

        ColumnLayout {
            id: cardLayout
            anchors.fill: parent
            anchors.margins: PluginTheme.gapTight
            spacing: PluginTheme.gapTight

            // Header row: status icon + tool name + status + toggle
            RowLayout {
                Layout.fillWidth: true
                spacing: PluginTheme.gapTight

                // Status icon
                Text {
                    text: {
                        switch (root.toolStatus) {
                            case "running": return "⏳"
                            case "success": return "✅"
                            case "error":   return "❌"
                            default:        return "🔧"
                        }
                    }
                    font.pixelSize: 14
                }

                // Tool name
                Label {
                    text: root.toolName
                    color: PluginTheme.textPrimary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                // Status label
                Label {
                    text: root.toolStatus
                    color: {
                        switch (root.toolStatus) {
                            case "running": return PluginTheme.warning
                            case "success": return PluginTheme.success
                            case "error":   return PluginTheme.danger
                            default:        return PluginTheme.textSecondary
                        }
                    }
                    font.pixelSize: 12
                }

                // Expand/collapse toggle
                ToolButton {
                    visible: root.toolResult.length > 0
                    text: card.expanded ? "▲" : "▼"
                    font.pixelSize: 10
                    onClicked: card.expanded = !card.expanded

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: PluginTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 24
                        implicitHeight: 24
                        radius: 4
                        color: parent.hovered
                               ? PluginTheme.surfaceMuted
                               : "transparent"
                    }
                }
            }

            // Expandable result area
            ScrollView {
                Layout.fillWidth: true
                Layout.maximumHeight: 200
                visible: card.expanded && root.toolResult.length > 0
                clip: true

                TextArea {
                    readOnly: true
                    text: root.toolResult
                    color: PluginTheme.textSecondary
                    font.family: PluginTheme.monoFont
                    font.pixelSize: 11
                    wrapMode: TextEdit.Wrap
                    background: Rectangle {
                        color: PluginTheme.surfaceMuted
                        radius: 4
                    }
                    selectByMouse: true
                }
            }
        }
    }

    // Spinner animation for running state
    SequentialAnimation on opacity {
        running: root.toolStatus === "running"
        loops: Animation.Infinite
        NumberAnimation { to: 0.6; duration: 800; easing.type: Easing.InOutSine }
        NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutSine }
    }
}
