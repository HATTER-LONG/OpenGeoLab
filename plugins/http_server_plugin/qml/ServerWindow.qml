import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

ApplicationWindow {
    id: window

    width: 720
    height: 700
    minimumWidth: 480
    minimumHeight: 400
    title: qsTr("HTTP Server — OpenGeoLab")
    color: PluginTheme.bg
    visible: true

    required property var backend

    // ── JSON Syntax Highlighter ─────────────────────────────────────
    function highlightJson(obj) {
        if (obj === undefined || obj === null)
            return "";
        const raw = JSON.stringify(obj, null, 2);
        let safe = raw.replace(/&/g, "&amp;")
                      .replace(/</g, "&lt;")
                      .replace(/>/g, "&gt;");

        const keyColor = PluginTheme.darkMode ? "#5aa2ff" : "#1473e6";
        const strColor = PluginTheme.darkMode ? "#6fe3b0" : "#1f9d68";
        const numColor = PluginTheme.darkMode ? "#ffb86c" : "#d08800";
        const boolColor = PluginTheme.darkMode ? "#c9a0ff" : "#7c3aed";
        const nullColor = PluginTheme.darkMode ? "#ff8d7d" : "#d9534f";

        safe = safe.replace(
            /("(?:\\.|[^"\\])*")(\s*:)?|\b(true|false)\b|\bnull\b|(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)/g,
            function(match, str, colon, bool, num) {
                if (str) {
                    if (colon)
                        return '<span style="color:' + keyColor + '">' + str + '</span>' + colon;
                    return '<span style="color:' + strColor + '">' + str + '</span>';
                }
                if (bool !== undefined && bool !== "")
                    return '<span style="color:' + boolColor + '">' + bool + '</span>';
                if (match === "null")
                    return '<span style="color:' + nullColor + '">null</span>';
                if (num !== undefined && num !== "")
                    return '<span style="color:' + numColor + '">' + num + '</span>';
                return match;
            }
        );
        return "<pre style='margin:0; white-space:pre-wrap; font-size:11px;'>" + safe + "</pre>";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: PluginTheme.gap
        spacing: PluginTheme.gap

        // ── Config Section ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: configColumn.implicitHeight + 24
            color: PluginTheme.surface
            radius: PluginTheme.radius
            border.width: 1
            border.color: PluginTheme.borderSubtle

            ColumnLayout {
                id: configColumn

                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: qsTr("HTTP Server")
                    color: PluginTheme.textPrimary
                    font.pixelSize: 16
                    font.bold: true
                }

                RowLayout {
                    spacing: 8

                    Text {
                        text: qsTr("Host:")
                        color: PluginTheme.textSecondary
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                    }

                    TextField {
                        id: hostField

                        text: window.backend.host
                        enabled: !window.backend.running
                        Layout.preferredWidth: 140
                        font.pixelSize: 13
                        color: PluginTheme.textPrimary
                        placeholderText: "127.0.0.1"

                        background: Rectangle {
                            color: PluginTheme.surfaceStrong
                            radius: 4
                            border.width: 1
                            border.color: hostField.activeFocus ? PluginTheme.accent : PluginTheme.border
                        }

                        onEditingFinished: window.backend.host = text
                    }

                    Text {
                        text: qsTr("Port:")
                        color: PluginTheme.textSecondary
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                    }

                    TextField {
                        id: portField

                        text: window.backend.port.toString()
                        enabled: !window.backend.running
                        Layout.preferredWidth: 80
                        font.pixelSize: 13
                        color: PluginTheme.textPrimary
                        placeholderText: "8080"
                        validator: IntValidator {
                            bottom: 1
                            top: 65535
                        }

                        background: Rectangle {
                            color: PluginTheme.surfaceStrong
                            radius: 4
                            border.width: 1
                            border.color: portField.activeFocus ? PluginTheme.accent : PluginTheme.border
                        }

                        onEditingFinished: window.backend.port = parseInt(text)
                    }
                }

                RowLayout {
                    spacing: 12

                    Button {
                        id: toggleBtn

                        text: window.backend.running ? qsTr("■ Stop Server") : qsTr("▶ Start Server")
                        font.pixelSize: 13
                        font.bold: true

                        contentItem: Text {
                            text: toggleBtn.text
                            color: "#ffffff"
                            font: toggleBtn.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 130
                            implicitHeight: 32
                            color: window.backend.running ? PluginTheme.danger : PluginTheme.accent
                            radius: 6
                            opacity: toggleBtn.hovered ? 0.85 : 1.0
                        }

                        onClicked: {
                            if (window.backend.running) {
                                window.backend.stop();
                            } else {
                                window.backend.start();
                            }
                        }
                    }

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: window.backend.running ? PluginTheme.success : PluginTheme.danger
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: window.backend.running ? qsTr("Running") : qsTr("Stopped")
                        color: window.backend.running ? PluginTheme.success : PluginTheme.textSecondary
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }
        }

        // ── Error banner ────────────────────────────────────────────
        Rectangle {
            id: errorBanner

            Layout.fillWidth: true
            Layout.preferredHeight: visible ? errorText.implicitHeight + 16 : 0
            visible: false
            color: PluginTheme.danger
            radius: PluginTheme.radius
            opacity: 0.9

            Text {
                id: errorText

                anchors.fill: parent
                anchors.margins: 8
                color: "#ffffff"
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }

            Connections {
                target: window.backend

                function onErrorOccurred(message) {
                    errorText.text = message;
                    errorBanner.visible = true;
                    errorHideTimer.restart();
                }
            }

            Timer {
                id: errorHideTimer

                interval: 5000
                onTriggered: errorBanner.visible = false
            }
        }

        // ── Main SplitView (Log + Detail) ───────────────────────────
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            handle: Rectangle {
                implicitWidth: parent.width
                implicitHeight: 6
                color: SplitHandle.hovered || SplitHandle.pressed
                       ? PluginTheme.accent : PluginTheme.borderSubtle

                Rectangle {
                    anchors.centerIn: parent
                    width: 40
                    height: 2
                    radius: 1
                    color: SplitHandle.hovered || SplitHandle.pressed
                           ? "#ffffff" : PluginTheme.textTertiary
                }
            }

            // ── Request Log Panel ───────────────────────────────────
            Rectangle {
                SplitView.preferredHeight: 260
                SplitView.minimumHeight: 100
                SplitView.fillWidth: true
                color: PluginTheme.surface
                radius: PluginTheme.radius
                border.width: 1
                border.color: PluginTheme.borderSubtle

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Text {
                        text: qsTr("Request Log")
                        color: PluginTheme.textPrimary
                        font.pixelSize: 14
                        font.bold: true
                    }

                    ListView {
                        id: logList

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: window.backend.requestLog
                        clip: true
                        spacing: 2

                        delegate: Rectangle {
                            id: logEntry

                            required property int index
                            required property var modelData

                            width: logList.width
                            height: logContent.implicitHeight + 8
                            color: logEntry.index === logList.currentIndex
                                   ? PluginTheme.surfaceStrong
                                   : (logMouse.containsMouse ? PluginTheme.surfaceMuted : "transparent")
                            radius: 4

                            ColumnLayout {
                                id: logContent

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 6
                                spacing: 2

                                Text {
                                    text: {
                                        const d = logEntry.modelData;
                                        const label = d.module
                                            ? d.module + "." + d.action
                                            : d.path;
                                        return d.time + "  " + d.method + "  " + label;
                                    }
                                    color: PluginTheme.textPrimary
                                    font.pixelSize: 12
                                    font.family: "monospace"
                                }

                                Text {
                                    text: {
                                        const d = logEntry.modelData;
                                        const statusText = d.ok ? "OK" : "FAIL";
                                        return "→ " + d.status + " " + statusText
                                               + " (" + d.duration_ms + "ms)";
                                    }
                                    color: logEntry.modelData.ok
                                           ? PluginTheme.success
                                           : PluginTheme.danger
                                    font.pixelSize: 11
                                    font.family: "monospace"
                                }
                            }

                            MouseArea {
                                id: logMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: logList.currentIndex = logEntry.index
                            }
                        }

                        onCountChanged: {
                            if (count > 0) {
                                positionViewAtEnd();
                            }
                        }
                    }
                }
            }

            // ── Detail Panel (Request | Response) ───────────────────
            Rectangle {
                id: detailPanel

                SplitView.preferredHeight: 240
                SplitView.minimumHeight: 80
                SplitView.fillWidth: true
                color: PluginTheme.surface
                radius: PluginTheme.radius
                border.width: 1
                border.color: PluginTheme.borderSubtle
                visible: logList.currentIndex >= 0 && logList.currentIndex < window.backend.requestLog.length

                property var selectedEntry: {
                    const idx = logList.currentIndex;
                    const log = window.backend.requestLog;
                    if (idx >= 0 && idx < log.length) return log[idx];
                    return null;
                }

                SplitView {
                    anchors.fill: parent
                    anchors.margins: 8
                    orientation: Qt.Horizontal

                    handle: Rectangle {
                        implicitWidth: 6
                        implicitHeight: parent.height
                        color: SplitHandle.hovered || SplitHandle.pressed
                               ? PluginTheme.accent : PluginTheme.borderSubtle

                        Rectangle {
                            anchors.centerIn: parent
                            width: 2
                            height: 30
                            radius: 1
                            color: SplitHandle.hovered || SplitHandle.pressed
                                   ? "#ffffff" : PluginTheme.textTertiary
                        }
                    }

                    // ── Request Side ────────────────────────────────
                    Rectangle {
                        SplitView.preferredWidth: parent.width / 2
                        SplitView.minimumWidth: 120
                        color: "transparent"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 4

                            RowLayout {
                                spacing: 6

                                Rectangle {
                                    width: 6
                                    height: 14
                                    radius: 2
                                    color: PluginTheme.accent
                                }

                                Text {
                                    text: qsTr("Request")
                                    color: PluginTheme.textPrimary
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }

                            Flickable {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                contentWidth: width
                                contentHeight: requestText.implicitHeight
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds

                                ScrollBar.vertical: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                }

                                Text {
                                    id: requestText

                                    width: parent.width
                                    wrapMode: Text.Wrap
                                    textFormat: Text.RichText
                                    font.family: "monospace"
                                    color: PluginTheme.textSecondary
                                    text: {
                                        if (!detailPanel.selectedEntry || !detailPanel.selectedEntry.request_body)
                                            return '<span style="color:' + PluginTheme.textTertiary + '">'
                                                   + qsTr("No request body.") + '</span>';
                                        return window.highlightJson(detailPanel.selectedEntry.request_body);
                                    }
                                }
                            }
                        }
                    }

                    // ── Response Side ───────────────────────────────
                    Rectangle {
                        SplitView.fillWidth: true
                        SplitView.minimumWidth: 120
                        color: "transparent"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 4

                            RowLayout {
                                spacing: 6

                                Rectangle {
                                    width: 6
                                    height: 14
                                    radius: 2
                                    color: (detailPanel.selectedEntry && detailPanel.selectedEntry.ok)
                                           ? PluginTheme.success : PluginTheme.danger
                                }

                                Text {
                                    text: qsTr("Response")
                                    color: PluginTheme.textPrimary
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                Text {
                                    text: {
                                        if (!detailPanel.selectedEntry) return "";
                                        return detailPanel.selectedEntry.ok
                                               ? "✓ " + detailPanel.selectedEntry.status
                                               : "✗ " + detailPanel.selectedEntry.status;
                                    }
                                    color: (detailPanel.selectedEntry && detailPanel.selectedEntry.ok)
                                           ? PluginTheme.success : PluginTheme.danger
                                    font.pixelSize: 11
                                    font.family: "monospace"
                                }
                            }

                            Flickable {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                contentWidth: width
                                contentHeight: responseText.implicitHeight
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds

                                ScrollBar.vertical: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                }

                                Text {
                                    id: responseText

                                    width: parent.width
                                    wrapMode: Text.Wrap
                                    textFormat: Text.RichText
                                    font.family: "monospace"
                                    color: PluginTheme.textSecondary
                                    text: {
                                        if (!detailPanel.selectedEntry || !detailPanel.selectedEntry.response_body)
                                            return '<span style="color:' + PluginTheme.textTertiary + '">'
                                                   + qsTr("No response body.") + '</span>';
                                        return window.highlightJson(detailPanel.selectedEntry.response_body);
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
