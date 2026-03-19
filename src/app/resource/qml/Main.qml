import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    width: 1400
    height: 860
    minimumWidth: 960
    minimumHeight: 640
    visible: true
    title: "OpenGeoLab Skeleton"
    readonly property bool compactLayout: width < 1220

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Label {
                text: "QML -> Python runtime -> py_wrapper -> C++ backend"
                font.bold: true
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                elide: Text.ElideRight
            }

            Label {
                text: appController.statusText
                color: "#5dade2"
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: root.compactLayout ? Qt.Vertical : Qt.Horizontal

        ScrollView {
            id: requestPanel
            SplitView.preferredWidth: 760
            SplitView.preferredHeight: 480
            SplitView.minimumWidth: 420
            SplitView.minimumHeight: 320
            clip: true

            Item {
                width: requestPanel.availableWidth
                implicitHeight: requestColumn.implicitHeight + 32

                ColumnLayout {
                    id: requestColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    GroupBox {
                        title: "Request presets"
                        Layout.fillWidth: true
                        Layout.preferredHeight: requestPresetFlow.implicitHeight + 24

                        Flow {
                            id: requestPresetFlow
                            x: 8
                            y: 8
                            width: parent.width - 16
                            spacing: 8

                            Button {
                                text: "Ping"
                                onClicked: appController.loadPingExample()
                            }

                            Button {
                                text: "Geometry"
                                onClicked: appController.loadGeometryExample()
                            }

                            Button {
                                text: "Snapshot"
                                onClicked: appController.loadSnapshotExample()
                            }

                            Button {
                                text: "Plugins"
                                onClicked: appController.loadPluginExample()
                            }

                            Button {
                                text: "Process"
                                highlighted: true
                                onClicked: {
                                    appController.requestText = requestEditor.text
                                    appController.sendRequest()
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: "JSON request"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 260

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 8
                            clip: true

                            TextArea {
                                id: requestEditor
                                wrapMode: TextEdit.NoWrap
                                selectByMouse: true
                                font.family: "Consolas"
                                text: ""
                                onTextChanged: appController.requestText = text
                            }
                        }
                    }

                    GroupBox {
                        title: "Protocol notes"
                        Layout.fillWidth: true

                        Label {
                            anchors.fill: parent
                            anchors.margins: 8
                            width: parent.width - 16
                            wrapMode: Text.WordWrap
                            text: "The skeleton keeps one JSON process envelope for QML, embedded Python, external Python callers, and future LLM tools. It already reserves hooks for render snapshots, selection descriptions, diagnostics, and plugin metadata."
                        }
                    }
                }
            }
        }

        ScrollView {
            id: responsePanel
            SplitView.preferredWidth: 640
            SplitView.preferredHeight: 360
            SplitView.minimumWidth: 380
            SplitView.minimumHeight: 320
            clip: true

            Item {
                width: responsePanel.availableWidth
                implicitHeight: responseColumn.implicitHeight + 32

                ColumnLayout {
                    id: responseColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    GroupBox {
                        title: "Response"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 240

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 8
                            clip: true

                            TextArea {
                                readOnly: true
                                wrapMode: TextEdit.NoWrap
                                selectByMouse: true
                                font.family: "Consolas"
                                text: appController.responseText
                            }
                        }
                    }

                    GroupBox {
                        title: "Snapshot preview"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 260
                        Layout.minimumHeight: 220

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 8
                            radius: 8
                            color: "#20252b"
                            border.color: "#3b4653"

                            Image {
                                anchors.fill: parent
                                anchors.margins: 12
                                source: appController.snapshotUrl
                                fillMode: Image.PreserveAspectFit
                                visible: source !== ""
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: appController.snapshotUrl === ""
                                text: "No snapshot returned yet"
                                color: "#aab2bd"
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: appController

        function onRequestTextChanged() {
            if (requestEditor.text !== appController.requestText) {
                requestEditor.text = appController.requestText
            }
        }
    }

    Component.onCompleted: requestEditor.text = appController.requestText
}
