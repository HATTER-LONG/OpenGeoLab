pragma ComponentBehavior: Bound

import QtQuick
import OpenGeoLab.Services 1.0
import "../theme"
import "../components"

Item {
    id: root

    required property AppTheme theme
    property bool activityOpen: false
    property bool hasNewErrors: false
    property bool hasNewLogs: false

    readonly property real availableWidth: parent ? Math.max(520, parent.width - 24) : 860
    readonly property real availableHeight: parent ? Math.max(420, parent.height - 36) : 760

    width: Math.max(
        Math.min(920, availableWidth),
        activityButton.width + (progressCard.visible ? progressCard.implicitWidth + 8 : 0)
    )
    height: activityButton.height
        + ((activityOpen || activityPanel.opacity > 0.01) ? activityPanel.height + 10 : 0)
    z: 40

    Connections {
        target: LogEventModel

        function onNewEntryAdded(level: int): void {
            if (!root.activityOpen) {
                root.hasNewLogs = true;
                if (level >= 4) {
                    root.hasNewErrors = true;
                }
            }
        }
    }

    onActivityOpenChanged: {
        if (activityOpen) {
            hasNewErrors = false;
            hasNewLogs = false;
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

    ProgressCard {
        id: progressCard

        theme: root.theme
        anchors.right: activityButton.left
        anchors.rightMargin: 8
        anchors.verticalCenter: activityButton.verticalCenter
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
            id: notificationDot
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
                running: notificationDot.visible
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