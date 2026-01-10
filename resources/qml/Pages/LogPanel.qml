pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    width: 360
    height: 260

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
            height: 28
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Logs")
                color: Theme.palette.text
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Button {
                implicitWidth: 60
                text: qsTr("Clear")
                onClicked: root.logService.clear()
            }

            Button {
                text: "✕"
                implicitWidth: 28
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
                    required property string file
                    required property int line
                    required property color levelColor

                    width: list.width
                    height: content.implicitHeight + 10
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

                                // Info level gets a badge-like background (similar to the old category chip)
                                Rectangle {
                                    visible: row.level === 2
                                    radius: 6
                                    color: row.levelColor
                                    Layout.preferredHeight: 18
                                    Layout.preferredWidth: Math.min(110, infoLevelText.implicitWidth + 12)

                                    Text {
                                        id: infoLevelText
                                        anchors.centerIn: parent
                                        text: row.levelName
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }
                                }

                                Label {
                                    visible: row.level !== 2
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

                            Text {
                                Layout.fillWidth: true
                                text: row.message
                                color: Theme.palette.text
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                maximumLineCount: 4
                                elide: Text.ElideRight
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
        }
    }
}
