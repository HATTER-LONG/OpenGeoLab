pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: Screen.width
    height: Screen.height
    title: qsTr("OpenGeoLab")

    Column {
        anchors.fill: parent

        // Ribbon Menu
        RibbonMenu {
            id: ribbonMenu
            width: parent.width

            onFileClicked: {
                console.log("File menu clicked");
            }
            onEditClicked: {
                console.log("Edit menu clicked");
            }
            onViewClicked: {
                console.log("View menu clicked");
            }
            onHelpClicked: {
                console.log("Help menu clicked");
            }
        }
        // Main Content Area
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: ribbonMenu.bottom
            anchors.bottom: parent.bottom
            color: "white"

            Text {
                text: "Welcome to OpenGeoLab!"
                anchors.centerIn: parent
                font.pointSize: 24
                color: "black"
            }
        }
    }
}
