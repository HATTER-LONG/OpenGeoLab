pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    property var appController: null
    property bool panelActive: false
    property int pendingCommandLineRequestId: -1
    property bool pendingPythonProcessResponse: false
    property bool suppressAutoServiceExchange: false
    property bool suppressNextPythonOutput: false
    property string commandText: ""
    readonly property int terminalEntryLimit: 160
    readonly property color terminalShellColor: root.theme.darkMode ? "#0b1620" : "#f6f8fb"
    readonly property color terminalBodyColor: root.theme.darkMode ? "#071018" : "#ffffff"
    readonly property color terminalBorderColor: root.theme.darkMode ? "#223341" : "#cfd7e3"
    readonly property color terminalMutedTextColor: root.theme.darkMode ? "#7f98b0" : "#66788b"
    readonly property color terminalPromptColor: root.theme.darkMode ? "#6fd6ff" : "#0c6ee6"
    readonly property color terminalThumbColor: root.theme.darkMode ? "#4b6984" : "#aac2d8"
    readonly property color commandEntryColor: root.theme.darkMode ? "#86f08d" : "#1e7f34"
    readonly property color responseEntryColor: root.theme.darkMode ? "#ffbf73" : "#b65a00"
    readonly property string terminalEmptyText: qsTr(">>> opengeolab_app.process({...})\n<<< no response captured yet.\n\nPress Ctrl+Enter to run multiline input.")
    readonly property int transcriptCount: transcriptEntries.count

    function transcriptEntryBody(index) {
        if (index < 0 || index >= transcriptEntries.count) {
            return "";
        }
        return transcriptEntries.get(index).body;
    }

    function runCommandLine() {
        if (!root.appController) {
            return;
        }
        const sourceText = root.commandText;
        const trimmedCommand = sourceText.trim();
        if (trimmedCommand.length === 0) {
            return;
        }
        if (trimmedCommand.startsWith("{") || trimmedCommand.startsWith("[")) {
            try {
                JSON.parse(trimmedCommand);
                const requestId = root.appController.submitServiceRequest(trimmedCommand);
                if (requestId > 0) {
                    root.pendingCommandLineRequestId = requestId;
                    root.appendTranscriptEntry("command", root.formatServiceRequest(trimmedCommand));
                    root.commandText = "";
                }
                return;
            } catch (error) {
            }
        }

        root.appendTranscriptEntry("command", root.formatPythonCommand(sourceText));
        root.commandText = "";
        root.pendingPythonProcessResponse = trimmedCommand.indexOf("process(") !== -1;
        root.suppressNextPythonOutput = root.pendingPythonProcessResponse;
        root.suppressAutoServiceExchange = !root.pendingPythonProcessResponse;
        root.appController.runEmbeddedPythonCommandLine(sourceText);
        if (root.pendingPythonProcessResponse) {
            root.pendingPythonProcessResponse = false;
            root.suppressNextPythonOutput = false;
        }
        root.suppressAutoServiceExchange = false;
    }

    function formatServiceRequest(requestText) {
        let requestBody = requestText;
        try {
            requestBody = JSON.stringify(JSON.parse(requestText), null, 2);
        } catch (error) {
        }
        return ">>> opengeolab_app.process(\n" + requestBody + "\n)";
    }

    function formatPythonCommand(commandText) {
        const normalizedCommand = commandText.replace(/\s+$/, "");
        const lines = normalizedCommand.split(/\r?\n/);
        if (lines.length === 0) {
            return ">>>";
        }
        return ">>> " + lines[0] + (lines.length > 1 ? "\n" + lines.slice(1).join("\n") : "");
    }

    function formatServiceResponse(responseText) {
        try {
            return "<<< " + JSON.stringify(JSON.parse(responseText), null, 2);
        } catch (error) {
            return "<<< " + responseText.replace(/\\n/g, "\n");
        }
    }

    function formatPythonResponse(outputText) {
        const normalizedText = outputText.replace(/\\n/g, "\n").trim();
        return normalizedText.length > 0 ? "<<< " + normalizedText : "<<<";
    }

    function appendTranscriptEntry(kind, text) {
        if (!text || text.length === 0) {
            return;
        }
        transcriptEntries.append({
            "kind": kind,
            "body": text
        });
        while (transcriptEntries.count > root.terminalEntryLimit) {
            transcriptEntries.remove(0);
        }
        transcriptView.scrollToEnd();
    }

    function appendLatestServiceExchange() {
        if (!root.appController || !root.appController.lastResponse || root.appController.lastResponse.length === 0) {
            return;
        }
        if (root.appController.lastRequest && root.appController.lastRequest.length > 0) {
            root.appendTranscriptEntry("command", root.formatServiceRequest(root.appController.lastRequest));
        }
        root.appendTranscriptEntry("response", root.formatServiceResponse(root.appController.lastResponse));
    }

    function appendLatestPythonOutput() {
        if (!root.appController || !root.appController.lastPythonOutput || root.appController.lastPythonOutput.length === 0) {
            return;
        }
        root.appendTranscriptEntry("response", root.formatPythonResponse(root.appController.lastPythonOutput));
    }

    function seedTerminalFromState() {
        if (transcriptEntries.count > 0 || !root.appController) {
            return;
        }
        if (root.appController.lastResponse && root.appController.lastResponse.length > 0) {
            root.appendLatestServiceExchange();
        }
        if (root.appController.lastPythonOutput && root.appController.lastPythonOutput.length > 0) {
            root.appendLatestPythonOutput();
        }
    }

    onPanelActiveChanged: {
        if (root.panelActive) {
            transcriptView.scrollToEnd();
        }
    }

    onAppControllerChanged: Qt.callLater(root.seedTerminalFromState)

    Connections {
        target: root.appController
        ignoreUnknownSignals: true

        function onLastResponseChanged() {
            if (root.pendingCommandLineRequestId > 0) {
                return;
            }
            if (root.pendingPythonProcessResponse) {
                root.pendingPythonProcessResponse = false;
                root.appendTranscriptEntry("response", root.formatServiceResponse(root.appController.lastResponse));
                return;
            }
            if (root.suppressAutoServiceExchange) {
                return;
            }
            root.appendLatestServiceExchange();
        }

        function onLastPythonOutputChanged() {
            if (root.suppressNextPythonOutput) {
                root.suppressNextPythonOutput = false;
                return;
            }
            root.appendLatestPythonOutput();
        }

        function onServiceRequestFinished(requestId, success) {
            if (requestId !== root.pendingCommandLineRequestId) {
                return;
            }
            root.pendingCommandLineRequestId = -1;
            root.appendTranscriptEntry("response", root.formatServiceResponse(root.appController.lastResponse));
        }
    }

    Component.onCompleted: root.seedTerminalFromState()

    ListModel {
        id: transcriptEntries
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.terminalShellColor
        border.width: 1
        border.color: root.terminalBorderColor

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            OperationTerminalTranscript {
                id: transcriptView

                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
                entriesModel: transcriptEntries
                emptyText: root.terminalEmptyText
                backgroundColor: root.terminalBodyColor
                borderColor: root.terminalBorderColor
                mutedTextColor: root.terminalMutedTextColor
                commandTextColor: root.commandEntryColor
                responseTextColor: root.responseEntryColor
                thumbColor: root.terminalThumbColor
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(148, Math.max(56, commandLineEditor.contentHeight + 20))
                color: root.theme.darkMode ? "#0f1b27" : "#eef3f8"
                border.width: 1
                border.color: root.theme.tint(root.terminalBorderColor, root.theme.darkMode ? 0.9 : 1.0)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Text {
                        text: ">"
                        color: root.terminalPromptColor
                        font.pixelSize: 13
                        font.bold: true
                        font.family: root.theme.monoFontFamily
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        Flickable {
                            id: commandLineFlickable

                            anchors.fill: parent
                            contentWidth: width
                            contentHeight: Math.max(height, commandLineEditor.y + commandLineEditor.contentHeight)
                            boundsBehavior: Flickable.StopAtBounds
                            clip: true

                            TextEdit {
                                id: commandLineEditor

                                width: commandLineFlickable.width
                                y: Math.max(0, (commandLineFlickable.height - contentHeight) / 2)
                                height: contentHeight
                                color: root.commandEntryColor
                                text: root.commandText
                                font.pixelSize: 12
                                font.family: root.theme.monoFontFamily
                                selectByMouse: true
                                wrapMode: TextEdit.WrapAnywhere
                                onTextChanged: {
                                    if (root.commandText !== text) {
                                        root.commandText = text;
                                    }
                                }
                                Keys.onPressed: function (event) {
                                    const isEnter = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                                    if (isEnter && (event.modifiers & Qt.ControlModifier)) {
                                        root.runCommandLine();
                                        event.accepted = true;
                                    }
                                }
                            }
                        }

                        Text {
                            visible: commandLineEditor.length === 0 && !commandLineEditor.activeFocus
                            text: qsTr("Paste JSON or Python here. Press Ctrl+Enter to run.")
                            color: root.terminalMutedTextColor
                            font.pixelSize: 12
                            font.family: root.theme.monoFontFamily
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }
    }
}
