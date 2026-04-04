# AI Chat Plugin — Segment 2a: Part 3 of 3

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build all QML chat components and perform integration verification in both hosted and standalone modes.

**Architecture:** Six QML files compose the Chat tab: `ChatPage.qml` (layout), `ChatInputArea.qml` (text input), `MessageDelegate.qml` (per-message visual), `ToolCallCard.qml` (tool execution display), `AskUserPanel.qml` (ask_user interaction), plus the already-created `PluginWindow.qml`.  All components use `PluginTheme` singleton for theming.

**Tech Stack:** QML / Qt Quick Controls, PySide6 6.9

**Spec:** `docs/superpowers/specs/2026-04-04-ai-chat-segment2a-chat-core-design.md`

**Depends on:** Part 1 (model, markdown) and Part 2 (backend, worker, window)

---

### Task 10: Create ChatInputArea.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/ChatInputArea.qml`

- [ ] **Step 1: Create `ChatInputArea.qml`**

Create file `plugins/ai_chat_plugin/qml/ChatInputArea.qml`:

```qml
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
```

- [ ] **Step 2: Commit**

```powershell
git add plugins/ai_chat_plugin/qml/ChatInputArea.qml
git commit -m "feat(ai-chat): add ChatInputArea QML component

Text input with Enter-to-send, Shift+Enter for newline, send button
disabled when empty, themed border highlight on focus.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 11: Create MessageDelegate.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/MessageDelegate.qml`

- [ ] **Step 1: Create `MessageDelegate.qml`**

Create file `plugins/ai_chat_plugin/qml/MessageDelegate.qml`:

```qml
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
```

- [ ] **Step 2: Commit**

```powershell
git add plugins/ai_chat_plugin/qml/MessageDelegate.qml
git commit -m "feat(ai-chat): add MessageDelegate QML component

User bubbles right-aligned with accent color, assistant bubbles left-aligned
with RichText/PlainText switch for streaming.  Blinking cursor during
streaming, system messages centered in muted italic.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 12: Create ToolCallCard.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/ToolCallCard.qml`

- [ ] **Step 1: Create `ToolCallCard.qml`**

Create file `plugins/ai_chat_plugin/qml/ToolCallCard.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Collapsible card showing a tool execution.
 * Shows tool name, status icon, and expandable result.
 */
Item {
    id: root

    required property string toolName
    required property string toolStatus
    required property string toolResult

    implicitHeight: card.height

    Rectangle {
        id: card
        anchors.left: parent.left
        width: Math.min(parent.width * 0.85, 500)
        height: cardLayout.implicitHeight + 2 * PluginTheme.gapTight
        radius: PluginTheme.radiusSmall
        color: PluginTheme.surface
        border.width: 1
        border.color: PluginTheme.borderSubtle

        property bool expanded: false

        ColumnLayout {
            id: cardLayout
            anchors.fill: parent
            anchors.margins: PluginTheme.gapTight
            spacing: PluginTheme.gapTight

            // Header row: status icon + tool name + status + toggle
            RowLayout {
                Layout.fillWidth: true
                spacing: PluginTheme.gapTight

                // Status icon
                Text {
                    text: {
                        switch (root.toolStatus) {
                            case "running": return "⏳"
                            case "success": return "✅"
                            case "error":   return "❌"
                            default:        return "🔧"
                        }
                    }
                    font.pixelSize: 14
                }

                // Tool name
                Label {
                    text: root.toolName
                    color: PluginTheme.textPrimary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                // Status label
                Label {
                    text: root.toolStatus
                    color: {
                        switch (root.toolStatus) {
                            case "running": return PluginTheme.warning
                            case "success": return PluginTheme.success
                            case "error":   return PluginTheme.danger
                            default:        return PluginTheme.textSecondary
                        }
                    }
                    font.pixelSize: 12
                }

                // Expand/collapse toggle
                ToolButton {
                    visible: root.toolResult.length > 0
                    text: card.expanded ? "▲" : "▼"
                    font.pixelSize: 10
                    onClicked: card.expanded = !card.expanded

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: PluginTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 24
                        implicitHeight: 24
                        radius: 4
                        color: parent.hovered
                               ? PluginTheme.surfaceMuted
                               : "transparent"
                    }
                }
            }

            // Expandable result area
            ScrollView {
                Layout.fillWidth: true
                Layout.maximumHeight: 200
                visible: card.expanded && root.toolResult.length > 0
                clip: true

                TextArea {
                    readOnly: true
                    text: root.toolResult
                    color: PluginTheme.textSecondary
                    font.family: PluginTheme.monoFont
                    font.pixelSize: 11
                    wrapMode: TextEdit.Wrap
                    background: Rectangle {
                        color: PluginTheme.surfaceMuted
                        radius: 4
                    }
                    selectByMouse: true
                }
            }
        }
    }

    // Spinner animation for running state
    SequentialAnimation on opacity {
        running: root.toolStatus === "running"
        loops: Animation.Infinite
        NumberAnimation { to: 0.6; duration: 800; easing.type: Easing.InOutSine }
        NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutSine }
    }
}
```

- [ ] **Step 2: Commit**

