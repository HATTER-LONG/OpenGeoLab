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
            title: qsTr("Shape Parameters")
            subtitle: root.pageState.supportsAxis
                ? qsTr("Define the primitive name and dimensions before confirming the creation direction.")
                : qsTr("Define the primitive name and dimension values before execution.")

            GridLayout {
                width: parent.width
                columns: 3
                columnSpacing: 10
                rowSpacing: 8

                ParameterInputField {
                    Layout.fillWidth: true
                    Layout.columnSpan: 3
                    theme: root.theme
                    label: qsTr("Model Name")
                    value: root.pageState.modelNameValue
                    placeholderText: root.pageState.requestSpec ? root.pageState.requestSpec.defaultName : ""
                    onValueEdited: function (nextValue) {
                        root.pageState.modelNameValue = nextValue;
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
                        onValueEdited: function (nextValue) {
                            root.pageState.setEditedFieldValue(modelData.key, nextValue);
                        }
                    }
                }
            }
        }

        GeometryCreateOrientationSection {
            Layout.fillWidth: true
            theme: root.theme
            pageState: root.pageState
            title: qsTr("Direction")
            subtitle: qsTr("Keep the creation axis visible on the setup page so directional primitives do not lose that control.")
        }
    }
}
