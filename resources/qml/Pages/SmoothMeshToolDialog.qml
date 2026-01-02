pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file SmoothMeshToolDialog.qml
 * @brief Non-modal tool dialog for smoothing mesh
 *
 * Allows users to configure and apply mesh smoothing operations.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Smooth Mesh")
    okButtonText: qsTr("Apply")

    okEnabled: !OGL.BackendService.busy

    onAccepted: {
        const params = {
            iterations: iterationsInput.value,
            factor: parseFloat(factorInput.text) || 0.5,
            preserveBoundary: preserveBoundaryCheck.checked
        };
        OGL.BackendService.request("smoothMesh", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Configure mesh smoothing parameters:")
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
                text: qsTr("Iterations:")
                color: Theme.textPrimaryColor
            }
            SpinBox {
                id: iterationsInput
                Layout.fillWidth: true
                from: 1
                to: 100
                value: 3
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Smoothing Factor:")
                color: Theme.textPrimaryColor
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Slider {
                    id: factorSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: 0.5
                    stepSize: 0.1
                    enabled: !OGL.BackendService.busy
                }

                TextField {
                    id: factorInput
                    Layout.preferredWidth: 60
                    text: factorSlider.value.toFixed(2)
                    validator: DoubleValidator { bottom: 0; top: 1 }
                    enabled: !OGL.BackendService.busy
                    onTextChanged: {
                        const v = parseFloat(text);
                        if (!isNaN(v) && v >= 0 && v <= 1) {
                            factorSlider.value = v;
                        }
                    }
                }
            }
        }

        CheckBox {
            id: preserveBoundaryCheck
            text: qsTr("Preserve boundary")
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

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "SmoothMesh") {
                root.closeRequested();
            }
        }
    }
}
