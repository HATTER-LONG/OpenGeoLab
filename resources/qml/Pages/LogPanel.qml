pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"

/**
 * @file LogPanel.qml
 * @brief Floating log panel for viewing application logs from LogService
 */
Item {
    id: root

    // Provided by parent (typically Pages.CornerOverlay).
    property var logService

    property bool open: false
    signal requestClose

    width: 480
    height: 500

    visible: open || showAnim.running || hideAnim.running
    opacity: 0.0
    y: 10

    function scrollToEnd() {
        if (list.count > 0) {
            list.positionViewAtEnd();
        }
    }

    states: [
        State {
            name: "open"
            when: root.open
            PropertyChanges {
                target: root
                opacity: 1.0
                y: 0
            }
        },
        State {
            name: "closed"
            when: !root.open
            PropertyChanges {
                target: root
                opacity: 0.0
                y: 10
            }
        }
    ]

    transitions: [
        Transition {
            from: "closed"
            to: "open"
            ParallelAnimation {
                NumberAnimation {
                    id: showAnim
                    properties: "opacity,y"
                    duration: 180
                    easing.type: Easing.OutQuad
                }
            }
        },
        Transition {
            from: "open"
            to: "closed"
            ParallelAnimation {
                NumberAnimation {
                    id: hideAnim
                    properties: "opacity,y"
                    duration: 160
                    easing.type: Easing.InQuad
                }
            }
        }
    ]

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            height: 26
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Logs")
                color: Theme.palette.text
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            BaseButton {
                id: filterButton
                implicitWidth: 70
                text: qsTr("Filter ▼")
                onClicked: {
                    if (filterPopup.opened) {
                        filterPopup.close();
                        filterButton.text = qsTr("Filter ▼");
                    } else {
                        filterPopup.open();
                        filterButton.text = qsTr("Filter ▲");
                    }
                }
            }

            Popup {
                id: filterPopup
                x: filterButton.x - width + filterButton.width
                y: filterButton.y + filterButton.height + 6
                width: 240
                modal: false
                focus: true
                closePolicy: Popup.CloseOnEscape
                padding: 12

                background: Rectangle {
                    radius: 10
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.border
                }

                contentItem: ColumnLayout {
                    spacing: 12

                    // Min Level Section
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: qsTr("Log Level")
                            color: Theme.palette.text
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        BaseComboBox {
                            id: minLevelCombo
                            model: ["Trace", "Debug", "Info", "Warn", "Error", "Critical"]
                            currentIndex: root.logService ? root.logService.minLevel : 0
                            onCurrentIndexChanged: if (root.logService)
                                root.logService.setMinLevel(minLevelCombo.currentIndex)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.border
                    }

                    // Level Toggles Section
                    Label {
                        text: qsTr("Show Levels")
                        color: Theme.palette.placeholderText
                        font.pixelSize: 11
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 6

                        CheckBox {
                            text: qsTr("Trace")
                            checked: root.logService ? root.logService.levelEnabled(0) : true
                            onToggled: if (root.logService)
                                root.logService.setLevelEnabled(0, checked)
                        }

                        CheckBox {
                            text: qsTr("Debug")
                            checked: root.logService ? root.logService.levelEnabled(1) : true
                            onToggled: if (root.logService)
                                root.logService.setLevelEnabled(1, checked)
                        }

                        CheckBox {
                            text: qsTr("Info")
                            checked: root.logService ? root.logService.levelEnabled(2) : true
                            onToggled: if (root.logService)
                                root.logService.setLevelEnabled(2, checked)
                        }

                        CheckBox {
                            text: qsTr("Warn")
                            checked: root.logService ? root.logService.levelEnabled(3) : true
                            onToggled: if (root.logService)
                                root.logService.setLevelEnabled(3, checked)
                        }

                        CheckBox {
                            text: qsTr("Error")
                            checked: root.logService ? root.logService.levelEnabled(4) : true
                            onToggled: if (root.logService)
                                root.logService.setLevelEnabled(4, checked)
                        }

                        CheckBox {
                            text: qsTr("Critical")
                            checked: root.logService ? root.logService.levelEnabled(5) : true
                            onToggled: if (root.logService)
                                root.logService.setLevelEnabled(5, checked)
                        }
                    }
                }
            }

            BaseButton {
                id: clearButton
                implicitWidth: 56
                text: qsTr("Clear")
                onClicked: root.logService.clear()
            }

            BaseButton {
                id: closeButton
                text: "✕"
                implicitWidth: 28
                padding: 4
                onClicked: root.requestClose()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border
            clip: true

            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6
                clip: true
                model: root.logService ? root.logService.model : null

                ScrollBar.vertical: ScrollBar {}

                delegate: Rectangle {
                    id: row
                    required property string time
                    required property int level
                    required property string levelName
                    required property string message
                    required property int tid
                    required property string file
                    required property int line
                    required property color levelColor

                    width: list.width
                    height: content.implicitHeight + 16
                    radius: 8
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.border

                    RowLayout {
                        id: content
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Rectangle {
                            width: 4
                            Layout.fillHeight: true
                            radius: 2
                            color: row.levelColor
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    text: row.time
                                    color: Theme.palette.placeholderText
                                    font.pixelSize: 11
                                }

                                Label {
                                    text: "tid " + row.tid
                                    color: Theme.palette.placeholderText
                                    font.pixelSize: 11
                                }

                                Label {
                                    text: row.levelName
                                    color: row.levelColor
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: row.file.length > 0 ? (row.file + ":" + row.line) : ""
                                    visible: text.length > 0
                                    color: Theme.palette.placeholderText
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignRight
                                }
                            }

                            TextEdit {
                                Layout.fillWidth: true
                                text: row.message
                                color: Theme.palette.text
                                wrapMode: TextEdit.Wrap
                                readOnly: true
                                selectByMouse: true
                                cursorVisible: false
                                textFormat: TextEdit.PlainText
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }

            // Auto-scroll when new rows arrive (only when panel is open)
            Connections {
                target: root.logService ? root.logService.model : null
                function onRowsInserted(parent, first, last) {
                    if (root.open) {
                        root.scrollToEnd();
                    }
                }
                function onModelReset() {
                    if (root.open) {
                        root.scrollToEnd();
                    }
                }
            }
            Connections {
                target: root
                function onOpenChanged() {
                    if (root.open) {
                        root.scrollToEnd();
                    }
                    if (root.open === false) {
                        filterPopup.close();
                        filterButton.text = qsTr("Filter ▼");
                    }
                }
            }
        }
    }
}
