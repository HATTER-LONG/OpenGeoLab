/**
 * @file RibbonFileMenu.qml
 * @brief File menu popup for the ribbon toolbar
 *
 * Provides file operations (New, Import, Export), theme toggle, and exit.
 * Emits actionTriggered signal for parent handling.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

Popup {
    id: fileMenu
    width: 200
    modal: true
    focus: true
    padding: 14

    /// Emitted when a menu action is triggered
    signal actionTriggered(string actionId, var payload)

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    function trigger(actionId, payload): void {
        fileMenu.close();
        fileMenu.actionTriggered(actionId, payload);
    }
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
            onClicked: {
                OGL.BackendService.request("GeometryService", JSON.stringify({
                    action: "new_model"
                }));
                fileMenu.close();
            }
        }

        RibbonFileMenuButton {
            text: qsTr("Import Model")
            iconSource: "qrc:/opengeolab/resources/icons/import.svg"
            onClicked: fileMenu.trigger("importModel", {})
        }

        RibbonFileMenuButton {
            text: qsTr("Export Model")
            iconSource: "qrc:/opengeolab/resources/icons/export.svg"
            onClicked: fileMenu.trigger("exportModel", {})
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
            onClicked: {
                Theme.toggleMode();
                fileMenu.close();
            }
        }

        RibbonFileMenuButton {
            text: qsTr("Exit")
            iconSource: "qrc:/opengeolab/resources/icons/exit.svg"
            onClicked: Qt.quit()
        }
    }
}
