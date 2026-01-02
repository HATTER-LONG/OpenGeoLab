pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file AddLineToolDialog.qml
 * @brief Non-modal tool dialog for adding a line with start and end points
 *
 * Allows users to create a new line geometry by specifying start and end coordinates.
 * The dialog is non-modal, enabling viewport interaction during input.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Add Line")
    okButtonText: qsTr("Create")

    okEnabled: !OGL.BackendService.busy

    onAccepted: {
        const params = {
            startX: parseFloat(startXInput.text) || 0,
            startY: parseFloat(startYInput.text) || 0,
            startZ: parseFloat(startZInput.text) || 0,
            endX: parseFloat(endXInput.text) || 0,
            endY: parseFloat(endYInput.text) || 0,
            endZ: parseFloat(endZInput.text) || 0
        };
        OGL.BackendService.request("addLine", params);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Start point section
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Start Point")

            background: Rectangle {
                y: parent.topPadding - parent.bottomPadding
                width: parent.width
                height: parent.height - parent.topPadding + parent.bottomPadding
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
                    id: startXInput
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
                    id: startYInput
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
                    id: startZInput
                    Layout.fillWidth: true
                    text: "0"
                    validator: DoubleValidator {}
                    enabled: !OGL.BackendService.busy
                }
            }
        }

        // End point section
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("End Point")

            background: Rectangle {
                y: parent.topPadding - parent.bottomPadding
                width: parent.width
                height: parent.height - parent.topPadding + parent.bottomPadding
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
                    id: endXInput
                    Layout.fillWidth: true
                    text: "10"
                    validator: DoubleValidator {}
                    enabled: !OGL.BackendService.busy
                }

                Label {
                    text: "Y:"
                    color: Theme.textPrimaryColor
                }
                TextField {
                    id: endYInput
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
                    id: endZInput
                    Layout.fillWidth: true
                    text: "0"
                    validator: DoubleValidator {}
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

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "addLine") {
                root.closeRequested();
            }
        }
    }
}
