pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../theme"

Item {
    id: root

    required property AppTheme theme
    required property var pageState

    width: parent ? parent.width : 0
    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn

        width: root.width
        spacing: root.theme.gap

        SectionCard {
            Layout.fillWidth: true
            theme: root.theme
            title: root.pageState.dimensionTitle
            subtitle: qsTr("Define the primitive name and driving dimensions before execution.")

            GridLayout {
                width: parent.width
                columns: Math.max(1, Math.min(3, root.pageState.dimensionFields.length))
                columnSpacing: 10
                rowSpacing: 8

                ParameterInputField {
                    Layout.fillWidth: true
                    Layout.columnSpan: 3
                    theme: root.theme
                    label: qsTr("Model Name")
                    value: root.pageState.fieldValue("modelName")
                    placeholderText: root.pageState.requestSpec && root.pageState.requestSpec.defaultName ? root.pageState.requestSpec.defaultName : ""
                    accentColor: root.theme.resolveAccentColor(root.pageState.accentName)
                    showAccentMarker: true
                    onValueEdited: function (nextValue) {
                        root.pageState.setEditedFieldValue("modelName", nextValue);
                    }
                }

                Repeater {
                    model: root.pageState.dimensionFields

                    delegate: ParameterInputField {
                        required property var modelData

                        Layout.fillWidth: true
                        theme: root.theme
                        label: modelData.label
                        value: root.pageState.fieldValue(modelData.key)
                        unit: modelData.unit
                        numeric: true
                        invalid: root.pageState.invalidFieldKey === modelData.key
                        accentColor: root.theme.resolveAccentColor(modelData.accent ? modelData.accent : root.pageState.accentName)
                        showAccentMarker: true
                        placeholderText: modelData.defaultValue
                        onValueEdited: function (nextValue) {
                            root.pageState.setEditedFieldValue(modelData.key, nextValue);
                        }
                    }
                }
            }
        }
    }
}
