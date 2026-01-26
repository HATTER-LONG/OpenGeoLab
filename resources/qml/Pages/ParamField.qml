/**
 * @file ParamField.qml
 * @brief Styled parameter input field component
 *
 * Provides a labeled input field with consistent styling for function pages.
 * Supports text, number, and coordinate inputs.
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

    /// Label text displayed above the input
    property string label: ""
    /// Placeholder text for empty input
    property string placeholder: ""
    /// Current input value
    property string value: ""
    /// Input type: "text", "number", "coordinate"
    property string inputType: "text"
    /// Optional suffix (e.g., "mm", "°")
    property string suffix: ""
    /// Whether the field is read-only
    property bool readOnly: false
    /// Minimum value for number type
    property real minValue: -Number.MAX_VALUE
    /// Maximum value for number type
    property real maxValue: Number.MAX_VALUE
    /// Decimal places for number type
    property int decimals: 2

    /// Signal emitted when value is edited by user
    signal valueEdited(string newValue)

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

        // Input container
        Rectangle {
            width: parent.width
            height: 28
            radius: 4
            color: root.readOnly ? Theme.surfaceAlt : Theme.surface
            border.width: inputField.activeFocus ? 2 : 1
            border.color: inputField.activeFocus ? Theme.accent : Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                TextField {
                    id: inputField
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    text: root.value
                    placeholderText: root.placeholder
                    readOnly: root.readOnly
                    selectByMouse: true

                    font.pixelSize: 12
                    color: Theme.textPrimary
                    placeholderTextColor: Theme.textDisabled

                    background: Item {}

                    validator: root.inputType === "number" ?
                        doubleValidator : null

                    onTextChanged: {
                        if (text !== root.value) {
                            root.value = text;
                            root.valueEdited(text);
                        }
                    }

                    onEditingFinished: {
                        if (root.inputType === "number") {
                            // Clamp and format number
                            let num = parseFloat(text) || 0;
                            num = Math.max(root.minValue, Math.min(root.maxValue, num));
                            text = num.toFixed(root.decimals);
                        }
                    }
                }

                // Suffix label
                Label {
                    visible: root.suffix.length > 0
                    text: root.suffix
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }
            }
        }
    }

    DoubleValidator {
        id: doubleValidator
        bottom: root.minValue
        top: root.maxValue
        decimals: root.decimals
        notation: DoubleValidator.StandardNotation
    }
}
