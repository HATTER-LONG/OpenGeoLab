import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Compact bar showing the active model selector and auth connection status.
 *
 * Left side: editable ComboBox for model name.
 * Right side: clickable auth status indicator (dot + label).
 */
Item {
    id: root

    required property var chatBackend
    implicitHeight: barRow.implicitHeight + 2 * PluginTheme.gapTight

    // Resolve the current model from config
    readonly property var chatConfig: root.chatBackend.chatConfig
    readonly property string currentModel: {
        if (chatConfig.authMethod === "byok")
            return chatConfig.byokModel
        return chatConfig.lastModel
    }

    // Auth status label for the right side
    readonly property string authLabel: {
        if (chatConfig.authMethod === "byok") {
            var name = chatConfig.byokProvider
            return "BYOK: " + name.charAt(0).toUpperCase() + name.slice(1)
        }
        return "GitHub"
    }

    Rectangle {
        anchors.fill: parent
        color: PluginTheme.surfaceMuted
        radius: PluginTheme.radiusSmall
    }

    RowLayout {
        id: barRow
        anchors.fill: parent
        anchors.margins: PluginTheme.gapTight
        spacing: PluginTheme.gapTight

        // ── Model selector (editable combo) ─────────────────────
        ComboBox {
            id: modelCombo
            Layout.fillWidth: true
            Layout.maximumWidth: 280
            editable: true
            enabled: !root.chatBackend.isStreaming
                     && !root.chatBackend.isConnecting

            editText: root.currentModel

            // When user presses Enter or focus leaves, switch model
            onAccepted: {
                var text = editText.trim()
                if (text.length > 0 && text !== root.currentModel) {
                    root.chatBackend.switchModel(text)
                }
            }

            contentItem: TextField {
                text: modelCombo.editText
                font.pixelSize: 12
                font.family: PluginTheme.monoFont
                color: PluginTheme.textPrimary
                placeholderText: qsTr("Model name...")
                placeholderTextColor: PluginTheme.textTertiary
                verticalAlignment: Text.AlignVCenter
                leftPadding: PluginTheme.gapTight

                background: null

                onAccepted: modelCombo.accepted()
            }

            background: Rectangle {
                implicitHeight: 32
                radius: 6
                color: modelCombo.activeFocus
                       ? PluginTheme.surface
                       : PluginTheme.surfaceStrong
                border.width: 1
                border.color: modelCombo.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
            }
        }

        // ── Spacer ──────────────────────────────────────────────
        Item { Layout.fillWidth: true }

        // ── Auth status indicator (clickable) ───────────────────
        MouseArea {
            id: statusArea
            Layout.preferredWidth: statusRow.implicitWidth
            Layout.preferredHeight: statusRow.implicitHeight
            cursorShape: Qt.PointingHandCursor

            onClicked: authPopup.open()

            RowLayout {
                id: statusRow
                spacing: 6

                Rectangle {
                    id: statusDot
                    width: 8; height: 8
                    radius: 4

                    color: {
                        switch (root.chatBackend.connectionStatus) {
                            case "connected":  return PluginTheme.success
                            case "connecting": return PluginTheme.warning
                            case "error":      return PluginTheme.danger
                            default:           return PluginTheme.textTertiary
                        }
                    }
                }

                Label {
                    font.pixelSize: 11
                    color: PluginTheme.textSecondary

                    text: {
                        switch (root.chatBackend.connectionStatus) {
                            case "connected":  return qsTr("Connected")
                            case "connecting": return qsTr("Connecting...")
                            case "error":      return qsTr("Error")
                            default:           return ""
                        }
                    }
                }

                Label {
                    font.pixelSize: 10
                    color: PluginTheme.textTertiary
                    text: "· " + root.authLabel
                }
            }
        }
    }

    // ── Auth settings popup ─────────────────────────────────────
    AuthSettingsPanel {
        id: authPopup
        chatBackend: root.chatBackend
    }
}
