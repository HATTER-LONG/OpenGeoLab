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

        GeometryCreateOrientationSection {
            Layout.fillWidth: true
            theme: root.theme
            pageState: root.pageState
        }

        SectionCard {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Placement")
            subtitle: qsTr("Position the primitive using workspace origin coordinates.")

            GridLayout {
                width: parent.width
                columns: 3
                columnSpacing: 10
                rowSpacing: 10

                Repeater {
                    model: root.pageState.placementFields

                    delegate: ParameterInputField {
                        required property var modelData

                        Layout.fillWidth: true
                        theme: root.theme
                        label: modelData.label
                        value: root.pageState.fieldValue(modelData.key)
                        unit: modelData.unit
                        numeric: true
                        accentColor: root.pageState.fieldAccentColor(modelData.key)
                        showAccentMarker: true
                        invalid: root.pageState.invalidFieldKey === modelData.key
                        onValueEdited: function (nextValue) {
                            root.pageState.setEditedFieldValue(modelData.key, nextValue);
                        }
                    }
                }
            }
        }
    }
}
