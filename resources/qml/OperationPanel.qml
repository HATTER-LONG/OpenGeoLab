pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

/**
 * @brief Generic operation panel for Ribbon toolbar actions
 *
 * This panel appears when a tool button is clicked, providing
 * a consistent UI for selection and workflow instructions.
 */
Rectangle {
    id: operationPanel

    // Panel configuration
    property string title: "Operation"
    property string selectionHint: "Select the entities you want to Operate"
    property bool showWorkflowSection: true

    // Selection state
    property int selectedCount: 0
    property bool hasValidSelection: selectedCount > 0

    // Signals
    signal applyClicked
    signal cancelClicked
    signal selectionRequested

    width: 280
    height: showWorkflowSection ? 180 : 140
    color: "#FFFFFF"
    border.color: "#C0C0C0"
    border.width: 1
    radius: 0

    // Drop shadow effect simulation
    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        z: -1
        color: "#20000000"
        radius: 0
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // Title bar
        Rectangle {
            width: parent.width
            height: 28
            color: "#F0F0F0"
            border.color: "#C0C0C0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 4

                // Title icon and text
                Row {
                    spacing: 6
                    Layout.fillWidth: true

                    Text {
                        text: "⚡"
                        font.pixelSize: 14
                        color: "#333333"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: operationPanel.title
                        font.pixelSize: 12
                        font.bold: false
                        color: "#333333"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Close button
                Rectangle {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    color: closeMouseArea.containsMouse ? "#E81123" : "transparent"
                    radius: 2

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 10
                        color: closeMouseArea.containsMouse ? "white" : "#666666"
                    }

                    MouseArea {
                        id: closeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: operationPanel.cancelClicked()
                    }
                }
            }
        }

        // Content area
        Item {
            width: parent.width
            height: parent.height - 28 - 40  // Subtract title and button bar

            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // Selection section
                Column {
                    width: parent.width
                    spacing: 4

                    // Section header
                    Row {
                        spacing: 6

                        Text {
                            text: "▼"
                            font.pixelSize: 8
                            color: "#333333"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "Selection"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#333333"
                        }
                    }

                    // Selection indicator
                    Rectangle {
                        width: parent.width
                        height: 24
                        color: operationPanel.hasValidSelection ? "#90EE90" : "#FFFF00"
                        border.color: operationPanel.hasValidSelection ? "#228B22" : "#DAA520"
                        border.width: 1
                        radius: 2

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            // Play button for selection
                            Rectangle {
                                width: 16
                                height: 16
                                anchors.verticalCenter: parent.verticalCenter
                                color: selectMouseArea.containsMouse ? "#E0E0E0" : "transparent"
                                radius: 2

                                Text {
                                    anchors.centerIn: parent
                                    text: "▶"
                                    font.pixelSize: 10
                                    color: "#333333"
                                }

                                MouseArea {
                                    id: selectMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: operationPanel.selectionRequested()
                                }
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: operationPanel.hasValidSelection ? operationPanel.selectedCount + " entities selected" : operationPanel.selectionHint
                                font.pixelSize: 11
                                color: "#333333"
                                elide: Text.ElideRight
                                width: parent.width - 50
                            }

                            // Clear selection button (X)
                            Rectangle {
                                width: 16
                                height: 16
                                anchors.verticalCenter: parent.verticalCenter
                                color: clearMouseArea.containsMouse ? "#FFB6C1" : "transparent"
                                radius: 2
                                visible: operationPanel.hasValidSelection

                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    font.pixelSize: 10
                                    color: "#C00000"
                                }

                                MouseArea {
                                    id: clearMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: operationPanel.selectedCount = 0
                                }
                            }
                        }
                    }
                }

                // Workflow instruction section
                Column {
                    width: parent.width
                    spacing: 4
                    visible: operationPanel.showWorkflowSection

                    Row {
                        spacing: 6

                        Text {
                            text: "▲"
                            font.pixelSize: 8
                            color: "#333333"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "Workflow Instruction"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#333333"
                        }
                    }
                }
            }
        }

        // Button bar
        Rectangle {
            width: parent.width
            height: 40
            color: "#F5F5F5"
            border.color: "#E0E0E0"
            border.width: 1

            Row {
                anchors.centerIn: parent
                spacing: 10

                Button {
                    id: applyButton
                    text: "Apply"
                    width: 75
                    height: 26
                    enabled: operationPanel.hasValidSelection

                    background: Rectangle {
                        color: applyButton.enabled ? (applyButton.hovered ? "#E5F1FB" : "#FFFFFF") : "#F0F0F0"
                        border.color: applyButton.enabled ? "#0078D4" : "#C0C0C0"
                        border.width: 1
                        radius: 2
                    }

                    contentItem: Text {
                        text: applyButton.text
                        font.pixelSize: 11
                        color: applyButton.enabled ? "#333333" : "#999999"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: operationPanel.applyClicked()
                }

                Button {
                    id: cancelButton
                    text: "Cancel"
                    width: 75
                    height: 26

                    background: Rectangle {
                        color: cancelButton.hovered ? "#E5F1FB" : "#FFFFFF"
                        border.color: "#C0C0C0"
                        border.width: 1
                        radius: 2
                    }

                    contentItem: Text {
                        text: cancelButton.text
                        font.pixelSize: 11
                        color: "#333333"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: operationPanel.cancelClicked()
                }
            }
        }
    }

    // Reset panel state
    function reset(): void {
        selectedCount = 0;
    }

    // Update selection count
    function setSelection(count: int): void {
        selectedCount = count;
    }
}
