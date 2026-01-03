pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import OpenGeoLab 1.0

Rectangle {
    id: ribbonToolBar
    width: parent.width
    height: 120
    color: Theme.ribbonBackground

    RibbonFileMenu {
        id: fileMenu
        x: ribbonToolBar.x + 4
        y: ribbonToolBar.y + tabBar.height + 4
    }
    Rectangle {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28
        color: Theme.ribbonTabBackground

        RowLayout {
            id: tabLayout
            anchors.fill: parent
            anchors.margins: 4
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 60
                Layout.fillHeight: true
                radius: 2
                color: "transparent"

                Text {
                    anchors.centerIn: parent
                    text: qsTr("File")
                    font.pixelSize: 12
                    font.bold: true
                    color: Theme.textPrimary
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.color = Theme.ribbonHoverColor
                    onExited: parent.color = "transparent"
                    onClicked: fileMenu.open()
                }
            }
        }
    }
}