```powershell
git add plugins/ai_chat_plugin/qml/ToolCallCard.qml
git commit -m "feat(ai-chat): add ToolCallCard QML component

Collapsible card showing tool name, status icon (spinner/check/cross),
status color, and expandable monospace result area.  Pulse animation
while tool is running.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 13: Create AskUserPanel.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/AskUserPanel.qml`

- [ ] **Step 1: Create `AskUserPanel.qml`**

Create file `plugins/ai_chat_plugin/qml/AskUserPanel.qml`:

```qml
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
```

- [ ] **Step 2: Commit**

```powershell
git add plugins/ai_chat_plugin/qml/AskUserPanel.qml
git commit -m "feat(ai-chat): add AskUserPanel QML component

Interactive panel with question text, single-select choice buttons,
and freeform text input.  Disabled after user responds.  Enter key
sends freeform text.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 14: Create ChatPage.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/ChatPage.qml`

- [ ] **Step 1: Create `ChatPage.qml`**

Create file `plugins/ai_chat_plugin/qml/ChatPage.qml`:

```qml
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

                sourceComponent: {
                    switch (delegateLoader.msgType) {
                        case "user":
                        case "assistant":
                        case "system":
                            return messageDelegateComp
                        case "tool":
                            return toolCardComp
                        case "askUser":
                            return askUserComp
                        default:
                            return null
                    }
                }
            }

            // ── Delegate components ─────────────────────────────────

            Component {
                id: messageDelegateComp
                MessageDelegate {
                    index: delegateLoader.index
                    msgId: delegateLoader.msgId
                    msgType: delegateLoader.msgType
                    content: delegateLoader.content
                    isHtml: delegateLoader.isHtml
                }
            }

            Component {
                id: toolCardComp
                ToolCallCard {
                    toolName: delegateLoader.toolName
                    toolStatus: delegateLoader.toolStatus
                    toolResult: delegateLoader.toolResult
                }
            }

            Component {
                id: askUserComp
                AskUserPanel {
                    content: delegateLoader.content
                    choices: delegateLoader.choices
                    answered: delegateLoader.answered
                    chatBackend: root.chatBackend
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

        // ── Input area ──────────────────────────────────────────────
        ChatInputArea {
            Layout.fillWidth: true
            Layout.margins: PluginTheme.gapTight
            enabled: !root.chatBackend.isConnecting

            onSendMessage: function(text) {
                root.chatBackend.sendMessage(text)
            }
        }
    }
}
```

- [ ] **Step 2: Commit**

```powershell
git add plugins/ai_chat_plugin/qml/ChatPage.qml
git commit -m "feat(ai-chat): add ChatPage QML layout

Chat page with connection error banner, connecting indicator, message
ListView with Loader delegates for user/assistant/tool/askUser/system
types, streaming indicator, and ChatInputArea.  Auto-scrolls to bottom.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 15: Integration Verification

**Files:** None (verification only)

- [ ] **Step 1: Sync plugin to build directory**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
if (Test-Path build\bin\plugins\ai_chat_plugin) {
    Remove-Item -Recurse -Force build\bin\plugins\ai_chat_plugin
}
Copy-Item -Path "plugins\ai_chat_plugin" -Destination "build\bin\plugins\ai_chat_plugin" -Recurse -Force
```

- [ ] **Step 2: Standalone smoke test**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab\build\bin\plugins
& D:\WorkSpace\OGLWorkSpace\OpenGeoLab\pyvenv\Scripts\python.exe -m ai_chat_plugin
```

Verify:
1. Window opens with "AI Chat — OpenGeoLab" title
2. Two tabs visible: "💬 Chat" (active by default) and "🔧 Debugger"
3. Chat tab shows input area at bottom
4. If Copilot CLI is not installed/authenticated, connection error banner appears with retry button — **no crash**
5. Switch to Debugger tab → Action Debugger works as before
6. Close window → **no crash** (no-teardown pattern)

- [ ] **Step 3: Run unit tests**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
```

Expected: all tests from Part 1 (markdown_converter + chat_message_model + tool_handlers) pass.

- [ ] **Step 4: Build C++ project (ensure no breakage)**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
cmake --build build --config RelWithDebInfo --parallel 4
```

Expected: build succeeds (no C++ changes were made, but confirm the build still works).

- [ ] **Step 5: Hosted mode test (if build available)**

Launch OpenGeoLab → AI Chat plugin → verify:
1. Chat tab opens by default
2. Connecting indicator shows while SDK initializes
3. If Copilot CLI is authenticated:
   - Type "list all available modules" → AI uses `list_modules` tool → tool card shows → AI reports modules
   - Type "describe the geometry module" → AI uses `describe_module` → reports actions
   - Type "create a box at origin" → AI uses `execute_action` → geometry appears
   - Streaming: text appears incrementally with blinking cursor
   - Tool cards show running → success status
4. Switch to Debugger tab → existing functionality unchanged
5. Close app → **no crash**

- [ ] **Step 6: Run all C++ tests**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: all existing tests pass (no C++ changes).

- [ ] **Step 7: Final commit (if any fixes needed)**

```powershell
git add -u plugins/ai_chat_plugin/
git commit -m "fix(ai-chat): integration fixes for Segment 2a

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

**End of Part 3 and Segment 2a plan.** All 15 tasks produce a working AI Chat feature with streaming conversation, tool execution, ask_user interaction, and tab-merged window.
