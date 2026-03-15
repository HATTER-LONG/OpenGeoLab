pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var appController
    property bool open: false
    property bool filterOpen: false
    property int currentTab: 0
    property int pendingCommandLineRequestId: -1
    property bool pendingPythonProcessResponse: false
    property bool suppressAutoServiceExchange: false
    property bool suppressNextPythonOutput: false
    property real availableHeight: 760
    property string commandLineText: ""
    readonly property var logService: root.appController ? root.appController.operationLogService : null
    readonly property int enabledLevelMask: root.logService ? root.logService.enabledLevelMask : 0x3F
    readonly property int runtimeMinLevel: root.logService ? root.logService.minLevel : 2
    readonly property int terminalEntryLimit: 160
    readonly property color terminalShellColor: root.theme.darkMode ? "#0b1620" : "#f6f8fb"
    readonly property color terminalBodyColor: root.theme.darkMode ? "#071018" : "#ffffff"
    readonly property color terminalBorderColor: root.theme.darkMode ? "#223341" : "#cfd7e3"
    readonly property color terminalTextColor: root.theme.darkMode ? "#d7e6f6" : "#162230"
    readonly property color terminalMutedTextColor: root.theme.darkMode ? "#7f98b0" : "#66788b"
    readonly property color terminalPromptColor: root.theme.darkMode ? "#6fd6ff" : "#0c6ee6"
    readonly property color terminalThumbColor: root.theme.darkMode ? "#4b6984" : "#aac2d8"
    readonly property color commandEntryColor: root.theme.darkMode ? "#86f08d" : "#1e7f34"
    readonly property color responseEntryColor: root.theme.darkMode ? "#ffbf73" : "#b65a00"
    readonly property var levelOptions: [
        { "level": 0, "label": qsTr("Trace") },
        { "level": 1, "label": qsTr("Debug") },
        { "level": 2, "label": qsTr("Info") },
        { "level": 3, "label": qsTr("Warn") },
        { "level": 4, "label": qsTr("Error") },
        { "level": 5, "label": qsTr("Critical") }
    ]
    readonly property var tabs: [
        { "title": qsTr("Events") },
        { "title": qsTr("Command Line") }
    ]
    readonly property int minimumHeight: 360
    readonly property int maximumHeight: Math.max(minimumHeight, Math.min(Math.floor(availableHeight), 760))
    readonly property int logHeight: Math.min(maximumHeight, Math.max(minimumHeight,
        listView.contentHeight + (filterOpen ? filterPanel.implicitHeight + 208 : 176)))
    readonly property int commandHeight: maximumHeight
    readonly property int resolvedHeight: maximumHeight
    readonly property string terminalEmptyText: qsTr(">>> opengeolab_app.process({...})\n<<< no response captured yet.\n\nPress Ctrl+Enter to run multiline input.")
    signal requestClose

    function levelTint(level) {
        if (level >= 4) {
            return root.theme.accentD;
        }
        if (level === 3) {
            return root.theme.accentC;
        }
        if (level === 2) {
            return root.theme.accentB;
        }
        return root.theme.accentA;
    }

    function levelLabel(level) {
        for (let index = 0; index < root.levelOptions.length; ++index) {
            if (root.levelOptions[index].level === level) {
                return root.levelOptions[index].label;
            }
        }
        return qsTr("Info");
    }

    function levelVisible(level) {
        return ((root.enabledLevelMask >> level) & 1) !== 0;
    }

    function runCommandLine() {
        if (!root.appController) {
            return;
        }
        const commandText = root.commandLineText;
        const trimmedCommand = commandText.trim();
        if (trimmedCommand.length === 0) {
            return;
        }
        if (trimmedCommand.startsWith("{") || trimmedCommand.startsWith("[")) {
            try {
                JSON.parse(trimmedCommand);
                const requestId = root.appController.submitServiceRequest(trimmedCommand);
                if (requestId > 0) {
                    root.pendingCommandLineRequestId = requestId;
                    root.appendTerminalEntry("command", root.formatServiceRequest(trimmedCommand));
                    root.commandLineText = "";
                }
                return;
            } catch (error) {
            }
        }

        root.appendTerminalEntry("command", root.formatPythonCommand(commandText));
        root.commandLineText = "";
        root.pendingPythonProcessResponse = trimmedCommand.indexOf("process(") !== -1;
        root.suppressNextPythonOutput = root.pendingPythonProcessResponse;
        root.suppressAutoServiceExchange = !root.pendingPythonProcessResponse;
        root.appController.runEmbeddedPythonCommandLine(commandText);
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

    function appendTerminalEntry(kind, text) {
        if (!text || text.length === 0) {
            return;
        }
        terminalEntries.append({
            "kind": kind,
            "body": text
        });
        while (terminalEntries.count > root.terminalEntryLimit) {
            terminalEntries.remove(0);
        }
        root.scrollTerminalToEnd();
    }

    function appendLatestServiceExchange() {
        if (!root.appController || !root.appController.lastResponse || root.appController.lastResponse.length === 0) {
            return;
        }
        if (root.appController.lastRequest && root.appController.lastRequest.length > 0) {
            root.appendTerminalEntry("command", root.formatServiceRequest(root.appController.lastRequest));
        }
        root.appendTerminalEntry("response", root.formatServiceResponse(root.appController.lastResponse));
    }

    function appendLatestPythonOutput() {
        if (!root.appController || !root.appController.lastPythonOutput || root.appController.lastPythonOutput.length === 0) {
            return;
        }
        root.appendTerminalEntry("response", root.formatPythonResponse(root.appController.lastPythonOutput));
    }

    function scrollTerminalToEnd() {
        Qt.callLater(function () {
            if (!terminalFlickable) {
                return;
            }
            terminalFlickable.contentY = Math.max(0, terminalFlickable.contentHeight - terminalFlickable.height);
        });
    }

    function seedTerminalFromState() {
        if (terminalEntries.count > 0 || !root.appController) {
            return;
        }
        if (root.appController.lastResponse && root.appController.lastResponse.length > 0) {
            root.appendLatestServiceExchange();
        }
        if (root.appController.lastPythonOutput && root.appController.lastPythonOutput.length > 0) {
            root.appendLatestPythonOutput();
        }
    }

    implicitWidth: 840
    height: resolvedHeight
    visible: open || opacity > 0.01
    opacity: open ? 1 : 0
    y: open ? 0 : 10

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Behavior on y {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    onOpenChanged: {
        if (open && logService) {
            logService.markAllSeen();
            Qt.callLater(function () {
                if (listView.count > 0 && root.currentTab === 0) {
                    listView.positionViewAtEnd();
                }
            });
        }
        if (!open) {
            filterOpen = false;
        }
    }

    onCurrentTabChanged: {
        if (root.currentTab === 0 && root.open && listView.count > 0) {
            Qt.callLater(function () {
                listView.positionViewAtEnd();
            });
        }
        if (root.currentTab === 1) {
            root.scrollTerminalToEnd();
        }
    }

    onCommandLineTextChanged: {
        if (commandLineEditor && commandLineEditor.text !== root.commandLineText) {
            commandLineEditor.text = root.commandLineText;
        }
    }

    Connections {
        target: listView.model

        function onRowsInserted() {
            if (root.open && root.currentTab === 0 && listView.count > 0) {
                listView.positionViewAtEnd();
            }
        }
    }

    Connections {
        target: root.appController

        function onLastResponseChanged() {
            if (root.pendingCommandLineRequestId > 0) {
                return;
            }
            if (root.pendingPythonProcessResponse) {
                root.pendingPythonProcessResponse = false;
                root.appendTerminalEntry("response", root.formatServiceResponse(root.appController.lastResponse));
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
            root.appendTerminalEntry("response", root.formatServiceResponse(root.appController.lastResponse));
        }
    }

    Component.onCompleted: root.seedTerminalFromState()

    ListModel {
        id: terminalEntries
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.97 : 1.0)
        border.width: 1
        border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.82 : 0.94)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("Activity Center")
                color: root.theme.textPrimary
                font.pixelSize: 14
                font.bold: true
                font.family: root.theme.titleFontFamily
                Layout.fillWidth: true
            }

            StatChip {
                visible: root.appController && root.appController.hasUnreadOperationErrors
                theme: root.theme
                text: qsTr("New errors")
                tintColor: root.theme.accentD
            }

            StatChip {
                visible: root.appController && !root.appController.hasUnreadOperationErrors && root.appController.hasUnreadOperationLogs
                theme: root.theme
                text: qsTr("New activity")
                tintColor: root.theme.accentB
            }

            Rectangle {
                implicitWidth: 28
                implicitHeight: 28
                radius: root.theme.radiusSmall
                color: closeMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                             : (closeMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : "transparent")
                border.width: closeMouseArea.containsMouse ? 1 : 0
                border.color: root.theme.tint(root.theme.borderSubtle, 0.82)

                Text {
                    anchors.centerIn: parent
                    text: "\u00d7"
                    color: root.theme.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                    font.family: root.theme.bodyFontFamily
                }

                MouseArea {
                    id: closeMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestClose()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: root.tabs

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: root.theme.radiusSmall
                    color: root.currentTab === index
                        ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.24 : 0.12)
                        : root.theme.surfaceMuted
                    border.width: 1
                    border.color: root.currentTab === index
                        ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.56 : 0.3)
                        : root.theme.tint(root.theme.borderSubtle, 0.75)

                    Text {
                        anchors.centerIn: parent
                        text: modelData.title
                        color: root.theme.textPrimary
                        font.pixelSize: 11
                        font.bold: root.currentTab === index
                        font.family: root.theme.bodyFontFamily
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentTab = index
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentTab

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        StatChip {
                            theme: root.theme
                            text: qsTr("Print ≥ %1").arg(root.levelLabel(root.runtimeMinLevel))
                            tintColor: root.levelTint(root.runtimeMinLevel)
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            id: filterButton

                            implicitWidth: 72
                            implicitHeight: 28
                            radius: root.theme.radiusSmall
                            color: filterMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                                           : (filterMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                            border.width: 1
                            border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                            Text {
                                anchors.centerIn: parent
                                text: root.filterOpen ? qsTr("Levels ▲") : qsTr("Levels ▼")
                                color: root.theme.textPrimary
                                font.pixelSize: 11
                                font.bold: true
                                font.family: root.theme.bodyFontFamily
                            }

                            MouseArea {
                                id: filterMouseArea

                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.filterOpen = !root.filterOpen
                            }
                        }

                        Rectangle {
                            implicitWidth: 54
                            implicitHeight: 28
                            radius: root.theme.radiusSmall
                            color: clearMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                                          : (clearMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                            border.width: 1
                            border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Clear")
                                color: root.theme.textPrimary
                                font.pixelSize: 11
                                font.bold: true
                                font.family: root.theme.bodyFontFamily
                            }

                            MouseArea {
                                id: clearMouseArea

                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (root.logService) {
                                        root.logService.clear();
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: filterPanel

                        visible: root.filterOpen
                        Layout.fillWidth: true
                        radius: root.theme.radiusSmall
                        color: root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.97)
                        border.width: 1
                        border.color: root.theme.tint(root.theme.borderSubtle, 0.8)
                        implicitHeight: filterColumn.implicitHeight + 16

                        Column {
                            id: filterColumn

                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            Text {
                                text: qsTr("spdlog output level")
                                color: root.theme.textSecondary
                                font.pixelSize: 11
                                font.bold: true
                                font.family: root.theme.bodyFontFamily
                            }

                            Text {
                                width: parent.width
                                text: qsTr("This controls which new log messages are emitted by the runtime logger pipeline.")
                                wrapMode: Text.WordWrap
                                color: root.theme.textTertiary
                                font.pixelSize: 10
                                font.family: root.theme.bodyFontFamily
                            }

                            Flow {
                                width: parent.width
                                spacing: 8

                                Repeater {
                                    model: root.levelOptions

                                    delegate: OperationLogLevelChip {
                                        required property var modelData

                                        theme: root.theme
                                        text: modelData.label
                                        accentColor: root.levelTint(modelData.level)
                                        selected: root.runtimeMinLevel === modelData.level
                                        onClicked: {
                                            if (root.logService) {
                                                root.logService.setMinLevel(modelData.level);
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 1
                                color: root.theme.tint(root.theme.borderSubtle, 0.82)
                            }

                            Text {
                                text: qsTr("Visible log entries")
                                color: root.theme.textSecondary
                                font.pixelSize: 11
                                font.bold: true
                                font.family: root.theme.bodyFontFamily
                            }

                            Text {
                                width: parent.width
                                text: qsTr("This only filters the entries already captured inside the panel.")
                                wrapMode: Text.WordWrap
                                color: root.theme.textTertiary
                                font.pixelSize: 10
                                font.family: root.theme.bodyFontFamily
                            }

                            Flow {
                                width: parent.width
                                spacing: 8

                                Repeater {
                                    model: root.levelOptions

                                    delegate: OperationLogLevelChip {
                                        required property var modelData

                                        readonly property bool selectedLevel: root.levelVisible(modelData.level)

                                        theme: root.theme
                                        text: modelData.label
                                        accentColor: root.levelTint(modelData.level)
                                        selected: selectedLevel
                                        onClicked: {
                                            if (root.logService) {
                                                root.logService.setLevelEnabled(modelData.level, !selectedLevel);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: root.theme.radiusSmall
                        color: root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.78 : 0.96)
                        border.width: 1
                        border.color: root.theme.tint(root.theme.borderSubtle, 0.78)
                        clip: true

                        ListView {
                            id: listView

                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8
                            clip: true
                            model: root.logService ? root.logService.model : null
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: OperationLogEntryDelegate {
                                width: listView.width
                                theme: root.theme
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: listView.count === 0
                            text: qsTr("No activity matches the current log view.")
                            color: root.theme.textTertiary
                            font.pixelSize: 12
                            font.family: root.theme.bodyFontFamily
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Rectangle {
                    anchors.fill: parent
                    radius: root.theme.radiusMedium
                    color: root.terminalShellColor
                    border.width: 1
                    border.color: root.terminalBorderColor

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: root.terminalBodyColor
                            border.width: 1
                            border.color: root.theme.tint(root.terminalBorderColor, root.theme.darkMode ? 0.9 : 1.0)
                            clip: true

                            Flickable {
                                id: terminalFlickable

                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 18
                                anchors.topMargin: 12
                                anchors.bottomMargin: 12
                                contentWidth: width
                                contentHeight: Math.max(height, terminalColumn.implicitHeight)
                                clip: true

                                Column {
                                    id: terminalColumn

                                    width: parent.width
                                    spacing: 6

                                    Repeater {
                                        model: terminalEntries

                                        delegate: TextEdit {
                                            required property string kind
                                            required property string body

                                            width: terminalColumn.width
                                            height: contentHeight
                                            readOnly: true
                                            selectByMouse: true
                                            textFormat: TextEdit.PlainText
                                            wrapMode: TextEdit.WrapAnywhere
                                            color: kind === "command" ? root.commandEntryColor : root.responseEntryColor
                                            text: body
                                            font.pixelSize: 12
                                            font.family: root.theme.monoFontFamily
                                        }
                                    }

                                    TextEdit {
                                        visible: terminalEntries.count === 0
                                        width: terminalColumn.width
                                        height: contentHeight
                                        readOnly: true
                                        textFormat: TextEdit.PlainText
                                        wrapMode: TextEdit.WrapAnywhere
                                        color: root.terminalMutedTextColor
                                        text: root.terminalEmptyText
                                        font.pixelSize: 12
                                        font.family: root.theme.monoFontFamily
                                    }
                                }
                            }

                            Rectangle {
                                id: terminalScrollTrack

                                anchors.top: parent.top
                                anchors.topMargin: 10
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 10
                                anchors.right: parent.right
                                anchors.rightMargin: 6
                                width: 6
                                radius: 3
                                color: root.theme.darkMode ? "#0f1b27" : "#eef3f8"
                                visible: terminalFlickable.contentHeight > terminalFlickable.height + 1

                                Rectangle {
                                    id: terminalScrollThumb

                                    width: parent.width
                                    radius: width / 2
                                    color: root.terminalThumbColor
                                    y: terminalFlickable.visibleArea.yPosition * parent.height
                                    height: Math.max(26, terminalFlickable.visibleArea.heightRatio * parent.height)

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                                        drag.target: parent
                                        drag.axis: Drag.YAxis
                                        drag.minimumY: 0
                                        drag.maximumY: Math.max(0, terminalScrollTrack.height - parent.height)
                                        onPositionChanged: {
                                            if (!drag.active) {
                                                return;
                                            }
                                            const denominator = Math.max(1, terminalScrollTrack.height - parent.height);
                                            const ratio = parent.y / denominator;
                                            terminalFlickable.contentY = ratio * Math.max(0, terminalFlickable.contentHeight - terminalFlickable.height);
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            id: inputBar

                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(156, Math.max(58, commandLineEditor.contentHeight + 22))
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
                                            text: root.commandLineText
                                            font.pixelSize: 12
                                            font.family: root.theme.monoFontFamily
                                            selectByMouse: true
                                            wrapMode: TextEdit.WrapAnywhere
                                            onTextChanged: {
                                                if (root.commandLineText !== text) {
                                                    root.commandLineText = text;
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
        }
    }
}
