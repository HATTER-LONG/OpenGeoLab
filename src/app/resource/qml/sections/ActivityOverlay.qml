pragma ComponentBehavior: Bound

import QtQuick
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

    readonly property real availableWidth: parent ? Math.max(520, parent.width - 24) : 860
    readonly property real availableHeight: parent ? Math.max(420, parent.height - 36) : 760

    width: Math.min(920, availableWidth)
    height: activityButton.height
        + (progressBar.visible ? progressBar.height + 10 : 0)
        + ((activityOpen || activityPanel.opacity > 0.01) ? activityPanel.height + 10 : 0)
    z: 40

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

    Rectangle {
        id: progressBar

        width: root.width
        height: 6
        radius: 3
        visible: root.progress >= 0
        color: root.theme.tint(root.theme.borderSubtle, 0.3)
        clip: true
        anchors.right: parent.right
        anchors.bottom: activityPanel.visible || activityPanel.opacity > 0.01
            ? activityPanel.top : activityButton.top
        anchors.bottomMargin: 10

        Rectangle {
            width: root.progress === 0 ? Math.max(parent.width * 0.32, 56) : parent.width * Math.max(0, Math.min(1, root.progress))
            height: parent.height
            radius: parent.radius
            color: root.progressColor
            x: root.progress === 0 ? -width : 0

            NumberAnimation on x {
                running: root.progress === 0 && progressBar.visible
                from: -progressBar.width * 0.32
                to: progressBar.width
                duration: 1200
                loops: Animation.Infinite
            }
        }
    }

    ActivityPanel {
        id: activityPanel

        width: root.width
        theme: root.theme
        availableHeight: root.availableHeight
        visible: root.activityOpen
        opacity: root.activityOpen ? 1 : 0
        anchors.right: parent.right
        anchors.bottom: activityButton.top
        anchors.bottomMargin: 10
        onCloseRequested: root.activityOpen = false

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
        }
    }

    Rectangle {
        id: activityButton

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: buttonRow.implicitWidth + 24
        height: 38
        radius: root.theme.radiusSmall
        color: buttonArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                  : (buttonArea.containsMouse
                                     ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94)
                                     : root.theme.surface)
        border.width: 1
        border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

        Row {
            id: buttonRow

            anchors.centerIn: parent
            spacing: 6

            AppIcon { theme: root.theme; iconKind: "pulse"; width: 14; height: 14 }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Activity")
                font.pixelSize: 11
                font.bold: true
                color: root.theme.textPrimary
            }
        }

        Rectangle {
            visible: root.hasNewErrors || root.hasNewLogs
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 5
            width: 9
            height: 9
            radius: 4.5
            color: root.hasNewErrors ? root.theme.accentD : root.theme.accentB
            z: 1

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: parent.visible
                NumberAnimation { from: 1.0; to: 0.38; duration: 800 }
                NumberAnimation { from: 0.38; to: 1.0; duration: 800 }
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
}
