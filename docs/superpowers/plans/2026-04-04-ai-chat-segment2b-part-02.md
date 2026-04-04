# AI Chat Plugin — Segment 2b: Part 2 of 2

> Part 文件：QML Components & Integration — ModelSelectorBar, AuthSettingsPanel, ChatPage update, Verification.
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build the QML model selector bar and auth settings panel, wire them into ChatPage, and verify the full Segment 2b feature end-to-end.

**Architecture:** `ModelSelectorBar.qml` displays current model + auth status; `AuthSettingsPanel.qml` is a Popup for switching between GitHub and BYOK auth. Both bind to `ChatBackend.chatConfig` properties added in Part 1. `ChatPage.qml` inserts `ModelSelectorBar` between the streaming indicator and input area.

**Tech Stack:** QML (Qt Quick Controls), PySide6 6.9

**Spec:** `docs/superpowers/specs/2026-04-04-ai-chat-segment2b-auth-config-design.md`

**Depends on:** Part 1 (Tasks 1-5) must be complete.

---

### Task 6: ModelSelectorBar.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/ModelSelectorBar.qml`

**Layout reference (from spec):**
```
┌───────────────────────────────────────────────────┐
│ [▼ gpt-4o             ]             🟢 Connected  │
└───────────────────────────────────────────────────┘
```

Left: editable ComboBox for model name.  Right: colored dot + status label (clickable → opens AuthSettingsPanel).

- [ ] **Step 1: Create ModelSelectorBar.qml**

Create `plugins/ai_chat_plugin/qml/ModelSelectorBar.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Compact bar showing the active model selector and auth connection status.
 *
 * Left side: editable ComboBox for model name.
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

    RowLayout {
        id: barRow
        anchors.fill: parent
        anchors.margins: PluginTheme.gapTight
        spacing: PluginTheme.gapTight

        // ── Model selector (editable combo) ─────────────────────
        ComboBox {
            id: modelCombo
            Layout.fillWidth: true
            Layout.maximumWidth: 280
            editable: true
            enabled: !root.chatBackend.isStreaming
                     && !root.chatBackend.isConnecting

            editText: root.currentModel

            // When user presses Enter or focus leaves, switch model
            onAccepted: {
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
```

