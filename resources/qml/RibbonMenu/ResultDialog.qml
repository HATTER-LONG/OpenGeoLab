pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

/**
 * Result dialog for displaying operation outcomes.
 * Only shows when result.show is true.
 */
Dialog {
    id: root
    anchors.centerIn: parent
    modal: true
    padding: 0

    // Auto-size based on content
    width: 420
    height: contentColumn.implicitHeight + headerRect.height + footerRect.height

    property string dialogMessage: ""
    property bool isError: false
    property var resultData: ({})

    // Listen for backend operation results
    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, result: var): void {
            console.log("[ResultDialog] Operation finished:", moduleName, JSON.stringify(result));
            // Only show if result.show is true
            if (result.show !== true) {
                return;
            }
            root.resultData = result;
            root.isError = false;
            root.title = result.title || qsTr("Operation Successful");
            root.dialogMessage = result.message || qsTr("Operation completed successfully.");

            // Load model data if this is a successful import
            if (moduleName === "ModelReader" && result.success === true) {
                OGL.ModelManager.loadFromResult(result);
            }

            root.open();
        }

        function onOperationFailed(moduleName: string, error: string): void {
            console.log("[ResultDialog] Operation failed:", moduleName, error);
            root.resultData = {
                moduleName: moduleName,
                error: error
            };
            root.isError = true;
            root.title = qsTr("Operation Failed");
            root.dialogMessage = qsTr("Module: %1\nError: %2").arg(moduleName).arg(error);
            root.open();
        }
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: 12
        border.width: 1
        border.color: Theme.borderColor
    }

    header: Rectangle {
        id: headerRect
        width: parent ? parent.width : 420
        height: 48
        color: root.isError ? Theme.dialogErrorColor : Theme.dialogSuccessColor
        radius: 12

        // Square bottom corners
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 8

            Label {
                text: root.isError ? "✕" : "✓"
                font.pixelSize: 18
                font.bold: true
                color: "#FFFFFF"
            }

            Label {
                Layout.fillWidth: true
                text: root.title
                font.bold: true
                font.pixelSize: 15
                color: "#FFFFFF"
                elide: Text.ElideRight
            }
        }
    }

    contentItem: ColumnLayout {
        id: contentColumn
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.margins: 16
            text: root.dialogMessage
            wrapMode: Text.WordWrap
            color: Theme.textPrimaryColor
            font.pixelSize: 13
        }

        // Details section
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.preferredHeight: detailsCol.implicitHeight + 16
            color: Theme.dialogDetailsBgColor
            radius: 8
            visible: root.resultData.details !== undefined

            ColumnLayout {
                id: detailsCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Repeater {
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
                        id: detailRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: detailRow.modelData.key + ":"
                            font.bold: true
                            font.pixelSize: 12
                            color: Theme.textSecondaryColor
                            Layout.preferredWidth: 60
                        }

                        Label {
                            Layout.fillWidth: true
                            text: String(detailRow.modelData.value)
                            font.pixelSize: 12
                            color: Theme.textPrimaryColor
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }

        Item {
            Layout.preferredHeight: 8
        }
    }

    footer: Rectangle {
        id: footerRect
        width: parent ? parent.width : 420
        height: 60
        color: Theme.surfaceAltColor
        radius: 12

        // Square top corners
        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 12
            color: parent.color
        }

        // Separator
        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: Theme.borderColor
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12

            Item {
                Layout.fillWidth: true
            }

            Button {
                id: okButton
                text: qsTr("OK")

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 36
                    radius: 8
                    color: okButton.pressed ? (root.isError ? Theme.dialogErrorPressedColor : Theme.dialogSuccessPressedColor) : (okButton.hovered ? (root.isError ? Theme.dialogErrorHoverColor : Theme.dialogSuccessHoverColor) : (root.isError ? Theme.dialogErrorColor : Theme.dialogSuccessColor))

                    Behavior on color {
                        ColorAnimation {
                            duration: 100
                        }
                    }
                }

                contentItem: Text {
                    text: okButton.text
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.close()
            }
        }
    }
}
