pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import OpenGeoLab 1.0
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

/**
 * @file Main.qml
 * @brief Main application window
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: qsTr("OpenGeoLab")

    palette: Theme.palette

    header: RibbonMenu.RibbonToolBar {
        id: ribbonToolBar
        onActionTriggered: (actionId, payload) => {
            MainPages.handleAction(actionId, payload);
        }
    }

    // Hello World
    Label {
        anchors.centerIn: parent
        text: qsTr("Hello, OpenGeoLab!")
        font.pixelSize: 24
    }

    // Bottom-right corner container for overlay widgets
    Item {
        id: bottomRightContainer
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        width: 320
        height: childrenRect.height
        z: 1000

        // Progress overlay
        Pages.ProgressOverlay {
            id: progressOverlay
            anchors.right: parent.right
            anchors.bottom: logButton.top
            anchors.bottomMargin: 8
        }

        // Log button
        AbstractButton {
            id: logButton
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            implicitWidth: 32
            implicitHeight: 32
            hoverEnabled: true

            background: Rectangle {
                radius: 6
                color: logButton.hovered ? Theme.hovered : Theme.surface
                border.width: 1
                border.color: Theme.border

                // Error indicator dot
                Rectangle {
                    visible: LogService.hasNewErrors
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
                        running: LogService.hasNewErrors
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

            // onClicked: logWindow.open()

            ToolTip.visible: hovered
            ToolTip.text: qsTr("View application logs")
            ToolTip.delay: 500
        }
    }

    // Log window popup
    // Pages.LogWindow {
    //     id: logWindow
    // }
}
