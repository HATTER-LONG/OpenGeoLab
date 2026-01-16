pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file CornerOverlay.qml
 * @brief Bottom-right overlay container (progress + logs)
 */
Item {
    id: root

    property int margin: 20
    property bool logOpen: false
    // Provided by parent (Main.qml), typically the injected context property LogService.
    property var logService

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: margin

    width: 480
    height: logButton.implicitHeight + (progressOverlay.height > 0 ? progressOverlay.height + 8 : 0) + (logOpen ? logPanel.height + 8 : 0)
    z: 1000

    // Progress overlay (bottommost)
    ProgressOverlay {
        id: progressOverlay
        width: root.width
        anchors.right: parent.right
        anchors.bottom: logButton.top
        anchors.bottomMargin: 8
    }

    // Log panel (above progress; sinks down when progress height == 0)
    LogPanel {
        id: logPanel
        width: root.width
        anchors.right: parent.right
        anchors.bottom: progressOverlay.height > 0 ? progressOverlay.top : logButton.top
        anchors.bottomMargin: 8
        open: root.logOpen
        logService: root.logService
        onRequestClose: root.logOpen = false
    }

    // Log button (always at bottom-right)
    AbstractButton {
        id: logButton
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        implicitWidth: 32
        implicitHeight: 32
        hoverEnabled: true

        onClicked: {
            root.logOpen = !root.logOpen;
            if (root.logOpen && root.logService) {
                root.logService.markAllSeen();
                logPanel.scrollToEnd();
            }
        }

        background: Rectangle {
            radius: 6
            color: logButton.hovered ? Theme.hovered : Theme.surface
            border.width: 1
            border.color: Theme.border

            // Error indicator dot
            Rectangle {
                visible: root.logService && root.logService.hasNewErrors
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                width: 8
                height: 8
                radius: 4
                color: Theme.danger

                // Pulse animation for new errors
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: root.logService && root.logService.hasNewErrors
                    NumberAnimation {
                        from: 1.0
                        to: 0.4
                        duration: 800
                    }
                    NumberAnimation {
                        from: 0.4
                        to: 1.0
                        duration: 800
                    }
                }
            }
        }

        contentItem: Text {
            text: "📋"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        ToolTip.visible: hovered
        ToolTip.text: qsTr("View application logs")
        ToolTip.delay: 500
    }
}
