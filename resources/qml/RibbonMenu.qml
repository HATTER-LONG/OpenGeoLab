import QtQuick
import QtQuick.Controls

Rectangle {
    id: ribbonMenu
    width: parent.width
    height: 120
    color: "#2c3e50"

    signal fileClicked
    signal editClicked
    signal viewClicked
    signal helpClicked
    Row {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20

        Button {
            text: "File"
            width: 80
            height: 40
            onClicked: ribbonMenu.fileClicked()
        }
        Button {
            text: "Edit"
            width: 80
            height: 40
            onClicked: ribbonMenu.editClicked()
        }
        Button {
            text: "View"
            width: 80
            height: 40
            onClicked: ribbonMenu.viewClicked()
        }
        Button {
            text: "Help"
            width: 80
            height: 40
            onClicked: ribbonMenu.helpClicked()
        }
    }
}
