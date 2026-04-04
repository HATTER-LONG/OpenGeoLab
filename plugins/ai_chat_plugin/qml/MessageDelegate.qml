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
    required property string reasoning

    // Fill Loader width — the Loader is our parent when loaded via setSource
    width: parent ? parent.width : 400

    implicitHeight: {
        switch (root.msgType) {
            case "user": return userRect.height
            case "assistant":
                var bubbleH = (root.content.length > 0 || root.isHtml)
                              ? assistantRect.height : 0
                var thinkH = root.reasoning.length > 0
                             ? thinkBlock.height + PluginTheme.gapTight : 0
                return bubbleH + thinkH
            case "system": return sysText.implicitHeight + PluginTheme.gapTight
            default: return 0
        }
    }

    // ── User Bubble (right-aligned) ─────────────────────────────────
    Rectangle {
        id: userRect
        visible: root.msgType === "user"
        anchors.right: parent.right
        width: Math.min(
            userText.implicitWidth + 2 * PluginTheme.gap,
            root.width * 0.75
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

    // ── Thinking / Reasoning (collapsible gray text) ──────────────
    Column {
        id: thinkBlock
        visible: root.msgType === "assistant" && root.reasoning.length > 0
        anchors.left: parent.left
        width: root.width * 0.85

        // Auto-expand during streaming, collapse once message is finalized
        property bool expanded: !root.isHtml

        Item {
            width: parent.width
            height: thinkHeader.implicitHeight

            Row {
                id: thinkHeader
                spacing: 4

                Text {
                    text: thinkBlock.expanded ? "▼" : "▶"
                    color: PluginTheme.textTertiary
                    font.pixelSize: 11
                }
                Text {
                    text: qsTr("Thinking")
                    color: PluginTheme.textTertiary
                    font.pixelSize: 11
                    font.italic: true
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: thinkBlock.expanded = !thinkBlock.expanded
            }
        }

        Text {
            visible: thinkBlock.expanded
            width: parent.width
            text: root.reasoning
            color: PluginTheme.textTertiary
            font.pixelSize: 11
            font.italic: true
            wrapMode: Text.Wrap
            topPadding: 4
        }
    }

    // ── Assistant Bubble (left-aligned) ─────────────────────────────
    Rectangle {
        id: assistantRect
        visible: root.msgType === "assistant"
                 && (root.content.length > 0 || root.isHtml)
        anchors.left: parent.left
        anchors.top: thinkBlock.visible ? thinkBlock.bottom : parent.top
        anchors.topMargin: thinkBlock.visible ? PluginTheme.gapTight : 0
        width: Math.min(
            assistantArea.implicitWidth + 2 * PluginTheme.gap,
            root.width * 0.85
        )
        height: assistantArea.implicitHeight + 2 * PluginTheme.gapTight
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

    // ── System Message (centered, muted) ────────────────────────────
    Text {
        id: sysText
        visible: root.msgType === "system"
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.width * 0.8
        text: root.content
        color: PluginTheme.textTertiary
        font.pixelSize: 12
        font.italic: true
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }
}