- [ ] **Step 2: Verify file loads (syntax check)**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -c "
from pathlib import Path
p = Path('plugins/ai_chat_plugin/qml/ModelSelectorBar.qml')
text = p.read_text(encoding='utf-8')
assert 'ModelSelectorBar' not in '__error__'
print(f'OK: {len(text)} chars, {text.count(chr(10))} lines')
"
```

- [ ] **Step 3: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/qml/ModelSelectorBar.qml
git commit -m "feat(ai-chat): add ModelSelectorBar QML component

Editable ComboBox for model name selection + clickable auth status
indicator (colored dot with connection state). Opens AuthSettingsPanel
popup on click.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: AuthSettingsPanel.qml

**Files:**
- Create: `plugins/ai_chat_plugin/qml/AuthSettingsPanel.qml`

**Layout reference (from spec):**
```
┌─ Authentication Settings ──────────────────────┐
│  Auth Method:                                   │
│  ( ● ) GitHub Copilot  ( ○ ) BYOK              │
│  ─── BYOK Settings (disabled when GitHub) ──── │
│  Provider:  [▼ OpenAI          ]                │
│  Base URL:  [https://...       ]                │
│  API Key:   [••••••••          ]                │
│  Model:     [gpt-4o            ]                │
│  Wire API:  [▼ completions     ]                │
│  [Test Connection]                              │
│  Status: ✅ Connected / ❌ Error message        │
│         [Cancel]  [Save & Reconnect]            │
└─────────────────────────────────────────────────┘
```

- [ ] **Step 1: Create AuthSettingsPanel.qml**

Create `plugins/ai_chat_plugin/qml/AuthSettingsPanel.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Popup overlay for configuring auth method (GitHub vs BYOK).
 *
 * Reads initial values from chatBackend.chatConfig. Changes are local
 * until "Save & Reconnect" is clicked. "Test Connection" validates
 * without saving.
 */
Popup {
    id: root

    required property var chatBackend
    readonly property var chatConfig: chatBackend.chatConfig

    width: Math.min(420, parent ? parent.width - 32 : 420)
    height: contentColumn.implicitHeight + 2 * padding
    padding: PluginTheme.gapWide
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    // Local form state — initialised from config on open
    property string formAuthMethod: "github"
    property string formProvider: "openai"
    property string formBaseUrl: ""
    property string formApiKey: ""
    property string formModel: ""
    property string formWireApi: "completions"

    // Test connection state
    property bool isTesting: false
    property string testResult: ""
    property bool testSuccess: false

    onOpened: {
        formAuthMethod = chatConfig.authMethod
        formProvider = chatConfig.byokProvider
        formBaseUrl = chatConfig.byokBaseUrl
        formApiKey = chatConfig.byokApiKey
        formModel = chatConfig.byokModel
        formWireApi = chatConfig.byokWireApi
        testResult = ""
        isTesting = false
    }

    // Handle testConnectionResult signal
    Connections {
        target: root.chatBackend
        function onTestConnectionResult(success, errorMessage) {
            root.isTesting = false
            root.testSuccess = success
            root.testResult = success
                ? qsTr("Connection successful")
                : errorMessage
        }
    }

    background: Rectangle {
        color: PluginTheme.surface
        radius: PluginTheme.radiusMedium
        border.width: 1
        border.color: PluginTheme.borderSubtle
    }

    contentItem: ColumnLayout {
        id: contentColumn
        spacing: PluginTheme.gap

        // ── Title ───────────────────────────────────────────────
        Label {
            text: qsTr("Authentication Settings")
            font.pixelSize: 16
            font.bold: true
            color: PluginTheme.textPrimary
        }

        // ── Auth method radio ───────────────────────────────────
        Label {
            text: qsTr("Auth Method:")
            font.pixelSize: 12
            color: PluginTheme.textSecondary
        }

        RowLayout {
            spacing: PluginTheme.gapWide

            RadioButton {
                id: githubRadio
                text: qsTr("GitHub Copilot")
                checked: root.formAuthMethod === "github"
                onToggled: {
                    if (checked) root.formAuthMethod = "github"
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 12
                    color: PluginTheme.textPrimary
                    leftPadding: parent.indicator.width + 4
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RadioButton {
                id: byokRadio
                text: qsTr("BYOK")
                checked: root.formAuthMethod === "byok"
                onToggled: {
                    if (checked) root.formAuthMethod = "byok"
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 12
                    color: PluginTheme.textPrimary
                    leftPadding: parent.indicator.width + 4
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // ── BYOK settings section ───────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: PluginTheme.borderSubtle
        }

        GridLayout {
            id: byokGrid
            Layout.fillWidth: true
            columns: 2
            columnSpacing: PluginTheme.gapTight
            rowSpacing: PluginTheme.gapTight
            enabled: root.formAuthMethod === "byok"
            opacity: enabled ? 1.0 : 0.4

            // Provider
            Label {
                text: qsTr("Provider:")
                font.pixelSize: 12
                color: PluginTheme.textSecondary
            }
            ComboBox {
                id: providerCombo
                Layout.fillWidth: true
                model: ["openai", "azure", "anthropic", "ollama", "custom"]
                currentIndex: Math.max(0, model.indexOf(root.formProvider))
                onCurrentTextChanged: {
                    root.formProvider = currentText
                    // Pre-fill base URL for Ollama
                    if (currentText === "ollama" && root.formBaseUrl === "") {
                        root.formBaseUrl = "http://localhost:11434/v1"
                    }
                }

                contentItem: Text {
                    text: providerCombo.currentText
                    font.pixelSize: 12
                    color: PluginTheme.textPrimary
                    leftPadding: PluginTheme.gapTight
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitHeight: 30
                    radius: 6
                    color: PluginTheme.surfaceStrong
                    border.width: 1
                    border.color: PluginTheme.borderSubtle
                }
            }

            // Base URL
            Label {
                text: qsTr("Base URL:")
                font.pixelSize: 12
                color: PluginTheme.textSecondary
            }
            TextField {
                id: baseUrlField
                Layout.fillWidth: true
                text: root.formBaseUrl
                onTextChanged: root.formBaseUrl = text
                font.pixelSize: 12
                font.family: PluginTheme.monoFont
                color: PluginTheme.textPrimary
                placeholderText: "https://api.openai.com/v1"
                placeholderTextColor: PluginTheme.textTertiary

                background: Rectangle {
                    implicitHeight: 30
                    radius: 6
                    color: PluginTheme.surfaceStrong
                    border.width: 1
                    border.color: baseUrlField.activeFocus
                                  ? PluginTheme.accentA
                                  : PluginTheme.borderSubtle
                }
            }

            // API Key
            Label {
                text: qsTr("API Key:")
                font.pixelSize: 12
                color: PluginTheme.textSecondary
            }
            TextField {
                id: apiKeyField
                Layout.fillWidth: true
                text: root.formApiKey
                onTextChanged: root.formApiKey = text
                echoMode: TextInput.Password
                font.pixelSize: 12
                color: PluginTheme.textPrimary
                placeholderText: qsTr("sk-...")
                placeholderTextColor: PluginTheme.textTertiary

                background: Rectangle {
                    implicitHeight: 30
                    radius: 6
                    color: PluginTheme.surfaceStrong
                    border.width: 1
                    border.color: apiKeyField.activeFocus
                                  ? PluginTheme.accentA
                                  : PluginTheme.borderSubtle
                }
            }

            // Model
            Label {
                text: qsTr("Model:")
                font.pixelSize: 12
                color: PluginTheme.textSecondary
            }
            TextField {
                id: modelField
                Layout.fillWidth: true
                text: root.formModel
                onTextChanged: root.formModel = text
                font.pixelSize: 12
                font.family: PluginTheme.monoFont
                color: PluginTheme.textPrimary
                placeholderText: "gpt-4o"
                placeholderTextColor: PluginTheme.textTertiary

                background: Rectangle {
                    implicitHeight: 30
                    radius: 6
                    color: PluginTheme.surfaceStrong
                    border.width: 1
                    border.color: modelField.activeFocus
                                  ? PluginTheme.accentA
                                  : PluginTheme.borderSubtle
                }
            }

            // Wire API (only for openai/azure)
            Label {
                text: qsTr("Wire API:")
                font.pixelSize: 12
                color: PluginTheme.textSecondary
                visible: root.formProvider === "openai"
                         || root.formProvider === "azure"
            }
            ComboBox {
                id: wireApiCombo
                Layout.fillWidth: true
                visible: root.formProvider === "openai"
                         || root.formProvider === "azure"
                model: ["completions", "responses"]
                currentIndex: Math.max(
                    0, model.indexOf(root.formWireApi),
                )
                onCurrentTextChanged: root.formWireApi = currentText

                contentItem: Text {
                    text: wireApiCombo.currentText
                    font.pixelSize: 12
                    color: PluginTheme.textPrimary
                    leftPadding: PluginTheme.gapTight
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitHeight: 30
                    radius: 6
                    color: PluginTheme.surfaceStrong
                    border.width: 1
                    border.color: PluginTheme.borderSubtle
                }
            }
        }

        // ── Test Connection ─────────────────────────────────────
        RowLayout {
            spacing: PluginTheme.gapTight
            enabled: root.formAuthMethod === "byok"
            opacity: enabled ? 1.0 : 0.4

            Button {
                text: root.isTesting ? qsTr("Testing...") : qsTr("Test Connection")
                enabled: !root.isTesting
                         && root.formBaseUrl.length > 0

                onClicked: {
                    root.isTesting = true
                    root.testResult = ""
                    root.chatBackend.testConnection(JSON.stringify({
                        provider: root.formProvider,
                        base_url: root.formBaseUrl,
                        api_key: root.formApiKey,
                    }))
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 12
                    color: PluginTheme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 30
                    implicitWidth: 120
                    radius: 6
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : PluginTheme.surfaceMuted
                    border.width: 1
                    border.color: PluginTheme.borderSubtle
                }
            }

            BusyIndicator {
                running: root.isTesting
                visible: root.isTesting
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
            }
        }

        // Test result message
        Label {
            visible: root.testResult.length > 0
            text: root.testResult
            font.pixelSize: 11
            color: root.testSuccess
                   ? PluginTheme.success
                   : PluginTheme.danger
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        // ── Action buttons ──────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: PluginTheme.borderSubtle
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: PluginTheme.gapTight

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancel")
                onClicked: root.close()

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 12
                    color: PluginTheme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 80
                    radius: 6
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : PluginTheme.surfaceMuted
                    border.width: 1
                    border.color: PluginTheme.borderSubtle
                }
            }

            Button {
                text: qsTr("Save & Reconnect")
                enabled: root.formAuthMethod === "github"
                         || root.formModel.trim().length > 0

                onClicked: {
                    // Write form values to config (triggers auto-save)
                    chatConfig.authMethod = root.formAuthMethod
                    if (root.formAuthMethod === "byok") {
                        chatConfig.byokProvider = root.formProvider
                        chatConfig.byokBaseUrl = root.formBaseUrl
                        chatConfig.byokApiKey = root.formApiKey
                        chatConfig.byokModel = root.formModel
                        chatConfig.byokWireApi = root.formWireApi
                    }
                    root.chatBackend.newSession()
                    root.close()
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 12
                    color: PluginTheme.textOnAccent
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 140
                    radius: 6
                    color: parent.enabled
                           ? (parent.down
                              ? Qt.darker(PluginTheme.accentA, 1.2)
                              : PluginTheme.accentA)
                           : PluginTheme.surfaceMuted
                    border.width: parent.enabled ? 0 : 1
                    border.color: PluginTheme.borderSubtle
                }
            }
        }

        // ── BYOK empty-model tooltip ────────────────────────────
        Label {
            visible: root.formAuthMethod === "byok"
                     && root.formModel.trim().length === 0
            text: qsTr("Model name is required for BYOK providers.")
            font.pixelSize: 10
            font.italic: true
            color: PluginTheme.warning
        }
    }
}
```

- [ ] **Step 2: Verify file loads (syntax check)**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -c "
from pathlib import Path
p = Path('plugins/ai_chat_plugin/qml/AuthSettingsPanel.qml')
text = p.read_text(encoding='utf-8')
print(f'OK: {len(text)} chars, {text.count(chr(10))} lines')
"
```

- [ ] **Step 3: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/qml/AuthSettingsPanel.qml
git commit -m "feat(ai-chat): add AuthSettingsPanel QML popup

Overlay Popup with GitHub/BYOK radio toggle, BYOK provider fields
(provider, base_url, api_key, model, wire_api), Test Connection
button, and Save & Reconnect / Cancel actions. All strings qsTr().

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: ChatPage.qml — Insert ModelSelectorBar

**Files:**
- Modify: `plugins/ai_chat_plugin/qml/ChatPage.qml`

- [ ] **Step 1: Add ModelSelectorBar between streaming indicator and input area**

In `ChatPage.qml`, insert the `ModelSelectorBar` between the streaming indicator `Rectangle` (line 189-204) and the `ChatInputArea` (line 207-216).

After:

```qml
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
```

Insert:

```qml
        // ── Model selector bar ──────────────────────────────────────
        ModelSelectorBar {
            Layout.fillWidth: true
            Layout.leftMargin: PluginTheme.gapTight
            Layout.rightMargin: PluginTheme.gapTight
            chatBackend: root.chatBackend
        }
```

Before:

```qml
        // ── Input area ──────────────────────────────────────────────
        ChatInputArea {
```

- [ ] **Step 2: Verify QML file is valid**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -c "
from pathlib import Path
text = Path('plugins/ai_chat_plugin/qml/ChatPage.qml').read_text('utf-8')
assert 'ModelSelectorBar' in text
assert text.count('ModelSelectorBar') >= 1
print('OK: ModelSelectorBar inserted')
"
```

- [ ] **Step 3: Commit**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
git add plugins/ai_chat_plugin/qml/ChatPage.qml
git commit -m "feat(ai-chat): insert ModelSelectorBar into ChatPage layout

Model selector bar with auth status indicator now appears between
the streaming indicator and input area.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 9: Integration Verification

**Files:** None (read-only verification).

- [ ] **Step 1: Run full Python test suite**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
.\pyvenv\Scripts\python.exe -m pytest plugins/ai_chat_plugin/tests/ -v
```

Expected: 39 passed (8 ChatConfig + 12 model + 10 markdown + 9 tools).

- [ ] **Step 2: Sync plugin to build directory**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab
if (Test-Path build\bin\plugins\ai_chat_plugin) {
    Remove-Item -Recurse -Force build\bin\plugins\ai_chat_plugin
}
Copy-Item -Path "plugins\ai_chat_plugin" -Destination "build\bin\plugins\ai_chat_plugin" -Recurse -Force
```

- [ ] **Step 3: Standalone smoke test**

```powershell
cd D:\WorkSpace\OGLWorkSpace\OpenGeoLab\build\bin\plugins
Start-Process -FilePath "D:\WorkSpace\OGLWorkSpace\OpenGeoLab\pyvenv\Scripts\python.exe" -ArgumentList "-m", "ai_chat_plugin" -PassThru
```

**Manual checks (if window opens):**
1. ModelSelectorBar visible at bottom of Chat tab (above input area).
2. Auth status indicator shows "Connecting..." then "Connected" (or "Error" if no GitHub auth).
3. Click the status indicator → AuthSettingsPanel popup opens.
4. Radio buttons toggle between GitHub and BYOK.
5. BYOK fields disable/dim when GitHub is selected.
6. Close popup with Cancel → no config change.
7. Close the window — process should exit cleanly.

- [ ] **Step 4: Verify config file creation**

After interacting with the AuthSettingsPanel (save a BYOK config and then switch back to GitHub):

```powershell
if (Test-Path "$env:USERPROFILE\.opengeolab\ai_chat_config.json") {
    Get-Content "$env:USERPROFILE\.opengeolab\ai_chat_config.json"
    Write-Host "`nConfig file exists and is readable."
} else {
    Write-Host "Config file not found (expected if auth panel was not saved)."
}
```

- [ ] **Step 5: Kill standalone process**

```powershell
Stop-Process -Id <PID_FROM_STEP_3>
```
