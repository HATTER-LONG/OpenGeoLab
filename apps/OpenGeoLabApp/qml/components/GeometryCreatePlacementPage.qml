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
            title: root.pageState.positionTitle
            subtitle: qsTr("Set the placement coordinates used by the current geometry action.")

            GridLayout {
                width: parent.width
                columns: Math.max(1, Math.min(3, root.pageState.placementFields.length))
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
                        accentColor: root.theme.resolveAccentColor(modelData.accent ? modelData.accent : root.pageState.accentName)
                        showAccentMarker: true
                        invalid: root.pageState.invalidFieldKey === modelData.key
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
