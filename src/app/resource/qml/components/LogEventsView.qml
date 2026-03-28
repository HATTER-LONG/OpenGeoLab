pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import OpenGeoLab.Services
import "../theme"

Rectangle {
    id: root
    required property AppTheme theme
    required property var model
    property int enabledLevelMask: 0x3F
    property int runtimeMinLevel: 2
    property bool filterOpen: false
    color: "transparent"

    LogFilterProxyModel {
        id: filterProxy
        sourceModel: root.model
        enabledLevelMask: root.enabledLevelMask
    }

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

    CopyContextMenu {
        id: logContextMenu
        theme: root.theme
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
                implicitWidth: levelsRow.implicitWidth + 18
                implicitHeight: 28
                radius: root.theme.radiusSmall
                color: levelsArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                          : (levelsArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                Row {
                    id: levelsRow
                    anchors.centerIn: parent
                    spacing: 4

                    AppIcon {
                        theme: root.theme
                        iconKind: "list"
                        useThemeContrast: false
                        primaryColor: root.theme.textPrimary
                        width: 12
                        height: 12
                    }

                    Text {
                        text: root.filterOpen ? qsTr("Levels ▲") : qsTr("Levels ▼")
                        color: root.theme.textPrimary
                        font.pixelSize: 11
                        font.bold: true
                    }
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
                implicitWidth: clearRow.implicitWidth + 18
                implicitHeight: 28
                radius: root.theme.radiusSmall
                color: clearArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                         : (clearArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                Row {
                    id: clearRow
                    anchors.centerIn: parent
                    spacing: 4

                    AppIcon {
                        theme: root.theme
                        iconKind: "trash"
                        useThemeContrast: false
                        primaryColor: root.theme.textPrimary
                        width: 12
                        height: 12
                    }

                    Text {
                        text: qsTr("Clear")
                        color: root.theme.textPrimary
                        font.pixelSize: 11
                        font.bold: true
                    }
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
                    text: qsTr("Output Level")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                }

                Text {
                    width: parent.width
                    text: qsTr("Set the minimum severity for new log messages.")
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
                    text: qsTr("Display Filter")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                }

                Text {
                    width: parent.width
                    text: qsTr("Filter entries already captured in the panel.")
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
            model: filterProxy
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
                            text: eventCard.source
                            color: root.theme.textPrimary
                            elide: Text.ElideRight
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Text {
                            text: eventCard.time
                            color: root.theme.textTertiary
                            font.family: root.theme.monoFontFamily
                            font.pixelSize: 10
                            Layout.alignment: Qt.AlignRight
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: eventCard.message
                        color: root.theme.textPrimary
                        wrapMode: Text.Wrap
                        font.pixelSize: 12
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: eventCard.file.length > 0 || eventCard.threadId > 0
                        text: qsTr("tid %1").arg(eventCard.threadId) + (eventCard.file.length > 0 ? qsTr(" · %1:%2").arg(eventCard.file).arg(eventCard.line) : "")
                        color: root.theme.textTertiary
                        font.pixelSize: 10
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function(mouse) {
                        let parts = [];
                        parts.push("[" + eventCard.time + "]");
                        parts.push("[" + eventCard.displayLevelName + "]");
                        if (eventCard.threadId > 0)
                            parts.push("[tid:" + eventCard.threadId + "]");
                        parts.push("[" + eventCard.source + "]");
                        if (eventCard.file.length > 0)
                            parts.push("[" + eventCard.file + ":" + eventCard.line + "]");
                        parts.push(eventCard.message);
                        const pos = mapToItem(root, mouse.x, mouse.y);
                        logContextMenu.showAt(pos.x, pos.y, parts.join(" "));
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
                    implicitWidth: eventScrollBar.hovered ? 8 : 6
                    implicitHeight: Math.max(26, eventScrollBar.availableHeight * eventScrollBar.visualSize)
                    radius: width / 2
                    color: eventScrollBar.hovered
                        ? root.theme.tint(root.theme.textSecondary, root.theme.darkMode ? 0.6 : 0.4)
                        : root.theme.surfaceStrong

                    Behavior on implicitWidth {
                        NumberAnimation { duration: 120 }
                    }
                    Behavior on color {
                        ColorAnimation { duration: 120 }
                    }
                }
            }
        }
    }
}
