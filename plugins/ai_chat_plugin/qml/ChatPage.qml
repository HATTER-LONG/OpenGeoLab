import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Main chat page layout: message list + status bar + input area.
 * Uses Loader delegates to switch between message types.
 */
Item {
    id: root

    required property var chatBackend

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Connection error banner ─────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: errorLayout.implicitHeight + 2 * PluginTheme.gapTight
            color: PluginTheme.tint(PluginTheme.danger, 0.15)
            radius: PluginTheme.radiusSmall
            visible: root.chatBackend.connectionError.length > 0

            RowLayout {
                id: errorLayout
                anchors.fill: parent
                anchors.margins: PluginTheme.gapTight
                spacing: PluginTheme.gapTight

                Label {
                    Layout.fillWidth: true
                    text: root.chatBackend.connectionError
                    color: PluginTheme.danger
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }

                Button {
                    text: qsTr("Retry")
                    onClicked: root.chatBackend.retryConnection()

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 12
                        color: PluginTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                    }

                    background: Rectangle {
                        implicitHeight: 28
                        implicitWidth: 60
                        radius: 4
                        color: parent.down
                               ? PluginTheme.surfaceStrong
                               : PluginTheme.surfaceMuted
                        border.width: 1
                        border.color: PluginTheme.borderSubtle
                    }
                }
            }
        }

        // ── Connecting indicator ────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall
            visible: root.chatBackend.isConnecting
                     && root.chatBackend.connectionError.length === 0

            RowLayout {
                anchors.centerIn: parent
                spacing: PluginTheme.gapTight

                BusyIndicator {
                    running: true
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                }

                Label {
                    text: qsTr("Connecting to AI assistant...")
                    color: PluginTheme.textSecondary
                    font.pixelSize: 12
                }
            }
        }

        // ── Message list ────────────────────────────────────────────
        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: PluginTheme.gapTight
            model: root.chatBackend.messageModel
            verticalLayoutDirection: ListView.TopToBottom

            // Auto-scroll to bottom
            onCountChanged: {
                Qt.callLater(function() {
                    messageList.positionViewAtEnd()
                })
            }

            delegate: Loader {
                id: delegateLoader
                width: messageList.width - 2 * PluginTheme.gapTight
                x: PluginTheme.gapTight

                // Expose model roles to loaded components
                required property int index
                required property string msgId
                required property string msgType
                required property string content
                required property bool isHtml
                required property string toolName
                required property string toolCallId
                required property string toolStatus
                required property string toolResult
                required property var choices
                required property bool answered
                required property string reasoning

                // Use setSource to pass properties into the loaded component,
                // satisfying required property declarations at creation time.
                Component.onCompleted: _loadDelegate()

                function _loadDelegate() {
                    switch (msgType) {
                        case "user":
                        case "assistant":
                        case "system":
                            setSource("MessageDelegate.qml", {
                                "index": Qt.binding(function() { return delegateLoader.index }),
                                "msgId": Qt.binding(function() { return delegateLoader.msgId }),
                                "msgType": Qt.binding(function() { return delegateLoader.msgType }),
                                "content": Qt.binding(function() { return delegateLoader.content }),
                                "isHtml": Qt.binding(function() { return delegateLoader.isHtml }),
                                "reasoning": Qt.binding(function() { return delegateLoader.reasoning }),
                            })
                            break
                        case "tool":
                            setSource("ToolCallCard.qml", {
                                "toolName": Qt.binding(function() { return delegateLoader.toolName }),
                                "toolStatus": Qt.binding(function() { return delegateLoader.toolStatus }),
                                "toolResult": Qt.binding(function() { return delegateLoader.toolResult }),
                            })
                            break
                        case "askUser":
                            setSource("AskUserPanel.qml", {
                                "content": Qt.binding(function() { return delegateLoader.content }),
                                "choices": Qt.binding(function() { return delegateLoader.choices }),
                                "answered": Qt.binding(function() { return delegateLoader.answered }),
                                "chatBackend": root.chatBackend,
                            })
                            break
                    }
                }
            }

            // Empty state
            Label {
                anchors.centerIn: parent
                visible: messageList.count === 0
                         && !root.chatBackend.isConnecting
                         && root.chatBackend.connectionError.length === 0
                text: qsTr("Start a conversation with OpenGeoLab AI")
                color: PluginTheme.textTertiary
                font.pixelSize: 14
            }
        }

        // ── Streaming indicator ─────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 24
            color: "transparent"
            visible: root.chatBackend.isStreaming

            Label {
                anchors.left: parent.left
                anchors.leftMargin: PluginTheme.gap
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("AI is responding...")
                color: PluginTheme.textTertiary
                font.pixelSize: 11
                font.italic: true
            }
        }

        // ── Model selector bar ──────────────────────────────────────
        ModelSelectorBar {
            Layout.fillWidth: true
            Layout.leftMargin: PluginTheme.gapTight
            Layout.rightMargin: PluginTheme.gapTight
            chatBackend: root.chatBackend
        }

        // ── Input area ──────────────────────────────────────────────
        ChatInputArea {
            Layout.fillWidth: true
            Layout.margins: PluginTheme.gapTight
            enabled: !root.chatBackend.isConnecting
                     && !root.chatBackend.isStreaming

            onSendMessage: function(text) {
                root.chatBackend.sendMessage(text)
            }
        }
    }
}
