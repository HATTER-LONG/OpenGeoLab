pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @brief Dialog for adding a line with start and end points.
 */
Pages.BaseDialog {
    id: root

    title: qsTr("Add Line")
    okButtonText: qsTr("Create")

    property var initialParams: ({})

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

        // Start point section.
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Start Point")

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
                    placeholderText: "0.0"
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
                    placeholderText: "0.0"
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
                    placeholderText: "0.0"
                    text: "0"
                    validator: DoubleValidator {}
                    enabled: !OGL.BackendService.busy
                }
            }
        }

        // End point section.
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("End Point")

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
                    placeholderText: "0.0"
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
                    placeholderText: "0.0"
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
                    placeholderText: "0.0"
                    text: "0"
                    validator: DoubleValidator {}
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
            if (moduleName === "AddLine")
                root.closeRequested();
        }
    }
}
