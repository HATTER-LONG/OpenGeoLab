pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "." as App

ApplicationWindow {
    id: root
    visible: true
    width: Screen.width
    height: Screen.height
    title: qsTr("OpenGeoLab")

    App.RibbonActions {
        id: ribbonActions
    }

    Column {
        anchors.fill: parent

        // Ribbon Menu
        RibbonMenu {
            id: ribbonMenu
            width: parent.width
            actions: ribbonActions.defaultActions
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
