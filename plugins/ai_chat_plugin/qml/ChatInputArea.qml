import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Text input area at the bottom of the Chat page.
 * Enter sends, Shift+Enter inserts newline.
 * Send button disabled when input is empty.
 * Includes a 📎 button for viewport capture and an attachment preview strip.
 */
Rectangle {
    id: root

    color: PluginTheme.surface
    radius: PluginTheme.radiusSmall
    border.width: 1
    border.color: inputField.activeFocus
                  ? PluginTheme.accentA
                  : PluginTheme.borderSubtle
    implicitHeight: mainLayout.implicitHeight + 2 * PluginTheme.gapTight

    signal sendMessage(string text)
    signal captureViewport()
    signal clearAttachment()

    property bool hasAttachment: false
    property string attachmentThumbnail: ""

    function clear() {
        inputField.text = ""
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: PluginTheme.gapTight
        spacing: 2

        // Attachment preview (shown when a screenshot is pending)
        Rectangle {
            id: attachmentPreview
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 28 : 0
            visible: root.hasAttachment
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall / 2

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 4
                spacing: 4

                Text {
                    text: "\uD83D\uDCF7"
                    font.pixelSize: 13
                }

                Text {
                    text: qsTr("viewport.png")
                    font.pixelSize: 11
                    color: PluginTheme.textSecondary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Button {
                    id: removeAttachButton

                    flat: true
                    implicitWidth: 20
                    implicitHeight: 20
                    onClicked: root.clearAttachment()

                    contentItem: Text {
                        text: "\u2715"
                        font.pixelSize: 11
                        color: PluginTheme.textTertiary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Item {}
                }
            }
        }

        // Input row
        RowLayout {
            id: inputLayout
            Layout.fillWidth: true
            spacing: PluginTheme.gapTight

            // Capture viewport button
            Button {
                id: captureButton

                Layout.alignment: Qt.AlignBottom
                onClicked: root.captureViewport()

                contentItem: Text {
                    text: "\uD83D\uDCCE"
                    font.pixelSize: 16
                    color: PluginTheme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: PluginTheme.radiusSmall / 2
                    color: captureButton.hovered
                           ? PluginTheme.surfaceMuted
                           : "transparent"

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.maximumHeight: 120

                TextArea {
                    id: inputField

                    placeholderText: qsTr("Ask OpenGeoLab AI...")
                    placeholderTextColor: PluginTheme.textTertiary
                    color: PluginTheme.textPrimary
                    selectionColor: PluginTheme.tint(PluginTheme.accentA, 0.4)
                    font.pixelSize: 13
                    wrapMode: TextEdit.Wrap
                    background: Item {}

                    Keys.onReturnPressed: function(event) {
                        if (!(event.modifiers & Qt.ShiftModifier)) {
                            if (inputField.text.trim().length > 0) {
                                root.sendMessage(inputField.text.trim())
                                inputField.text = ""
                            }
                            event.accepted = true
                        }
                    }
                }
            }

            Button {
                id: sendButton

                text: "\u27A4"
                enabled: inputField.text.trim().length > 0
                Layout.alignment: Qt.AlignBottom

                onClicked: {
                    if (inputField.text.trim().length > 0) {
                        root.sendMessage(inputField.text.trim())
                        inputField.text = ""
                    }
                }

                contentItem: Text {
                    text: sendButton.text
                    font.pixelSize: 16
                    color: sendButton.enabled
                           ? PluginTheme.textOnAccent
                           : PluginTheme.textTertiary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: PluginTheme.radiusSmall / 2
                    color: sendButton.enabled
                           ? (sendButton.down
                              ? Qt.darker(PluginTheme.accentA, 1.2)
                              : PluginTheme.accentA)
                           : PluginTheme.surfaceMuted

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }
        }
    }

    Behavior on border.color {
        ColorAnimation { duration: PluginTheme.animFast }
    }
}
