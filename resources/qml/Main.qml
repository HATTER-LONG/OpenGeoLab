pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: qsTr("OpenGeoLab")

    RibbonActionRouter {
        id: ribbonActions

        onExitApp: Qt.quit()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Ribbon Menu
        RibbonToolBar {
            id: ribbon

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            // Centralized action handling (keeps migration surface small).
            onActionTriggered: (actionId, payload) => {
                ribbonActions.handle(actionId, payload);
            }
        }
        // Main Content Area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.backgroundColor

            Text {
                text: "Welcome to OpenGeoLab!"
                anchors.centerIn: parent
                font.pointSize: 24
                color: Theme.textPrimaryColor
            }
        }
    }
}
