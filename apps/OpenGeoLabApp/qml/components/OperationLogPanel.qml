pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var appController
    property bool open: false
    property int currentTab: 0
    property real availableHeight: 760
    readonly property var logService: root.appController ? root.appController.operationLogService : null
    readonly property var tabs: [
        { "title": qsTr("Events") },
        { "title": qsTr("Command Line") }
    ]
    readonly property int minimumHeight: 360
    readonly property int maximumHeight: Math.max(minimumHeight, Math.min(Math.floor(availableHeight), 760))
    readonly property int resolvedHeight: maximumHeight
    signal requestClose

    implicitWidth: 840
    height: resolvedHeight
    visible: open || opacity > 0.01
    opacity: open ? 1 : 0
    y: open ? 0 : 10

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Behavior on y {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    onOpenChanged: {
        if (root.open && root.logService && root.currentTab === 0) {
            root.logService.markAllSeen();
        }
    }

    onCurrentTabChanged: {
        if (root.open && root.logService && root.currentTab === 0) {
            root.logService.markAllSeen();
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.97 : 1.0)
        border.width: 1
        border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.82 : 0.94)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("Activity Center")
                color: root.theme.textPrimary
                font.pixelSize: 14
                font.bold: true
                font.family: root.theme.titleFontFamily
                Layout.fillWidth: true
            }

            StatChip {
                visible: root.appController && root.appController.hasUnreadOperationErrors
                theme: root.theme
                text: qsTr("New errors")
                tintColor: root.theme.accentD
            }

            StatChip {
                visible: root.appController && !root.appController.hasUnreadOperationErrors && root.appController.hasUnreadOperationLogs
                theme: root.theme
                text: qsTr("New activity")
                tintColor: root.theme.accentB
            }

            Rectangle {
                implicitWidth: 28
                implicitHeight: 28
                radius: root.theme.radiusSmall
                color: closeMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                             : (closeMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : "transparent")
                border.width: closeMouseArea.containsMouse ? 1 : 0
                border.color: root.theme.tint(root.theme.borderSubtle, 0.82)

                Text {
                    anchors.centerIn: parent
                    text: "\u00d7"
                    color: root.theme.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                    font.family: root.theme.bodyFontFamily
                }

                MouseArea {
                    id: closeMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestClose()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: root.tabs

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: root.theme.radiusSmall
                    color: root.currentTab === index
                        ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.24 : 0.12)
                        : root.theme.surfaceMuted
                    border.width: 1
                    border.color: root.currentTab === index
                        ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.56 : 0.3)
                        : root.theme.tint(root.theme.borderSubtle, 0.75)

                    Text {
                        anchors.centerIn: parent
                        text: modelData.title
                        color: root.theme.textPrimary
                        font.pixelSize: 11
                        font.bold: root.currentTab === index
                        font.family: root.theme.bodyFontFamily
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentTab = index
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentTab

            OperationLogEventsTab {
                theme: root.theme
                logService: root.logService
                panelActive: root.open && root.currentTab === 0
            }

            OperationCommandLineTab {
                theme: root.theme
                appController: root.appController
                panelActive: root.open && root.currentTab === 1
            }
        }
    }
}
