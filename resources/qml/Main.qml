pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "RibbonMenu" as RibbonMenu
import "Pages" as Pages

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: qsTr("OpenGeoLab")

    RibbonMenu.DialogHost {
        id: dialogHost
    }

    Pages.ImportModel {
        id: importModelDialog
    }

    RibbonMenu.ActionRouter {
        id: ribbonActions

        dialogHost: dialogHost
        importModelDialog: importModelDialog

        onExitApp: Qt.quit()
    }

    // Listen for backend operation results
    Connections {
        target: OGL.BackendService

        function onOperationFinished(actionId, result) {
            console.log("[Main] Operation finished:", actionId, JSON.stringify(result));
            if (actionId === "read_model" && result.success) {
                resultDialog.title = qsTr("Import Successful");
                resultDialog.message = qsTr("Model loaded successfully!\n\nFile: %1").arg(result.file_path || "");
                resultDialog.isError = false;
                resultDialog.open();
            }
        }

        function onOperationFailed(actionId, error) {
            console.log("[Main] Operation failed:", actionId, error);
            resultDialog.title = qsTr("Operation Failed");
            resultDialog.message = qsTr("Action: %1\nError: %2").arg(actionId).arg(error);
            resultDialog.isError = true;
            resultDialog.open();
        }
    }

    // Result dialog for showing operation outcomes
    Dialog {
        id: resultDialog
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok

        property string message: ""
        property bool isError: false

        width: 400

        background: Rectangle {
            color: Theme.surfaceColor
            radius: 8
            border.width: 1
            border.color: Theme.borderColor
        }

        header: Rectangle {
            width: parent.width
            height: 40
            color: resultDialog.isError ? "#FFEBEE" : "#E8F5E9"
            radius: 8

            Label {
                anchors.centerIn: parent
                text: resultDialog.title
                font.bold: true
                font.pixelSize: 14
                color: resultDialog.isError ? "#C62828" : "#2E7D32"
            }

            // Mask bottom corners
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 8
                color: parent.color
            }
        }

        contentItem: Label {
            text: resultDialog.message
            wrapMode: Text.WordWrap
            padding: 16
            color: Theme.textPrimaryColor
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Ribbon Menu
        RibbonToolBar {
            id: ribbon

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            // Centralized action handling (keeps migration surface small).
            onActionTriggered: (actionId, payload) => {
                ribbonActions.handle(actionId, payload);
            }
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
                color: Theme.textPrimaryColor
            }

            // Progress overlay in bottom-right corner
            RibbonMenu.RibbonProgressOverlay {
                id: progressOverlay
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 16
            }
        }
    }
}
