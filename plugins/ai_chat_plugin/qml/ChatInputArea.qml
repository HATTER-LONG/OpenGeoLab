import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Text input area at the bottom of the Chat page.
 * Enter sends, Shift+Enter inserts newline.
 * Send button disabled when input is empty.
 */
Rectangle {
    id: root

    color: PluginTheme.surface
    radius: PluginTheme.radiusSmall
    border.width: 1
    border.color: inputField.activeFocus
                  ? PluginTheme.accentA
                  : PluginTheme.borderSubtle
    implicitHeight: inputLayout.implicitHeight + 2 * PluginTheme.gapTight

    signal sendMessage(string text)

    function clear() {
        inputField.text = ""
    }

    RowLayout {
        id: inputLayout
        anchors.fill: parent
        anchors.margins: PluginTheme.gapTight
        spacing: PluginTheme.gapTight

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

            text: "➤"
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

    Behavior on border.color {
        ColorAnimation { duration: PluginTheme.animFast }
    }
}
