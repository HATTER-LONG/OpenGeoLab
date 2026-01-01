pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for generating mesh from geometry.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("Generate Mesh")
    okButtonText: qsTr("Generate")

    property var initialParams: ({})

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
        anchors.fill: parent
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Configure mesh generation parameters:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
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

            Label {
                text: qsTr("Options:")
                color: Theme.textPrimaryColor
            }
            CheckBox {
                id: refineBoundaryCheck
                text: qsTr("Refine boundary")
                checked: true
                enabled: !OGL.BackendService.busy
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Status area.
        RowLayout {
            Layout.fillWidth: true
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
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "GenerateMesh")
                root.closeRequested();
        }
    }
}
