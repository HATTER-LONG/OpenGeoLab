pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var appController
    property bool activityOpen: false
    readonly property real availableWidth: parent ? Math.max(520, parent.width - 24) : 860
    readonly property real availableHeight: parent ? Math.max(420, parent.height - 36) : 760

    width: Math.min(920, availableWidth)
    height: activityButton.height
        + (progressOverlay.visible ? progressOverlay.height + 10 : 0)
        + ((activityOpen || activityPanel.opacity > 0.01) ? activityPanel.height + 10 : 0)
    z: 40

    OperationProgressOverlay {
        id: progressOverlay

        width: root.width
        theme: root.theme
        appController: root.appController
        anchors.right: parent.right
        anchors.bottom: activityButton.top
        anchors.bottomMargin: 10
    }

    OperationLogPanel {
        id: activityPanel

        width: root.width
        theme: root.theme
        appController: root.appController
        availableHeight: root.availableHeight
        open: root.activityOpen
        anchors.right: parent.right
        anchors.bottom: progressOverlay.visible ? progressOverlay.top : activityButton.top
        anchors.bottomMargin: 10
        onRequestClose: root.activityOpen = false
    }

    Rectangle {
        id: activityButton

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 112
        height: 38
        radius: root.theme.radiusSmall
        color: activityMouseArea.pressed ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.88 : 0.98)
                                          : (activityMouseArea.containsMouse ? root.theme.tint(root.theme.surfaceStrong, root.theme.darkMode ? 0.78 : 0.94) : root.theme.surface)
        border.width: 1
        border.color: root.theme.tint(root.theme.borderSubtle, root.theme.darkMode ? 0.8 : 0.44)

        Row {
            anchors.centerIn: parent
            spacing: 6

            AppIcon {
                width: 14
                height: 14
                theme: root.theme
                iconKind: "record"
                primaryColor: root.theme.textPrimary
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Activity")
                color: root.theme.textPrimary
                font.pixelSize: 11
                font.bold: true
                font.family: root.theme.bodyFontFamily
            }
        }

        Rectangle {
            visible: root.appController && (root.appController.hasUnreadOperationErrors || root.appController.hasUnreadOperationLogs)
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 5
            width: 9
            height: 9
            radius: 4.5
            color: root.appController && root.appController.hasUnreadOperationErrors
                ? root.theme.accentD
                : root.theme.accentB

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: parent.visible
                NumberAnimation {
                    from: 1.0
                    to: 0.38
                    duration: 800
                }
                NumberAnimation {
                    from: 0.38
                    to: 1.0
                    duration: 800
                }
            }
        }

        MouseArea {
            id: activityMouseArea

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.activityOpen = !root.activityOpen;
                if (root.activityOpen && root.appController) {
                    root.appController.markOperationLogSeen();
                }
            }
        }
    }
}
