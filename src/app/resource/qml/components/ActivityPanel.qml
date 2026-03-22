pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    required property AppTheme theme
    property int currentTab: 0
    property real availableHeight: 760
    signal closeRequested

    readonly property int minimumHeight: 360
    readonly property int maximumHeight: Math.max(minimumHeight, Math.min(Math.floor(availableHeight), 760))
    readonly property int resolvedHeight: maximumHeight

    implicitWidth: 460
    height: resolvedHeight
    implicitHeight: resolvedHeight

    Rectangle {
        id: shadowRect
        anchors.fill: panelBody
        anchors.margins: -2
        radius: panelBody.radius + 2
        color: root.theme.tint(root.theme.shell, root.theme.darkMode ? 0.18 : 0.08)
    }

    Rectangle {
        id: panelBody
        anchors.fill: parent
        radius: root.theme.radiusLarge
        color: root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.96 : 0.98)
        border.width: 1
        border.color: root.theme.borderSubtle
        clip: true
    }

    ListModel { id: mockLogModel }
    ListModel { id: mockTerminalModel }

    function appendTerminalEntry(type: string, text: string): void {
        mockTerminalModel.append({ type: type, text: text });
        if (mockTerminalModel.count > 160) {
            mockTerminalModel.remove(0);
        }
    }

    function runCommand(text: string): void {
        appendTerminalEntry("command", text);
        responseTimer.text = text;
        responseTimer.start();
    }

    Component.onCompleted: {
        const samples = [
            { level: 4, levelName: qsTr("ERROR"), source: "GeometryKernel", message: qsTr("Boolean operation failed: self-intersecting input"), time: "14:32:07", threadId: 1024, file: "boolean_op.cpp", line: 342 },
            { level: 2, levelName: qsTr("INFO"), source: "SceneManager", message: qsTr("Scene loaded successfully (12 objects)"), time: "14:32:05", threadId: 1, file: "", line: 0 },
            { level: 3, levelName: qsTr("WARN"), source: "MeshGenerator", message: qsTr("Degenerate triangle detected, skipping face #847"), time: "14:32:04", threadId: 2048, file: "mesh_gen.cpp", line: 156 },
            { level: 1, levelName: qsTr("DEBUG"), source: "RenderPipeline", message: qsTr("Frame buffer resized to 1920x1080"), time: "14:32:03", threadId: 1, file: "", line: 0 },
            { level: 2, levelName: qsTr("INFO"), source: "PluginLoader", message: qsTr("Loaded 3 plugins: geometry, mesh, export"), time: "14:32:01", threadId: 1, file: "", line: 0 },
            { level: 0, levelName: qsTr("TRACE"), source: "EventLoop", message: qsTr("Processing 42 pending events"), time: "14:31:58", threadId: 1, file: "event_loop.cpp", line: 89 },
            { level: 5, levelName: qsTr("CRITICAL"), source: "MemoryPool", message: qsTr("Allocation failed: out of memory (requested 2.1 GB)"), time: "14:31:55", threadId: 4096, file: "memory_pool.cpp", line: 67 },
            { level: 2, levelName: qsTr("INFO"), source: "CommandRecorder", message: qsTr("Recording started"), time: "14:31:50", threadId: 1, file: "", line: 0 }
        ];
        for (const entry of samples) {
            mockLogModel.append(entry);
        }
    }

    Timer {
        id: responseTimer

        property string text: ""

        interval: 200
        repeat: false
        onTriggered: {
            if (text.trimStart().startsWith("{")) {
                try {
                    const parsed = JSON.parse(text);
                    root.appendTerminalEntry("response", JSON.stringify({ status: "ok", echo: parsed }, null, 2));
                } catch (e) {
                    root.appendTerminalEntry("error", e.toString());
                }
            } else {
                root.appendTerminalEntry("response", qsTr("Command executed: ") + text);
            }
        }
    }

    readonly property var tabs: [
        { "title": qsTr("Logs"), "icon": "list" },
        { "title": qsTr("Command Line"), "icon": "terminal" }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.gapWide
        spacing: root.theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.gapTight

            Text {
                text: qsTr("Activity Center")
                color: root.theme.textPrimary
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: root.theme.radiusSmall
                color: closeArea.pressed ? root.theme.surfaceStrong
                                         : (closeArea.containsMouse ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.96) : "transparent")
                border.width: 1
                border.color: closeArea.containsMouse ? root.theme.tint(root.theme.accentA, 0.35) : "transparent"

                AppIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    iconKind: "closePanel"
                    width: 16
                    height: 16
                }

                MouseArea {
                    id: closeArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.closeRequested()
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

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        AppIcon {
                            theme: root.theme
                            iconKind: modelData.icon
                            useThemeContrast: false
                            primaryColor: root.currentTab === index ? root.theme.textPrimary : root.theme.textSecondary
                            width: 14
                            height: 14
                        }

                        Text {
                            text: modelData.title
                            color: root.theme.textPrimary
                            font.pixelSize: 11
                            font.bold: root.currentTab === index
                        }
                    }

                    MouseArea {
                        id: tabArea

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

            LogEventsView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
                model: mockLogModel
            }

            TerminalView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
                model: mockTerminalModel
                onCommandSubmitted: function(text) { root.runCommand(text) }
            }
        }
    }
}
