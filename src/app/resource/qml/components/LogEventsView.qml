pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"

Rectangle {
    id: root
    required property AppTheme theme
    required property var model
    property int enabledLevelMask: 0x3F
    property int runtimeMinLevel: 2
    property bool filterOpen: false
    color: "transparent"

    readonly property var levelOptions: [
        { "level": 0, "label": qsTr("Trace") },
        { "level": 1, "label": qsTr("Debug") },
        { "level": 2, "label": qsTr("Info") },
        { "level": 3, "label": qsTr("Warn") },
        { "level": 4, "label": qsTr("Error") },
        { "level": 5, "label": qsTr("Critical") }
    ]

    function levelTint(level: int): color {
        if (level >= 4) return root.theme.accentD;
        if (level === 3) return root.theme.accentC;
        if (level === 2) return root.theme.accentB;
        return root.theme.accentA;
    }

    function levelLabel(level: int): string {
        for (let i = 0; i < root.levelOptions.length; ++i) {
            if (root.levelOptions[i].level === level)
                return root.levelOptions[i].label;
        }
        return qsTr("Info");
    }

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

            Item { Layout.fillWidth: true }

            Rectangle {
                implicitWidth: 72
                implicitHeight: 28
                radius: root.theme.radiusSmall
                color: levelsArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                          : (levelsArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                Text {
                    anchors.centerIn: parent
                    text: root.filterOpen ? qsTr("Levels ▲") : qsTr("Levels ▼")
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                }

                MouseArea {
                    id: levelsArea
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
                color: clearArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                         : (clearArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Clear")
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                }

                MouseArea {
                    id: clearArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.model.clear()
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
                }

                Text {
                    width: parent.width
                    text: qsTr("This controls which new log messages are emitted by the runtime logger pipeline.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textTertiary
                    font.pixelSize: 10
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: root.levelOptions

                        delegate: LogLevelChip {
                            required property var modelData

                            theme: root.theme
                            text: modelData.label
                            accentColor: root.levelTint(modelData.level)
                            selected: root.runtimeMinLevel === modelData.level
                            onClicked: root.runtimeMinLevel = modelData.level
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
                }

                Text {
                    width: parent.width
                    text: qsTr("This only filters the entries already captured inside the panel.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textTertiary
                    font.pixelSize: 10
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: root.levelOptions

                        delegate: LogLevelChip {
                            required property var modelData

                            readonly property bool selectedLevel: (root.enabledLevelMask & (1 << modelData.level)) !== 0

                            theme: root.theme
                            text: modelData.label
                            accentColor: root.levelTint(modelData.level)
                            selected: selectedLevel
                            onClicked: root.enabledLevelMask ^= (1 << modelData.level)
                        }
                    }
                }
            }
        }

        ListView {
            id: eventsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            rightMargin: eventScrollBar.width + root.theme.gapTight
            clip: true
            spacing: root.theme.gapTight
            model: root.model
            boundsBehavior: Flickable.StopAtBounds
            property bool stickToEnd: true

            onCountChanged: if (stickToEnd) Qt.callLater(function() { eventsList.positionViewAtEnd(); })
            onMovementEnded: stickToEnd = atYEnd || contentHeight <= height
            onFlickEnded: stickToEnd = atYEnd || contentHeight <= height
            onContentYChanged: if (!moving && !flicking) stickToEnd = atYEnd || contentHeight <= height

            delegate: Rectangle {
                id: eventCard
                required property int level
                required property string levelName
                required property string source
                required property string message
                required property string time
                required property int threadId
                required property string file
                required property int line
                required property int index

                readonly property string displayLevelName: root.levelLabel(level).toUpperCase()
                readonly property color entryAccent: root.levelTint(level)

                visible: (root.enabledLevelMask & (1 << level)) !== 0
                height: visible ? implicitHeight : 0
                width: eventsList.width - eventsList.leftMargin - eventsList.rightMargin
                implicitHeight: contentColumn.implicitHeight + 18
                radius: root.theme.radiusSmall
                color: root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.72 : 0.98)
                border.width: 1
                border.color: root.theme.tint(entryAccent, root.theme.darkMode ? 0.36 : 0.2)

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 7
                    width: 4
                    radius: 2
                    color: eventCard.entryAccent
                }

                ColumnLayout {
                    id: contentColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 9
                    anchors.leftMargin: 14
                    anchors.rightMargin: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.theme.gapTight

                        Rectangle {
                            implicitWidth: badgeLabel.implicitWidth + 14
                            implicitHeight: 20
                            radius: 8
                            color: root.theme.tint(eventCard.entryAccent, root.theme.darkMode ? 0.24 : 0.14)
                            border.width: 1
                            border.color: root.theme.tint(eventCard.entryAccent, root.theme.darkMode ? 0.5 : 0.28)

                            Text {
                                id: badgeLabel
                                anchors.centerIn: parent
                                text: eventCard.displayLevelName
                                color: root.theme.textPrimary
                                font.pixelSize: 10
                                font.bold: true
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: source
                            color: root.theme.textPrimary
                            elide: Text.ElideRight
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Text {
                            text: time
                            color: root.theme.textTertiary
                            font.family: root.theme.monoFontFamily
                            font.pixelSize: 10
                            Layout.alignment: Qt.AlignRight
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: message
                        color: root.theme.textPrimary
                        wrapMode: Text.Wrap
                        font.pixelSize: 12
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: file.length > 0 || threadId > 0
                        text: qsTr("tid %1").arg(threadId) + (file.length > 0 ? qsTr(" · %1:%2").arg(file).arg(line) : "")
                        color: root.theme.textTertiary
                        font.pixelSize: 10
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                id: eventScrollBar
                width: 6
                policy: ScrollBar.AsNeeded

                background: Rectangle {
                    implicitWidth: 6
                    radius: width / 2
                    color: root.theme.tint(root.theme.borderSubtle, 0.3)
                }

                contentItem: Rectangle {
                    implicitWidth: 6
                    implicitHeight: Math.max(26, eventScrollBar.availableHeight * eventScrollBar.visualSize)
                    radius: width / 2
                    color: root.theme.surfaceStrong
                }
            }
        }
    }
}
