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
