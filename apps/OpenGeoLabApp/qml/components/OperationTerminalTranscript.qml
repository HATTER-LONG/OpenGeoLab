pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Item {
    id: root

    required property AppTheme theme
    property var entriesModel: null
    property string emptyText: ""
    property color backgroundColor: theme.darkMode ? "#071018" : "#ffffff"
    property color borderColor: theme.darkMode ? "#223341" : "#cfd7e3"
    property color mutedTextColor: theme.darkMode ? "#7f98b0" : "#66788b"
    property color commandTextColor: theme.darkMode ? "#86f08d" : "#1e7f34"
    property color responseTextColor: theme.darkMode ? "#ffbf73" : "#b65a00"
    property color thumbColor: theme.darkMode ? "#4b6984" : "#aac2d8"
    readonly property int entryCount: entriesModel && entriesModel.count !== undefined ? entriesModel.count : 0

    function scrollToEnd() {
        Qt.callLater(function () {
            if (!transcriptFlickable) {
                return;
            }
            transcriptFlickable.contentY = Math.max(0, transcriptFlickable.contentHeight - transcriptFlickable.height);
        });
    }

    onEntriesModelChanged: root.scrollToEnd()

    Component.onCompleted: root.scrollToEnd()

    Connections {
        target: root.entriesModel
        ignoreUnknownSignals: true

        function onRowsInserted() {
            root.scrollToEnd();
        }

        function onModelReset() {
            root.scrollToEnd();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
        border.width: 1
        border.color: root.theme.tint(root.borderColor, root.theme.darkMode ? 0.9 : 1.0)
        clip: true

        Flickable {
            id: transcriptFlickable

            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 18
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            contentWidth: width
            contentHeight: Math.max(height, transcriptColumn.implicitHeight)
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: transcriptColumn

                width: parent.width
                spacing: 6

                Repeater {
                    model: root.entriesModel

                    delegate: TextEdit {
                        required property string kind
                        required property string body

                        width: transcriptColumn.width
                        height: contentHeight
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.WrapAnywhere
                        color: kind === "command" ? root.commandTextColor : root.responseTextColor
                        text: body
                        font.pixelSize: 12
                        font.family: root.theme.monoFontFamily
                    }
                }

                TextEdit {
                    visible: root.entryCount === 0
                    width: transcriptColumn.width
                    height: contentHeight
                    readOnly: true
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.WrapAnywhere
                    color: root.mutedTextColor
                    text: root.emptyText
                    font.pixelSize: 12
                    font.family: root.theme.monoFontFamily
                }
            }
        }

        Rectangle {
            id: scrollTrack

            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 6
            width: 6
            radius: 3
            color: root.theme.darkMode ? "#0f1b27" : "#eef3f8"
            visible: transcriptFlickable.contentHeight > transcriptFlickable.height + 1

            Rectangle {
                id: scrollThumb

                width: parent.width
                radius: width / 2
                color: root.thumbColor
                y: transcriptFlickable.visibleArea.yPosition * parent.height
                height: Math.max(26, transcriptFlickable.visibleArea.heightRatio * parent.height)

                MouseArea {
                    anchors.fill: parent
                    cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    drag.target: parent
                    drag.axis: Drag.YAxis
                    drag.minimumY: 0
                    drag.maximumY: Math.max(0, scrollTrack.height - parent.height)

                    onPositionChanged: {
                        if (!drag.active) {
                            return;
                        }
                        const denominator = Math.max(1, scrollTrack.height - parent.height);
                        const ratio = parent.y / denominator;
                        transcriptFlickable.contentY = ratio * Math.max(0, transcriptFlickable.contentHeight - transcriptFlickable.height);
                    }
                }
            }
        }
    }
}
