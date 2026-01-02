pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file AddPointToolDialog.qml
 * @brief Non-modal tool dialog for adding a point with X, Y, Z coordinates
 *
 * Allows users to create a new point geometry by specifying 3D coordinates.
 * The dialog is non-modal, enabling viewport interaction during input.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Add Point")
    okButtonText: qsTr("Create")
    preferredContentHeight: 200

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
        width: parent.width
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Enter the coordinates for the new point:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
            font.pixelSize: 12
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            Label {
                text: qsTr("X:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 40
            }
            TextField {
                id: xInput
                Layout.fillWidth: true
                placeholderText: "0.0"
                text: root.initialParams.x !== undefined ? String(root.initialParams.x) : "0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
                color: Theme.textPrimaryColor
                placeholderTextColor: Theme.textSecondaryColor
                background: Rectangle {
                    implicitHeight: 32
                    radius: 6
                    color: xInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: xInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                }
            }

            Label {
                text: qsTr("Y:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 40
            }
            TextField {
                id: yInput
                Layout.fillWidth: true
                placeholderText: "0.0"
                text: root.initialParams.y !== undefined ? String(root.initialParams.y) : "0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
                color: Theme.textPrimaryColor
                placeholderTextColor: Theme.textSecondaryColor
                background: Rectangle {
                    implicitHeight: 32
                    radius: 6
                    color: yInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: yInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                }
            }

            Label {
                text: qsTr("Z:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 40
            }
            TextField {
                id: zInput
                Layout.fillWidth: true
                placeholderText: "0.0"
                text: root.initialParams.z !== undefined ? String(root.initialParams.z) : "0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
                color: Theme.textPrimaryColor
                placeholderTextColor: Theme.textSecondaryColor
                background: Rectangle {
                    implicitHeight: 32
                    radius: 6
                    color: zInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: zInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                }
            }
        }

        // Status area
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                running: true
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "addPoint") {
                root.closeRequested();
            }
        }
    }
}
