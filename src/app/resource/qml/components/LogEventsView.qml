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
    property bool filterExpanded: true
    color: "transparent"

    ColumnLayout {
        anchors.fill: parent
        spacing: root.theme.gapTight

        Row {
            Layout.fillWidth: true
            spacing: root.theme.gapTight

            Rectangle {
                id: filterButton
                width: 32; height: 32; radius: 10
                color: filterArea.pressed ? root.theme.surfaceStrong
                                          : (filterArea.containsMouse ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.96) : root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.54 : 0.88))
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.84 : 0.52)

                AppIcon { anchors.centerIn: parent; width: 16; height: 16; theme: root.theme; iconKind: "funnel" }
                MouseArea {
                    id: filterArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.filterExpanded = !root.filterExpanded
                }
            }

            Item {
                width: 16; height: 16
                y: Math.round((filterButton.height - height) / 2)
                AppIcon {
                    anchors.fill: parent
                    theme: root.theme
                    iconKind: "chevronDown"
                    rotation: root.filterExpanded ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 150 } }
                }
            }

            Rectangle {
                width: 32; height: 32; radius: 10
                color: clearArea.pressed ? root.theme.surfaceStrong
                                         : (clearArea.containsMouse ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.96) : root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.54 : 0.88))
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.84 : 0.52)

                AppIcon { anchors.centerIn: parent; width: 16; height: 16; theme: root.theme; iconKind: "trash" }
                MouseArea {
                    id: clearArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.model.clear()
                }

                ToolTip {
                    visible: clearArea.containsMouse
                    delay: 600
                    text: qsTr("Clear")
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: root.theme.gapTight
            visible: root.filterExpanded

            Repeater {
                model: 6
                LogLevelChip {
                    required property int index
                    theme: root.theme
                    text: [qsTr("TRACE"), qsTr("DEBUG"), qsTr("INFO"), qsTr("WARN"), qsTr("ERROR"), qsTr("CRITICAL")][index]
                    accentColor: [root.theme.accentA, root.theme.accentA, root.theme.accentB, root.theme.accentC, root.theme.accentD, root.theme.accentD][index]
                    selected: (root.enabledLevelMask & (1 << index)) !== 0
                    onClicked: root.enabledLevelMask ^= (1 << index)
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

                readonly property color entryAccent: {
                    if (level >= 4) return root.theme.accentD;
                    if (level === 3) return root.theme.accentC;
                    if (level === 2) return root.theme.accentB;
                    return root.theme.accentA;
                }

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
                                text: levelName.toUpperCase()
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
