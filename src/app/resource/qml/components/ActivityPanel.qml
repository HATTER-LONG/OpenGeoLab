pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services
import "../theme"

Item {
    id: root

    required property AppTheme theme
    property int currentTab: 0
    property real availableHeight: 480
    signal closeRequested

    readonly property int minimumHeight: 300
    readonly property int maximumHeight: Math.max(minimumHeight, Math.min(Math.floor(availableHeight), 560))
    readonly property int resolvedHeight: maximumHeight

    implicitWidth: 360
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

    ListModel {
        id: terminalModel
    }

    function formatJson(text: string): string {
        try {
            return JSON.stringify(JSON.parse(text), null, 2);
        } catch (e) {
            return text;
        }
    }

    function appendTerminalEntry(type: string, header: string, json: string): void {
        terminalModel.append({
            type: type,
            header: header,
            json: json
        });
        if (terminalModel.count > 160) {
            terminalModel.remove(0);
        }
    }

    function runCommand(text: string): void {
        RequestService.submitAsync(text);
    }

    Connections {
        target: RequestService

        function onRequestSent(description: string, requestJson: string, muted: bool): void {
            const label = "[" + description + "]" + (muted ? " (muted)" : "");
            root.appendTerminalEntry("command", label, root.formatJson(requestJson));
        }

        function onResponseReady(responseJson: string, muted: bool): void {
            root.appendTerminalEntry("response", "", root.formatJson(responseJson));
        }

        function onErrorOccurred(errorMessage: string, muted: bool): void {
            root.appendTerminalEntry("error", "", errorMessage);
        }
    }

    readonly property var tabs: [
        {
            "title": qsTr("Logs"),
            "icon": "list"
        },
        {
            "title": qsTr("Command Line"),
            "icon": "terminal"
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.gapWide
        spacing: root.theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: root.tabs

                delegate: Rectangle {
                    id: tabDelegate
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: root.theme.radiusSmall
                    color: root.currentTab === tabDelegate.index ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.24 : 0.12) : root.theme.surfaceMuted
                    border.width: 1
                    border.color: root.currentTab === tabDelegate.index ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.56 : 0.3) : root.theme.tint(root.theme.borderSubtle, 0.75)

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        AppIcon {
                            theme: root.theme
                            iconKind: tabDelegate.modelData.icon
                            useThemeContrast: false
                            primaryColor: root.currentTab === tabDelegate.index ? root.theme.textPrimary : root.theme.textSecondary
                            width: 14
                            height: 14
                        }

                        Text {
                            text: tabDelegate.modelData.title
                            color: root.theme.textPrimary
                            font.pixelSize: 11
                            font.bold: root.currentTab === tabDelegate.index
                        }
                    }

                    MouseArea {
                        id: tabArea

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentTab = tabDelegate.index
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: root.theme.radiusSmall
                color: closeArea.pressed ? root.theme.surfaceStrong : (closeArea.containsMouse ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.96) : "transparent")
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

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentTab

            LogEventsView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
                model: LogEventModel
                onRuntimeMinLevelChanged: LogEventModel.setRuntimeMinLevel(runtimeMinLevel)
            }

            TerminalView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
                model: terminalModel
                onCommandSubmitted: function (text) {
                    root.runCommand(text);
                }
            }
        }
    }
}
