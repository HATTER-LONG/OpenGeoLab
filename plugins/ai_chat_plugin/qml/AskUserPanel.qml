import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Interactive panel for SDK ask_user requests.
 * Shows the question, choice buttons (single-select), and freeform input.
 * Disabled after the user responds.
 */
Item {
    id: root

    required property string content
    required property var choices
    required property bool answered
    required property var chatBackend

    implicitHeight: panel.height

    Rectangle {
        id: panel
        anchors.left: parent.left
        width: Math.min(parent.width * 0.85, 500)
        height: panelLayout.implicitHeight + 2 * PluginTheme.gap
        radius: PluginTheme.radiusSmall
        color: PluginTheme.surfaceStrong
        border.width: 1
        border.color: PluginTheme.accentB
        opacity: root.answered ? 0.6 : 1.0

        ColumnLayout {
            id: panelLayout
            anchors.fill: parent
            anchors.margins: PluginTheme.gap
            spacing: PluginTheme.gapTight
            enabled: !root.answered

            // Question text
            Label {
                Layout.fillWidth: true
                text: root.content
                color: PluginTheme.textPrimary
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }

            // Choice buttons (if provided)
            Flow {
                Layout.fillWidth: true
                spacing: PluginTheme.gapTight
                visible: root.choices && root.choices.length > 0

                Repeater {
                    model: root.choices || []

                    Button {
                        text: modelData

                        onClicked: {
                            root.chatBackend.respondToAskUser(modelData)
                        }

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 12
                            color: PluginTheme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitHeight: 32
                            radius: PluginTheme.radiusSmall / 2
                            color: parent.down
                                   ? PluginTheme.surfaceMuted
                                   : parent.hovered
                                     ? PluginTheme.surface
                                     : PluginTheme.surfaceMuted
                            border.width: 1
                            border.color: PluginTheme.borderSubtle
                        }
                    }
                }
            }

            // Freeform input
            RowLayout {
                Layout.fillWidth: true
                spacing: PluginTheme.gapTight

                TextField {
                    id: freeformInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Type a response...")
                    placeholderTextColor: PluginTheme.textTertiary
                    color: PluginTheme.textPrimary
                    font.pixelSize: 13

                    background: Rectangle {
                        radius: PluginTheme.radiusSmall / 2
                        color: PluginTheme.surface
                        border.width: 1
                        border.color: freeformInput.activeFocus
                                      ? PluginTheme.accentA
                                      : PluginTheme.borderSubtle
                    }

                    Keys.onReturnPressed: {
                        if (freeformInput.text.trim().length > 0) {
                            root.chatBackend.respondToAskUser(
                                freeformInput.text.trim()
                            )
                        }
                    }
                }

                Button {
                    text: qsTr("Send")
                    enabled: freeformInput.text.trim().length > 0

                    onClicked: {
                        root.chatBackend.respondToAskUser(
                            freeformInput.text.trim()
                        )
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 12
                        color: parent.enabled
                               ? PluginTheme.textOnAccent
                               : PluginTheme.textTertiary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitHeight: 32
                        implicitWidth: 60
                        radius: PluginTheme.radiusSmall / 2
                        color: parent.enabled
                               ? PluginTheme.accentA
                               : PluginTheme.surfaceMuted
                    }
                }
            }

            // Answered indicator
            Label {
                visible: root.answered
                text: qsTr("✓ Responded")
                color: PluginTheme.success
                font.pixelSize: 11
                font.italic: true
            }
        }
    }
}
