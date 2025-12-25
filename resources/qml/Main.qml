pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
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
            color: Theme.ribbonBackgroundColor
        }
        // Main Content Area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.backgroundColor

            Text {
                text: "Welcome to OpenGeoLab!"
                anchors.centerIn: parent
                font.pointSize: 24
                font.family: Theme.fontFamily
                color: Theme.textPrimaryColor
            }
        }
    }
}
