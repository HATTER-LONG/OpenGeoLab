import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    width: 1500
    height: 900
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: "OpenGeoLab"
    color: "#1a1a2e"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Header placeholder
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            radius: 10
            color: "#16213e"

            Text {
                anchors.centerIn: parent
                text: "OpenGeoLab"
                font.pixelSize: 22
                font.bold: true
                color: "#e2e8f0"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Sidebar placeholder
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                radius: 10
                color: "#16213e"

                Text {
                    anchors.centerIn: parent
                    text: "Sidebar"
                    font.pixelSize: 16
                    color: "#94a3b8"
                }
            }

            // Viewport placeholder
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#0f3460"

                Text {
                    anchors.centerIn: parent
                    text: "3D Viewport\n(Hello World)"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#e94560"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
