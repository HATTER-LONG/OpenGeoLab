pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: fileMenu
    width: 200
    modal: true
    focus: true
    padding: 14

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: 8
        color: Theme.ribbonFileMenuBackground
        border.color: Theme.border
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 6

        RibbonFileMenuButton {
            text: qsTr("New Model")
            iconSource: "qrc:/opengeolab/resources/icons/new.svg"
            onClicked: console.log("New Model")
        }

        RibbonFileMenuButton {
            text: qsTr("Import Model")
            iconSource: "qrc:/opengeolab/resources/icons/import.svg"
            onClicked: console.log("Import Model")
        }

        RibbonFileMenuButton {
            text: qsTr("Export Model")
            iconSource: "qrc:/opengeolab/resources/icons/export.svg"
            onClicked: console.log("Export Model")
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.palette.midlight
            opacity: 0.5
        }

        RibbonFileMenuButton {
            text: qsTr("Theme Switch")
            iconSource: Theme.isDark ? "qrc:/opengeolab/resources/icons/dark.svg" : "qrc:/opengeolab/resources/icons/light.svg"
            onClicked: Theme.toggleMode()
        }

        RibbonFileMenuButton {
            text: qsTr("Exit")
            iconSource: "qrc:/opengeolab/resources/icons/exit.svg"
            onClicked: Qt.quit()
        }
    }
}
