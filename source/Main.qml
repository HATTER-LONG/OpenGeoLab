import QtQuick
import QtQuick.Controls
import "widgets"

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Rectangle { // our inlined button ui
        id: button
        anchors {
            top: parent.top
            left: parent.left
            margins: 12
        }
        width: 116
        height: 26
        color: "lightsteelblue"
        border.color: "slategrey"
        Text {
            anchors.centerIn: parent
            text: "Start"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                status.text = "Button clicked!";
            }
        }
    }

    Button { // our custom button component
        id: customButton
        anchors {
            top: button.bottom
            left: parent.left
            margins: 12
        }
        text: "Custom"
        onClicked: {
            status.text = "Custom Button clicked!";
        }
    }

    Text { // text changes when button was clicked
        id: status
        anchors {
            top: customButton.bottom
            left: parent.left
            margins: 12
        }
        width: 116
        height: 26
        text: "waiting ..."
        horizontalAlignment: Text.AlignHCenter
    }
}
