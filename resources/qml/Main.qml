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
                console.log("[Ribbon] action:", actionId, "payload:", JSON.stringify(payload));

                switch (actionId) {
                case "addBox":
                    break;
                case "importModel":
                    break;
                case "exitApp":
                    Qt.quit();
                    break;
                default:
                    // TODO: 把旧 Main.qml 里对应的 onAddPoint/onGenerateMesh 等逻辑迁移到这里
                    break;
                }
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
