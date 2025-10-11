pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import OpenGeoLab

Window {
    id: root
    visible: true
    width: 960
    height: 600
    title: "OpenGeoLab - 3D Cube Demo"

    // 3D Cube renderer - fills entire window
    Cube3D {
        id: cubeRenderer
        anchors.fill: parent
    }

    // Information overlay
    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.7)
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: "black"
        wrapMode: Text.WordWrap
        text: qsTr("3D Cube rendering demo using OpenGL. The cube rotates automatically with simple lighting effects.")
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
