import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Compact bar showing the active model selector and auth connection status.
 *
 * Left side: editable ComboBox populated from SDK model list.
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

    // Build model ID list from availableModels (skip disabled)
    readonly property var modelIds: {
        var ids = []
        var models = root.chatBackend.availableModels
        if (models) {
            for (var i = 0; i < models.length; ++i) {
                var m = models[i]
                if (m.state && m.state === "disabled")
                    continue
                ids.push(m.id)
            }
        }
        return ids
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

    // Sync editText when currentModel changes (e.g. after switchModel)
    Connections {
        target: root
        function onCurrentModelChanged() {
            modelCombo.editText = root.currentModel
        }
    }

    RowLayout {
        id: barRow
        anchors.fill: parent
        anchors.margins: PluginTheme.gapTight
        spacing: PluginTheme.gapTight

        // ── Model selector (editable combo with SDK model list) ──
        ComboBox {
            id: modelCombo
            Layout.fillWidth: true
            Layout.maximumWidth: 280
            editable: true
            enabled: !root.chatBackend.isStreaming
                     && !root.chatBackend.isConnecting

            model: root.modelIds
            editText: root.currentModel

            // When user selects from dropdown or presses Enter
            onAccepted: _applyModel()
            onActivated: function(index) {
                if (index >= 0 && index < root.modelIds.length) {
                    editText = root.modelIds[index]
                    _applyModel()
                }
            }

            function _applyModel() {
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

            // Custom dropdown delegate showing model id
            delegate: ItemDelegate {
                required property int index
                required property var modelData
                width: modelCombo.width
                highlighted: modelCombo.highlightedIndex === index

                contentItem: Label {
                    text: modelData || ""
                    font.pixelSize: 12
                    font.family: PluginTheme.monoFont
                    color: parent.highlighted
                           ? PluginTheme.textOnAccent
                           : PluginTheme.textPrimary
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.highlighted ? PluginTheme.accentA : "transparent"
                    radius: 4
                }
            }

            popup: Popup {
                y: modelCombo.height + 2
                width: modelCombo.width
                implicitHeight: contentItem.implicitHeight + 2 * padding
                padding: 4

                contentItem: ListView {
                    clip: true
                    implicitHeight: Math.min(contentHeight, 300)
                    model: modelCombo.delegateModel
                    currentIndex: modelCombo.highlightedIndex
                    ScrollBar.vertical: ScrollBar {}
                }

                background: Rectangle {
                    color: PluginTheme.surface
                    border.width: 1
                    border.color: PluginTheme.borderSubtle
                    radius: 8
                }
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
