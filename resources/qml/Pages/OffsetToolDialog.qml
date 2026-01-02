pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file OffsetToolDialog.qml
 * @brief Non-modal tool dialog for offsetting geometry
 *
 * Allows users to offset geometry by a specified distance and direction.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Offset")
    okButtonText: qsTr("Apply")

    okEnabled: !OGL.BackendService.busy && !isNaN(parseFloat(distanceInput.text))

    onAccepted: {
        const params = {
            distance: parseFloat(distanceInput.text) || 0,
            direction: directionCombo.currentText,
            keepOriginal: keepOriginalCheck.checked
        };
        OGL.BackendService.request("offset", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Configure offset parameters:")
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
                text: qsTr("Distance:")
                color: Theme.textPrimaryColor
            }
            TextField {
                id: distanceInput
                Layout.fillWidth: true
                placeholderText: "1.0"
                text: "1.0"
                validator: DoubleValidator {}
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Direction:")
                color: Theme.textPrimaryColor
            }
            ComboBox {
                id: directionCombo
                Layout.fillWidth: true
                model: [qsTr("Normal"), qsTr("Inward"), qsTr("Outward")]
                enabled: !OGL.BackendService.busy

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: directionCombo.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: directionCombo.pressed ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }
            }
        }

        CheckBox {
            id: keepOriginalCheck
            text: qsTr("Keep original geometry")
            checked: true
            enabled: !OGL.BackendService.busy
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

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "Offset") {
                root.closeRequested();
            }
        }
    }
}
