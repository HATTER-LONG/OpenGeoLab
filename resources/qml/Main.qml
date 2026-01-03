pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

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

    palette: Theme.palette

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                text: qsTr("OpenGeoLab")
                font.pixelSize: 14
                elide: Label.ElideRight
                Layout.fillWidth: true
            }

            Button {
                text: Theme.isDark ? qsTr("浅色") : qsTr("深色")
                onClicked: Theme.toggleMode()
            }
        }
    }

    // Hello World
    Label {
        anchors.centerIn: parent
        text: qsTr("Hello, OpenGeoLab!")
        font.pixelSize: 24
    }
}
