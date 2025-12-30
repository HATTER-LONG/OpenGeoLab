pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for adding a point with X, Y, Z coordinates.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("Add Point")
    okButtonText: qsTr("Create")

    property var initialParams: ({})

    // Validation: all coordinates must be valid numbers.
    okEnabled: !OGL.BackendService.busy && !isNaN(parseFloat(xInput.text)) && !isNaN(parseFloat(yInput.text)) && !isNaN(parseFloat(zInput.text))

    onAccepted: {
        const params = {
            x: parseFloat(xInput.text),
            y: parseFloat(yInput.text),
            z: parseFloat(zInput.text)
        };
        OGL.BackendService.request("addPoint", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Enter the coordinates for the new point:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            Label {
                text: qsTr("X:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 60
            }
            TextField {
                id: xInput
                Layout.fillWidth: true
                placeholderText: "0.0"
                text: root.initialParams.x !== undefined ? String(root.initialParams.x) : "0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Y:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 60
            }
            TextField {
                id: yInput
                Layout.fillWidth: true
                placeholderText: "0.0"
                text: root.initialParams.y !== undefined ? String(root.initialParams.y) : "0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Z:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 60
            }
            TextField {
                id: zInput
                Layout.fillWidth: true
                placeholderText: "0.0"
                text: root.initialParams.z !== undefined ? String(root.initialParams.z) : "0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
            }
        }

        // Status area.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20

            RowLayout {
                anchors.fill: parent
                spacing: 8
                visible: OGL.BackendService.busy

                BusyIndicator {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    running: true
                }

                Label {
                    Layout.fillWidth: true
                    text: OGL.BackendService.message
                    color: Theme.textSecondaryColor
                    elide: Text.ElideRight
                }
            }

            Label {
                anchors.fill: parent
                visible: OGL.BackendService.lastError.length > 0 && !OGL.BackendService.busy
                text: OGL.BackendService.lastError
                color: "#E74C3C"
                wrapMode: Text.Wrap
            }
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(actionId, _result) {
            if (actionId === "addPoint")
                root.closeRequested();
        }
    }
}
