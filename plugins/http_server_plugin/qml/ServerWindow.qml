import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

ApplicationWindow {
    id: window

    width: 640
    height: 560
    title: qsTr("HTTP Server — OpenGeoLab")
    color: PluginTheme.bg
    visible: true

    required property var backend

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

        // ── Request Log ─────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
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

        // ── Detail Panel ────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            color: PluginTheme.surface
            radius: PluginTheme.radius
            border.width: 1
            border.color: PluginTheme.borderSubtle
            visible: logList.currentIndex >= 0 && logList.currentIndex < window.backend.requestLog.length

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Text {
                    text: qsTr("Request / Response Detail")
                    color: PluginTheme.textPrimary
                    font.pixelSize: 13
                    font.bold: true
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: width
                    contentHeight: detailText.implicitHeight
                    clip: true

                    Text {
                        id: detailText

                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 11
                        font.family: "monospace"
                        color: PluginTheme.textSecondary
                        text: {
                            const idx = logList.currentIndex;
                            const log = window.backend.requestLog;
                            if (idx < 0 || idx >= log.length) return "";
                            const entry = log[idx];
                            let result = "";
                            if (entry.request_body) {
                                result += "Request:\n" + JSON.stringify(entry.request_body, null, 2) + "\n\n";
                            }
                            if (entry.response_body) {
                                result += "Response:\n" + JSON.stringify(entry.response_body, null, 2);
                            }
                            return result || qsTr("No detail available.");
                        }
                    }
                }
            }
        }
    }
}
