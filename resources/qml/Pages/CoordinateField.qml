/**
 * @file CoordinateField.qml
 * @brief 3D coordinate input component (X, Y, Z)
 *
 * Provides three number inputs for 3D coordinate entry with consistent styling.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Item {
    id: root

    // =========================================================
    // Public API
    // =========================================================

    /// Label text displayed above the inputs
    property string label: ""
    /// X coordinate value
    property real coordX: 0.0
    /// Y coordinate value
    property real coordY: 0.0
    /// Z coordinate value
    property real coordZ: 0.0
    /// Decimal places
    property int decimals: 3
    /// Whether the fields are read-only
    property bool readOnly: false

    /// Signal emitted when any coordinate changes
    signal coordinateChanged(real coordX, real coordY, real coordZ)

    // =========================================================
    // Layout
    // =========================================================

    implicitWidth: parent ? parent.width : 200
    implicitHeight: column.implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: 4

        // Label
        Label {
            visible: root.label.length > 0
            text: root.label
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        // Coordinate inputs row
        RowLayout {
            width: parent.width
            spacing: 6

            // X input
            CoordInput {
                id: xInput
                Layout.fillWidth: true
                label: "X"
                value: root.coordX
                decimals: root.decimals
                readOnly: root.readOnly
                accentColor: "#E53935" // Red for X
                onValueEdited: newVal => {
                    root.coordX = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }

            // Y input
            CoordInput {
                id: yInput
                Layout.fillWidth: true
                label: "Y"
                value: root.coordY
                decimals: root.decimals
                readOnly: root.readOnly
                accentColor: "#43A047" // Green for Y
                onValueEdited: newVal => {
                    root.coordY = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }

            // Z input
            CoordInput {
                id: zInput
                Layout.fillWidth: true
                label: "Z"
                value: root.coordZ
                decimals: root.decimals
                readOnly: root.readOnly
                accentColor: "#1E88E5" // Blue for Z
                onValueEdited: newVal => {
                    root.coordZ = newVal;
                    root.coordinateChanged(root.coordX, root.coordY, root.coordZ);
                }
            }
        }
    }

    // =========================================================
    // Internal coordinate input component
    // =========================================================
    component CoordInput: Item {
        id: coordItem

        property string label: ""
        property real value: 0.0
        property int decimals: 3
        property bool readOnly: false
        property color accentColor: Theme.accent

        signal valueEdited(real newVal)

        implicitHeight: 28

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: coordItem.readOnly ? Theme.surfaceAlt : Theme.surface
            border.width: coordField.activeFocus ? 2 : 1
            border.color: coordField.activeFocus ? coordItem.accentColor : Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 2

                // Axis label
                Rectangle {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    radius: 3
                    color: Qt.rgba(coordItem.accentColor.r,
                                   coordItem.accentColor.g,
                                   coordItem.accentColor.b, 0.2)

                    Label {
                        anchors.centerIn: parent
                        text: coordItem.label
                        font.pixelSize: 10
                        font.bold: true
                        color: coordItem.accentColor
                    }
                }

                TextField {
                    id: coordField
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    text: coordItem.value.toFixed(coordItem.decimals)
                    readOnly: coordItem.readOnly
                    selectByMouse: true

                    font.pixelSize: 11
                    color: Theme.textPrimary

                    background: Item {}

                    validator: DoubleValidator {
                        decimals: coordItem.decimals
                        notation: DoubleValidator.StandardNotation
                    }

                    onEditingFinished: {
                        const newVal = parseFloat(text) || 0;
                        if (newVal !== coordItem.value) {
                            coordItem.valueEdited(newVal);
                        }
                    }
                }
            }
        }
    }
}
