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
    preferredContentHeight: 260

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
        width: parent.width
        spacing: 12

        // Start point section
        GroupBox {
            id: startPointGroup
            Layout.fillWidth: true
            title: qsTr("Start Point")

            background: Rectangle {
                y: startPointGroup.topPadding - startPointGroup.bottomPadding
                width: parent.width
                height: parent.height - startPointGroup.topPadding + startPointGroup.bottomPadding
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
                    color: Theme.textPrimaryColor
                    placeholderTextColor: Theme.textSecondaryColor
                    background: Rectangle {
                        implicitHeight: 32
                        radius: 6
                        color: startXInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                        border.width: 1
                        border.color: startXInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    }
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
                    color: Theme.textPrimaryColor
                    placeholderTextColor: Theme.textSecondaryColor
                    background: Rectangle {
                        implicitHeight: 32
                        radius: 6
                        color: startYInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                        border.width: 1
                        border.color: startYInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    }
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
                    color: Theme.textPrimaryColor
                    placeholderTextColor: Theme.textSecondaryColor
                    background: Rectangle {
                        implicitHeight: 32
                        radius: 6
                        color: startZInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                        border.width: 1
                        border.color: startZInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    }
                }
            }
        }

        // End point section
        GroupBox {
            id: endPointGroup
            Layout.fillWidth: true
            title: qsTr("End Point")

            background: Rectangle {
                y: endPointGroup.topPadding - endPointGroup.bottomPadding
                width: parent.width
                height: parent.height - endPointGroup.topPadding + endPointGroup.bottomPadding
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
                    color: Theme.textPrimaryColor
                    placeholderTextColor: Theme.textSecondaryColor
                    background: Rectangle {
                        implicitHeight: 32
                        radius: 6
                        color: endXInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                        border.width: 1
                        border.color: endXInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    }
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
                    color: Theme.textPrimaryColor
                    placeholderTextColor: Theme.textSecondaryColor
                    background: Rectangle {
                        implicitHeight: 32
                        radius: 6
                        color: endYInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                        border.width: 1
                        border.color: endYInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    }
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
                    color: Theme.textPrimaryColor
                    placeholderTextColor: Theme.textSecondaryColor
                    background: Rectangle {
                        implicitHeight: 32
                        radius: 6
                        color: endZInput.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                        border.width: 1
                        border.color: endZInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    }
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
            if (moduleName === "addLine") {
                root.closeRequested();
            }
        }
    }
}
