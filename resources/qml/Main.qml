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

        // 应用级动作落点（后续接入 C++/业务模块时，优先改这里）
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

            // 统一处理所有动作（移植时你只需要带走这一段逻辑）
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
