pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file AddBoxToolDialog.qml
 * @brief Non-modal tool dialog for adding a box with origin and dimensions
 *
 * Allows users to create a new box geometry by specifying origin coordinates
 * and dimensions (width, height, depth).
 * The dialog is non-modal, enabling viewport interaction during input.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Add Box")
    okButtonText: qsTr("Create")
    preferredContentHeight: 320

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
        OGL.BackendService.request("AddBox", params);
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        // Name input
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("Name:")
                color: Theme.textPrimaryColor
                Layout.preferredWidth: 50
            }
            TextField {
                id: nameInput
                Layout.fillWidth: true
                placeholderText: qsTr("Box")
                text: ""
                enabled: !OGL.BackendService.busy
            }
        }

        // Origin section
        GroupBox {
            id: originGroup
            Layout.fillWidth: true
            title: qsTr("Origin")

            background: Rectangle {
                y: originGroup.topPadding - originGroup.bottomPadding
                width: parent.width
                height: parent.height - originGroup.topPadding + originGroup.bottomPadding
                color: Theme.surfaceAltColor
                radius: 6
                border.width: 1
                border.color: Theme.borderColor
            }

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

        // Dimensions section
        GroupBox {
            id: dimensionsGroup
            Layout.fillWidth: true
            title: qsTr("Dimensions")

            background: Rectangle {
                y: dimensionsGroup.topPadding - dimensionsGroup.bottomPadding
                width: parent.width
                height: parent.height - dimensionsGroup.topPadding + dimensionsGroup.bottomPadding
                color: Theme.surfaceAltColor
                radius: 6
                border.width: 1
                border.color: Theme.borderColor
            }

            GridLayout {
                anchors.fill: parent
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                Label {
                    text: qsTr("Width:")
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
                    text: qsTr("Height:")
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
                    text: qsTr("Depth:")
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
            if (moduleName === "AddBox") {
                root.closeRequested();
            }
        }
    }
}
