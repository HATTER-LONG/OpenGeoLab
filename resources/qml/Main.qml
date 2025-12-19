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
        Rectangle {
            id: ribbonMenu
            width: parent.width
            height: 120
            color: "#2c3e50"
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 20

                // Example Ribbon Buttons
                Button {
                    text: "File"
                    width: 80
                    height: 40
                }
                Button {
                    text: "Edit"
                    width: 80
                    height: 40
                }
                Button {
                    text: "View"
                    width: 80
                    height: 40
                }
                Button {
                    text: "Help"
                    width: 80
                    height: 40
                }
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
