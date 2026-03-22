pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: root

    required property AppTheme theme
    property bool activityOpen: false
    property bool hasNewErrors: false
    property bool hasNewLogs: false
    property real progress: -1
    property string progressStatus: ""

    implicitWidth: Math.min(920, parent ? parent.width * 0.5 : 460)
    implicitHeight: overlayColumn.implicitHeight
    width: implicitWidth
    height: implicitHeight

    readonly property color progressColor: root.progressStatus === "Done" ? root.theme.success
                                                   : (root.progressStatus === "Failed" ? root.theme.danger : root.theme.accentA)

    Timer {
        id: progressHideTimer
        repeat: false
        onTriggered: root.progress = -1
    }

    onProgressStatusChanged: {
        if (root.progressStatus === "Done") {
            progressHideTimer.interval = 3000;
            progressHideTimer.restart();
        } else if (root.progressStatus === "Failed") {
            progressHideTimer.interval = 6000;
            progressHideTimer.restart();
        } else {
            progressHideTimer.stop();
        }
    }

    ColumnLayout {
        id: overlayColumn

        anchors.fill: parent
        spacing: root.theme.gap

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 6
            radius: 3
            visible: root.progress >= 0
            color: root.theme.tint(root.theme.borderSubtle, 0.3)
            clip: true

            Rectangle {
                width: root.progress === 0 ? Math.max(parent.width * 0.32, 56) : parent.width * Math.max(0, Math.min(1, root.progress))
                height: parent.height
                radius: parent.radius
                color: root.progressColor
                x: root.progress === 0 ? -width : 0

                NumberAnimation on x {
                    running: root.progress === 0 && parent.visible
                    from: -parent.width * 0.32
                    to: parent.width
                    duration: 1200
                    loops: Animation.Infinite
                }
            }
        }

        ActivityPanel {
            id: activityPanel

            Layout.fillWidth: true
            Layout.topMargin: root.activityOpen ? 0 : 10
            visible: root.activityOpen
            opacity: root.activityOpen ? 1 : 0
            theme: root.theme
            onCloseRequested: root.activityOpen = false

            Behavior on opacity {
                NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
            }
            Behavior on Layout.topMargin {
                NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
            }
        }

        Item {
            Layout.alignment: Qt.AlignRight
            implicitWidth: activityButton.width + 4
            implicitHeight: activityButton.height + 4

            Rectangle {
                id: activityButton

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: buttonRow.implicitWidth + 20
                height: 38
                radius: root.theme.radiusMedium
                color: buttonArea.pressed ? root.theme.surfaceStrong
                                          : (buttonArea.containsMouse
                                             ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.84 : 0.96)
                                             : root.theme.tint(root.theme.surface, root.theme.darkMode ? 0.72 : 0.94))
                border.width: 1
                border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.84 : 0.52)

                Row {
                    id: buttonRow

                    anchors.centerIn: parent
                    spacing: 8

                    AppIcon { theme: root.theme; iconKind: "pulse"; width: 18; height: 18 }

                    Text {
                        text: qsTr("Activity")
                        font.pixelSize: 12
                        color: root.theme.textPrimary
                    }
                }

                MouseArea {
                    id: buttonArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.activityOpen = !root.activityOpen
                }
            }

            Rectangle {
                width: 8
                height: 8
                radius: 4
                anchors.top: activityButton.top
                anchors.right: activityButton.right
                anchors.topMargin: -2
                anchors.rightMargin: -2
                visible: root.hasNewErrors || root.hasNewLogs
                color: root.hasNewErrors ? root.theme.danger : root.theme.accentB
                z: 1

                SequentialAnimation on opacity {
                    id: pulseAnimation
                    loops: Animation.Infinite
                    running: root.hasNewErrors || root.hasNewLogs
                    NumberAnimation { to: 0.38; duration: 800 }
                    NumberAnimation { to: 1.0; duration: 800 }
                }
            }
        }
    }
}
