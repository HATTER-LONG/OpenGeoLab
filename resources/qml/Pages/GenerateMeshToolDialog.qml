pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file GenerateMeshToolDialog.qml
 * @brief Non-modal tool dialog for generating mesh from geometry
 *
 * Allows users to configure and generate a mesh from existing geometry.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Generate Mesh")
    okButtonText: qsTr("Generate")
    preferredContentHeight: 280

    okEnabled: !OGL.BackendService.busy

    onAccepted: {
        const params = {
            algorithm: algorithmCombo.currentText,
            elementSize: parseFloat(elementSizeInput.text) || 1.0,
            quality: qualitySlider.value,
            refineBoundary: refineBoundaryCheck.checked
        };
        OGL.BackendService.request("generateMesh", params);
    }

    ColumnLayout {
        width: parent.width
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Configure mesh generation parameters:")
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
                text: qsTr("Algorithm:")
                color: Theme.textPrimaryColor
            }
            ComboBox {
                id: algorithmCombo
                Layout.fillWidth: true
                model: [qsTr("Delaunay"), qsTr("Frontal"), qsTr("Automatic")]
                enabled: !OGL.BackendService.busy

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: algorithmCombo.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: algorithmCombo.pressed ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }
            }

            Label {
                text: qsTr("Element Size:")
                color: Theme.textPrimaryColor
            }
            TextField {
                id: elementSizeInput
                Layout.fillWidth: true
                placeholderText: "1.0"
                text: "1.0"
                validator: DoubleValidator {
                    bottom: 0.001
                }
                enabled: !OGL.BackendService.busy
            }

            Label {
                text: qsTr("Quality:")
                color: Theme.textPrimaryColor
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Slider {
                    id: qualitySlider
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: 0.7
                    stepSize: 0.1
                    enabled: !OGL.BackendService.busy
                }

                Label {
                    text: qualitySlider.value.toFixed(1)
                    color: Theme.textSecondaryColor
                    Layout.preferredWidth: 30
                }
            }
        }

        CheckBox {
            id: refineBoundaryCheck
            text: qsTr("Refine boundary")
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
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "GenerateMesh") {
                root.closeRequested();
            }
        }
    }
}
