pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for adding a box with origin, dimensions, and optional name.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("Add Box")
    okButtonText: qsTr("Create")

    property var initialParams: ({})

    okEnabled: !OGL.BackendService.busy && parseFloat(widthInput.text) > 0 && parseFloat(heightInput.text) > 0 && parseFloat(depthInput.text) > 0

    onAccepted: {
        const params = {
            name: nameInput.text.trim() || "Box",
            originX: parseFloat(originXInput.text) || 0,
            originY: parseFloat(originYInput.text) || 0,
            originZ: parseFloat(originZInput.text) || 0,
            width: parseFloat(widthInput.text) || 1,
            height: parseFloat(heightInput.text) || 1,
            depth: parseFloat(depthInput.text) || 1
        };
        OGL.BackendService.request("addBox", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Name input.
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("Name:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 60
            }
            TextField {
                id: nameInput
                Layout.fillWidth: true
                placeholderText: qsTr("Box")
                text: ""
                enabled: !OGL.BackendService.busy
            }
        }

        // Origin section.
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Origin")

            GridLayout {
                anchors.fill: parent
                columns: 6
                columnSpacing: 8
                rowSpacing: 8

                Label {
                    text: "X:"
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: originXInput
                    Layout.fillWidth: true
                    text: "0"
                    validator: DoubleValidator {}
                    enabled: !OGL.BackendService.busy
                }

                Label {
                    text: "Y:"
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: originYInput
                    Layout.fillWidth: true
                    text: "0"
                    validator: DoubleValidator {}
                    enabled: !OGL.BackendService.busy
                }

                Label {
                    text: "Z:"
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: originZInput
                    Layout.fillWidth: true
                    text: "0"
                    validator: DoubleValidator {}
                    enabled: !OGL.BackendService.busy
                }
            }
        }

        // Dimensions section.
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Dimensions")

            GridLayout {
                anchors.fill: parent
                columns: 6
                columnSpacing: 8
                rowSpacing: 8

                Label {
                    text: qsTr("W:")
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: widthInput
                    Layout.fillWidth: true
                    text: "10"
                    validator: DoubleValidator {
                        bottom: 0.001
                    }
                    enabled: !OGL.BackendService.busy
                }

                Label {
                    text: qsTr("H:")
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: heightInput
                    Layout.fillWidth: true
                    text: "10"
                    validator: DoubleValidator {
                        bottom: 0.001
                    }
                    enabled: !OGL.BackendService.busy
                }

                Label {
                    text: qsTr("D:")
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: depthInput
                    Layout.fillWidth: true
                    text: "10"
                    validator: DoubleValidator {
                        bottom: 0.001
                    }
                    enabled: !OGL.BackendService.busy
                }
            }
        }

        // Status area.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

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
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "AddBox")
                root.closeRequested();
        }
    }
}
