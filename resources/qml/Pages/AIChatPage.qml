/**
 * @file AIChatPage.qml
 * @brief Function page for AI chat interface
 *
 * Provides an AI chat interface for interactive assistance.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("AI Chat")
    pageIcon: "qrc:/opengeolab/resources/icons/ai_chat.svg"
    serviceName: "AIService"
    actionId: "aiChat"

    width: 400
    height: 480

    // =========================================================
    // Parameters
    // =========================================================

    /// Current message
    property string currentMessage: ""
    /// Chat history
    property var chatHistory: []
    /// Include workspace context
    property bool includeContext: true

    function getParameters() {
        return {
            "action": "chat",
            "message": currentMessage,
            "includeContext": includeContext
        };
    }

    // Override execute to add message to history
    function execute() {
        if (currentMessage.trim().length === 0) return;

        // Add user message to history
        chatHistory.push({
            role: "user",
            content: currentMessage,
            timestamp: Date.now()
        });
        chatHistoryChanged();

        // Send request
        const params = getParameters();
        const jsonPayload = JSON.stringify(params);
        console.log("[AIChatPage] sending:", jsonPayload);

        if (serviceName) {
            BackendService.request(serviceName, jsonPayload);
        }
        executed(jsonPayload);

        // Clear input
        currentMessage = "";
    }

    // =========================================================
    // Content - Custom layout (override default)
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        // Chat history area
        Rectangle {
            width: parent.width
            height: 200  // Fixed height for chat history
            radius: 4
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            ListView {
                id: chatListView
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 8
                model: root.chatHistory

                delegate: ChatMessage {
                    required property var modelData
                    required property int index

                    width: chatListView.width
                    isUser: modelData.role === "user"
                    message: modelData.content
                }

                // Auto-scroll to bottom
                onCountChanged: {
                    Qt.callLater(() => positionViewAtEnd());
                }

                // Empty state
                Label {
                    anchors.centerIn: parent
                    visible: chatListView.count === 0
                    text: qsTr("Start a conversation with AI assistant")
                    font.pixelSize: 12
                    color: Theme.textDisabled
                }
            }
        }

        // Context toggle
        Rectangle {
            id: contextToggle
            width: parent.width
            height: contextRow.implicitHeight + 8
            radius: 4
            color: Theme.surface

            RowLayout {
                id: contextRow
                anchors.fill: parent
                anchors.margins: 4
                spacing: 8

                CheckBox {
                    id: contextCheck
                    checked: root.includeContext
                    onCheckedChanged: root.includeContext = checked

                    indicator: Rectangle {
                        implicitWidth: 14
                        implicitHeight: 14
                        radius: 3
                        color: contextCheck.checked ? Theme.accent : Theme.surface
                        border.width: 1
                        border.color: contextCheck.checked ? Theme.accent : Theme.border

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 9
                            color: Theme.white
                            visible: contextCheck.checked
                        }
                    }
                }

                Label {
                    text: qsTr("Include workspace context")
                    font.pixelSize: 10
                    color: Theme.textSecondary
                    Layout.fillWidth: true

                    MouseArea {
                        anchors.fill: parent
                        onClicked: contextCheck.toggle()
                    }
                }
            }
        }

        // Input area
        Rectangle {
            id: inputArea
            width: parent.width
            height: 60
            radius: 4
            color: Theme.surface
            border.width: messageInput.activeFocus ? 2 : 1
            border.color: messageInput.activeFocus ? Theme.accent : Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        id: messageInput
                        text: root.currentMessage
                        placeholderText: qsTr("Type your message...")
                        wrapMode: TextEdit.Wrap
                        font.pixelSize: 12
                        color: Theme.textPrimary
                        placeholderTextColor: Theme.textDisabled

                        background: Item {}

                        onTextChanged: root.currentMessage = text

                        Keys.onReturnPressed: event => {
                            if (event.modifiers & Qt.ShiftModifier) {
                                event.accepted = false;
                            } else {
                                root.execute();
                                event.accepted = true;
                            }
                        }
                    }
                }

                // Send button
                AbstractButton {
                    id: sendButton
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    Layout.alignment: Qt.AlignBottom
                    hoverEnabled: true
                    enabled: root.currentMessage.trim().length > 0

                    background: Rectangle {
                        radius: 18
                        color: sendButton.enabled ?
                               (sendButton.pressed ? Qt.darker(Theme.accent, 1.2) :
                                sendButton.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent) :
                               Theme.surfaceAlt
                    }

                    contentItem: Label {
                        text: "➤"
                        font.pixelSize: 16
                        color: sendButton.enabled ? Theme.white : Theme.textDisabled
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.execute()

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Send message (Enter)")
                    ToolTip.delay: 500
                }
            }
        }
    }

    // =========================================================
    // Chat message component
    // =========================================================
    component ChatMessage: Item {
        id: msgItem

        property bool isUser: true
        property string message: ""

        height: msgBubble.height + 4

        Rectangle {
            id: msgBubble
            anchors.right: msgItem.isUser ? parent.right : undefined
            anchors.left: msgItem.isUser ? undefined : parent.left
            width: Math.min(msgText.implicitWidth + 16, parent.width * 0.85)
            height: msgText.implicitHeight + 12
            radius: 8
            color: msgItem.isUser ? Theme.accent : Theme.surface
            border.width: msgItem.isUser ? 0 : 1
            border.color: Theme.border

            Label {
                id: msgText
                anchors.fill: parent
                anchors.margins: 8
                text: msgItem.message
                font.pixelSize: 11
                color: msgItem.isUser ? Theme.white : Theme.textPrimary
                wrapMode: Text.Wrap
            }
        }
    }
}
