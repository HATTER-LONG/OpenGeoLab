import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Per-message visual delegate for the chat ListView.
 * Switches layout based on msgType: user (right-aligned bubble),
 * assistant (left-aligned with Markdown), system (centered muted).
 */
Item {
    id: root

    required property int index
    required property string msgId
    required property string msgType
    required property string content
    required property bool isHtml

    // Full width of the ListView
    width: ListView.view ? ListView.view.width : 400
    implicitHeight: loader.implicitHeight

    Loader {
        id: loader
        anchors.left: parent.left
        anchors.right: parent.right
        sourceComponent: {
            switch (root.msgType) {
                case "user": return userBubble
                case "assistant": return assistantBubble
                case "system": return systemMsg
                default: return null
            }
        }
    }

    // ── User Bubble (right-aligned) ─────────────────────────────────
    Component {
        id: userBubble

        Item {
            implicitHeight: userRect.height

            Rectangle {
                id: userRect
                anchors.right: parent.right
                width: Math.min(
                    userText.implicitWidth + 2 * PluginTheme.gap,
                    parent.width * 0.75
                )
                height: userText.implicitHeight + 2 * PluginTheme.gapTight
                radius: PluginTheme.radiusSmall
                color: PluginTheme.accentA

                Text {
                    id: userText
                    anchors.fill: parent
                    anchors.margins: PluginTheme.gapTight
                    text: root.content
                    color: PluginTheme.textOnAccent
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    // ── Assistant Bubble (left-aligned) ─────────────────────────────
    Component {
        id: assistantBubble

        Item {
            implicitHeight: assistantArea.implicitHeight + 2 * PluginTheme.gapTight

            Rectangle {
                anchors.left: parent.left
                width: Math.min(
                    assistantArea.implicitWidth + 2 * PluginTheme.gap,
                    parent.width * 0.85
                )
                height: parent.implicitHeight
                radius: PluginTheme.radiusSmall
                color: PluginTheme.surfaceMuted

                TextArea {
                    id: assistantArea
                    anchors.fill: parent
                    anchors.margins: PluginTheme.gapTight
                    readOnly: true
                    textFormat: root.isHtml ? TextEdit.RichText : TextEdit.PlainText
                    text: root.isHtml ? root.content : root.content + _cursor
                    color: PluginTheme.textPrimary
                    font.pixelSize: 13
                    wrapMode: TextEdit.Wrap
                    background: Item {}
                    selectByMouse: true

                    // Blinking cursor during streaming
                    property string _cursor: ""
                    Timer {
                        running: !root.isHtml
                        interval: 530
                        repeat: true
                        onTriggered: {
                            assistantArea._cursor =
                                assistantArea._cursor === " ▌" ? "" : " ▌"
                        }
                    }
                }
            }
        }
    }

    // ── System Message (centered, muted) ────────────────────────────
    Component {
        id: systemMsg

        Item {
            implicitHeight: sysText.implicitHeight + PluginTheme.gapTight

            Text {
                id: sysText
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.8
                text: root.content
                color: PluginTheme.textTertiary
                font.pixelSize: 12
                font.italic: true
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
