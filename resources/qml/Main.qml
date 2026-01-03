pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

/**
 * @file Main.qml
 * @brief Main application window
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: qsTr("OpenGeoLab")

    // Hello World
    Label {
        anchors.centerIn: parent
        text: qsTr("Hello, OpenGeoLab!")
        font.pixelSize: 24
    }
}
