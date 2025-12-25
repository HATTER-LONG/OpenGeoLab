pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenGeoLab 1.0

ApplicationWindow {
    id: root
    visible: true
    width: Screen.width
    height: Screen.height
    title: qsTr("OpenGeoLab")

    RibbonActions {
        id: ribbonActions
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Ribbon Menu
        RibbonMenu {
            id: ribbonMenu
            Layout.fillWidth: true
            actions: ribbonActions.defaultActions
        }
        // Main Content Area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.ribbonBackgroundColor

            Text {
                text: "Welcome to OpenGeoLab!"
                anchors.centerIn: parent
                font.pointSize: 24
                color: Theme.ribbonBackgroundColor
            }
        }
    }
}
