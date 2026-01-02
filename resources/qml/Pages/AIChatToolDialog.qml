pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file AIChatToolDialog.qml
 * @brief Non-modal tool dialog for AI chat interface
 *
 * Provides a conversational interface for AI-assisted geometry operations.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("AI Chat")
    showOkButton: false
    cancelButtonText: qsTr("Close")
    preferredContentHeight: 400
    maxContentHeight: 500

    /**
     * @brief Chat messages model
     */
    property var messages: []

    /**
     * @brief Send a message to the AI
     */
    function sendMessage(): void {
        const text = inputField.text.trim();
        if (text.length === 0 || OGL.BackendService.busy) {
            return;
        }

        // Add user message
        const newMessages = root.messages.slice();
        newMessages.push({
            role: "user",
            content: text
        });
        root.messages = newMessages;

        inputField.text = "";

        // Request AI response
        OGL.BackendService.request("aiChat", {
            message: text
        });
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        // Chat history
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            color: Theme.surfaceAltColor
            radius: 6
            border.width: 1
            border.color: Theme.borderColor

            ListView {
                id: chatList
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 8
                model: root.messages

                delegate: Rectangle {
                    id: msgDelegate
                    required property var modelData
                    required property int index

                    width: chatList.width
                    height: msgContent.implicitHeight + 16
                    radius: 8
                    color: modelData.role === "user" ? Theme.primaryColor : Theme.surfaceColor
                    border.width: modelData.role === "user" ? 0 : 1
                    border.color: Theme.borderColor

                    Label {
                        id: msgContent
                        anchors.fill: parent
                        anchors.margins: 8
                        text: msgDelegate.modelData.content || ""
                        color: msgDelegate.modelData.role === "user" ? "#FFFFFF" : Theme.textPrimaryColor
                        wrapMode: Text.Wrap
                        font.pixelSize: 12
                    }
                }

                onCountChanged: {
                    Qt.callLater(() => chatList.positionViewAtEnd());
                }
            }

            // Empty state
            Label {
                anchors.centerIn: parent
                visible: root.messages.length === 0
                text: qsTr("Start a conversation...")
                color: Theme.textSecondaryColor
                font.italic: true
            }
        }

        // Input area
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: inputField
                Layout.fillWidth: true
                placeholderText: qsTr("Type your message...")
                enabled: !OGL.BackendService.busy

                color: Theme.textPrimaryColor
                placeholderTextColor: Theme.textSecondaryColor
                background: Rectangle {
                    implicitHeight: 32
                    radius: 6
                    color: inputField.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: inputField.activeFocus ? Theme.primaryColor : Theme.borderColor
                }

                Keys.onReturnPressed: root.sendMessage()
                Keys.onEnterPressed: root.sendMessage()
            }

            Button {
                id: sendBtn
                text: qsTr("Send")
                enabled: !OGL.BackendService.busy && inputField.text.trim().length > 0
                onClicked: root.sendMessage()

                background: Rectangle {
                    implicitWidth: 60
                    implicitHeight: 32
                    radius: 6
                    color: sendBtn.enabled ? (sendBtn.pressed ? Theme.buttonPressedColor : (sendBtn.hovered ? Theme.buttonHoverColor : Theme.primaryColor)) : Theme.buttonDisabledBackgroundColor
                }

                contentItem: Text {
                    text: sendBtn.text
                    color: sendBtn.enabled ? "#FFFFFF" : Theme.buttonDisabledTextColor
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Status area
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                running: true
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, result: var): void {
            if (moduleName === "aiChat") {
                const newMessages = root.messages.slice();
                newMessages.push({
                    role: "assistant",
                    content: result.response || qsTr("No response received.")
                });
                root.messages = newMessages;
            }
        }
    }

    // Clear messages when dialog is hidden
    onIsVisibleChanged: {
        if (!isVisible) {
            root.messages = [];
        }
    }
}
