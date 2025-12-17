import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: qsTr("OpenGeoLab")

    Rectangle {
        anchors.fill: parent
        color: "lightgray"

        Text {
            text: "Welcome to OpenGeoLab!"
            anchors.centerIn: parent
            font.pointSize: 24
            color: "black"
        }
    }
}
