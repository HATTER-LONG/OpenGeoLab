pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    property var logService: null
    property bool panelActive: false
    property bool filterOpen: false

    function levelTint(level) {
        if (level >= 4) {
            return root.theme.accentD;
        }
        if (level === 3) {
            return root.theme.accentC;
        }
        if (level === 2) {
            return root.theme.accentB;
        }
        return root.theme.accentA;
    }

    function levelLabel(level) {
        switch (level) {
        case 0:
            return qsTr("Trace");
        case 1:
            return qsTr("Debug");
        case 2:
            return qsTr("Info");
        case 3:
            return qsTr("Warn");
        case 4:
            return qsTr("Error");
        case 5:
            return qsTr("Critical");
        default:
            return qsTr("Info");
        }
    }

    function scrollToEnd() {
        Qt.callLater(function () {
            if (listView && listView.count > 0) {
                listView.positionViewAtEnd();
            }
        });
    }

    readonly property int runtimeMinLevel: root.logService ? root.logService.minLevel : 2

    onPanelActiveChanged: {
        if (root.panelActive) {
            root.scrollToEnd();
        }
    }

    Connections {
        target: listView.model
        ignoreUnknownSignals: true

        function onRowsInserted() {
            if (root.panelActive) {
                root.scrollToEnd();
            }
        }

        function onModelReset() {
            if (root.panelActive) {
                root.scrollToEnd();
            }
        }
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

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                implicitWidth: 72
                implicitHeight: 28
                radius: root.theme.radiusSmall
                color: filterMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                               : (filterMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                Text {
                    anchors.centerIn: parent
                    text: root.filterOpen ? qsTr("Levels ▲") : qsTr("Levels ▼")
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    font.family: root.theme.bodyFontFamily
                }

                MouseArea {
                    id: filterMouseArea

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
                color: clearMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                              : (clearMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surfaceMuted)
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Clear")
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    font.family: root.theme.bodyFontFamily
                }

                MouseArea {
                    id: clearMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.logService) {
                            root.logService.clear();
                        }
                    }
                }
            }
        }

        OperationLogFilterPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            visible: root.filterOpen
            theme: root.theme
            logService: root.logService
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: root.theme.radiusSmall
            color: root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.78 : 0.96)
            border.width: 1
            border.color: root.theme.tint(root.theme.borderSubtle, 0.78)
            clip: true

            ListView {
                id: listView

                anchors.fill: parent
                anchors.margins: 8
                spacing: 8
                clip: true
                model: root.logService ? root.logService.model : null
                boundsBehavior: Flickable.StopAtBounds

                delegate: OperationLogEntryDelegate {
                    width: listView.width
                    theme: root.theme
                }
            }

            Text {
                anchors.centerIn: parent
                visible: listView.count === 0
                text: qsTr("No activity matches the current log view.")
                color: root.theme.textTertiary
                font.pixelSize: 12
                font.family: root.theme.bodyFontFamily
            }
        }
    }
}
