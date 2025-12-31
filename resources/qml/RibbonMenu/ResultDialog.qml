pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

/**
 * Generic result dialog for displaying operation outcomes.
 * Automatically listens to BackendService signals.
 */
Dialog {
    id: root
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Ok
    width: 420

    property string dialogMessage: ""
    property bool isError: false
    property var resultData: ({})

    // Listen for backend operation results
    Connections {
        target: OGL.BackendService

        function onOperationFinished(actionId: string, result: var): void {
            console.log("[ResultDialog] Operation finished:", actionId, JSON.stringify(result));
            root.resultData = result;
            root.isError = false;
            root.title = result.title || qsTr("Operation Successful");
            root.dialogMessage = result.message || qsTr("Operation completed successfully.");
            root.open();
        }

        function onOperationFailed(actionId: string, error: string): void {
            console.log("[ResultDialog] Operation failed:", actionId, error);
            root.resultData = {
                action_id: actionId,
                error: error
            };
            root.isError = true;
            root.title = qsTr("Operation Failed");
            root.dialogMessage = qsTr("Action: %1\nError: %2").arg(actionId).arg(error);
            root.open();
        }
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: 8
        border.width: 1
        border.color: Theme.borderColor
    }

    header: Rectangle {
        width: parent ? parent.width : 0
        height: 44
        color: root.isError ? "#FFEBEE" : "#E8F5E9"
        radius: 8

        Label {
            anchors.centerIn: parent
            text: root.title
            font.bold: true
            font.pixelSize: 14
            color: root.isError ? "#C62828" : "#2E7D32"
        }

        // Mask bottom corners
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 8
            color: parent.color
        }
    }

    contentItem: ColumnLayout {
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: root.dialogMessage
            wrapMode: Text.WordWrap
            padding: 8
            color: Theme.textPrimaryColor
        }

        // Show additional details if available
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: detailsColumn.height + 16
            color: Theme.mode === Theme.dark ? "#2A2A2A" : "#F5F5F5"
            radius: 4
            visible: root.resultData.details !== undefined

            ColumnLayout {
                id: detailsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                spacing: 4

                Repeater {
                    id: detailsRepeater
                    model: {
                        if (!root.resultData.details)
                            return [];
                        const details = root.resultData.details;
                        return Object.keys(details).map(key => ({
                                    key: key,
                                    value: details[key]
                                }));
                    }

                    delegate: RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: modelData.key + ":"
                            font.bold: true
                            font.pixelSize: 12
                            color: Theme.textSecondaryColor
                        }

                        Label {
                            Layout.fillWidth: true
                            text: String(modelData.value)
                            font.pixelSize: 12
                            color: Theme.textPrimaryColor
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }
    }
}
