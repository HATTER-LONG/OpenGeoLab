pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

/**
 * @file OperationPanel.qml
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

    // ========================================================================
    // Dark theme colors (fixed)
    // ========================================================================
    readonly property color panelBackgroundColor: "#252830"
    readonly property color titleBarColor: "#1e2127"
    readonly property color buttonBarColor: "#1a1d24"
    readonly property color textColor: "#e1e1e1"
    readonly property color textColorDim: "#a0a0a0"
    readonly property color borderColor: "#3a3f4b"
    readonly property color accentColor: "#0d6efd"
    readonly property color hoverColor: "#3a3f4b"
    readonly property color selectionValidColor: "#2d4a3e"
    readonly property color selectionInvalidColor: "#4a4a2d"

    // Signals
    signal applyClicked
    signal cancelClicked
    signal selectionRequested

    width: 280
    height: showWorkflowSection ? 180 : 140
    color: panelBackgroundColor
    border.color: borderColor
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

        // Title bar (draggable)
        Rectangle {
            id: titleBar
            width: parent.width
            height: 28
            color: operationPanel.titleBarColor
            border.color: operationPanel.borderColor
            border.width: 1

            // Drag handler for the title bar
            MouseArea {
                id: dragArea
                anchors.fill: parent
                property point clickPos: Qt.point(0, 0)

                onPressed: mouse => {
                    clickPos = Qt.point(mouse.x, mouse.y);
                    cursorShape = Qt.ClosedHandCursor;
                }

                onReleased: {
                    cursorShape = Qt.OpenHandCursor;
                }

                onPositionChanged: mouse => {
                    if (pressed) {
                        let deltaX = mouse.x - clickPos.x;
                        let deltaY = mouse.y - clickPos.y;
                        operationPanel.x += deltaX;
                        operationPanel.y += deltaY;
                    }
                }

                hoverEnabled: true
                cursorShape: Qt.OpenHandCursor
            }

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
                        color: operationPanel.textColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: operationPanel.title
                        font.pixelSize: 12
                        font.bold: false
                        color: operationPanel.textColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Close button
                Rectangle {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    color: closeMouseArea.containsMouse ? "#E81123" : "transparent"
                    radius: 2
                    z: 1  // Ensure close button is above drag area

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 10
                        color: closeMouseArea.containsMouse ? "white" : operationPanel.textColorDim
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
                            color: operationPanel.textColorDim
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "Selection"
                            font.pixelSize: 12
                            font.bold: true
                            color: operationPanel.textColor
                        }
                    }

                    // Selection indicator
                    Rectangle {
                        width: parent.width
                        height: 24
                        color: operationPanel.hasValidSelection ? operationPanel.selectionValidColor : operationPanel.selectionInvalidColor
                        border.color: operationPanel.borderColor
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
                                color: selectMouseArea.containsMouse ? operationPanel.hoverColor : "transparent"
                                radius: 2

                                Text {
                                    anchors.centerIn: parent
                                    text: "▶"
                                    font.pixelSize: 10
                                    color: operationPanel.textColor
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
                                color: operationPanel.textColor
                                elide: Text.ElideRight
                                width: parent.width - 50
                            }

                            // Clear selection button (X)
                            Rectangle {
                                width: 16
                                height: 16
                                anchors.verticalCenter: parent.verticalCenter
                                color: clearMouseArea.containsMouse ? "#4a2d2d" : "transparent"
                                radius: 2
                                visible: operationPanel.hasValidSelection

                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    font.pixelSize: 10
                                    color: "#ff6b6b"
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
                            color: operationPanel.textColorDim
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "Workflow Instruction"
                            font.pixelSize: 12
                            font.bold: true
                            color: operationPanel.textColor
                        }
                    }
                }
            }
        }

        // Button bar
        Rectangle {
            width: parent.width
            height: 40
            color: operationPanel.buttonBarColor
            border.color: operationPanel.borderColor
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
                        color: applyButton.enabled ? (applyButton.hovered ? operationPanel.accentColor : operationPanel.hoverColor) : operationPanel.titleBarColor
                        border.color: applyButton.enabled ? operationPanel.accentColor : operationPanel.borderColor
                        border.width: 1
                        radius: 2
                    }

                    contentItem: Text {
                        text: applyButton.text
                        font.pixelSize: 11
                        color: applyButton.enabled ? operationPanel.textColor : operationPanel.textColorDim
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
                        color: cancelButton.hovered ? operationPanel.hoverColor : operationPanel.panelBackgroundColor
                        border.color: operationPanel.borderColor
                        border.width: 1
                        radius: 2
                    }

                    contentItem: Text {
                        text: cancelButton.text
                        font.pixelSize: 11
                        color: operationPanel.textColor
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
