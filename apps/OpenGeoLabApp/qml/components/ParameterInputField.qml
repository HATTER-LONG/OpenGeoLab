pragma ComponentBehavior: Bound

import QtQuick

import "../theme"

Rectangle {
    id: field

    required property AppTheme theme
    property string label: ""
    property string value: ""
    property string unit: ""
    property string placeholderText: ""
    property bool numeric: false
    property bool invalid: false
    property color accentColor: theme.borderSubtle
    property bool showAccentMarker: false
    readonly property color focusColor: showAccentMarker ? accentColor : theme.accentA
    signal valueEdited(string value)

    radius: theme.radiusSmall
    color: theme.tint(theme.surface, theme.darkMode ? (fieldInput.activeFocus ? 0.86 : 0.76) : (fieldInput.activeFocus ? 1.0 : 0.99))
    border.width: 1
    border.color: invalid ? theme.tint(theme.accentD, theme.darkMode ? 0.72 : 0.42) : (fieldInput.activeFocus ? theme.tint(focusColor, theme.darkMode ? 0.68 : 0.36) : theme.tint(showAccentMarker ? accentColor : theme.borderSubtle, theme.darkMode ? 0.42 : 0.2))
    implicitHeight: fieldColumn.implicitHeight + 14

    Column {
        id: fieldColumn

        anchors.fill: parent
        anchors.margins: 7
        spacing: 4

        Row {
            width: parent.width
            spacing: 6

            Rectangle {
                visible: field.showAccentMarker
                width: 8
                height: 8
                radius: 4
                anchors.verticalCenter: parent.verticalCenter
                color: field.accentColor
                border.width: 1
                border.color: field.theme.tint(field.accentColor, field.theme.darkMode ? 0.84 : 0.5)
            }

            Text {
                width: field.showAccentMarker ? parent.width - unitLabel.implicitWidth - 14 : parent.width - unitLabel.implicitWidth
                text: field.label
                color: field.showAccentMarker ? field.theme.tint(field.accentColor, field.theme.darkMode ? 0.94 : 1.0) : field.theme.textSecondary
                font.pixelSize: 10
                font.bold: true
                font.family: field.theme.bodyFontFamily
                elide: Text.ElideRight
            }

            Text {
                id: unitLabel

                visible: false
            }
        }

        Rectangle {
            width: parent.width
            height: 28
            radius: field.theme.radiusSmall
            color: field.theme.tint(field.showAccentMarker && fieldInput.activeFocus ? field.focusColor : field.theme.surfaceMuted, field.theme.darkMode ? (fieldInput.activeFocus ? 0.18 : 0.66) : (fieldInput.activeFocus ? 0.08 : 0.94))
            border.width: 1
            border.color: field.invalid
                ? field.theme.tint(field.theme.accentD, field.theme.darkMode ? 0.7 : 0.42)
                : (fieldInput.activeFocus
                    ? field.theme.tint(field.focusColor, field.theme.darkMode ? 0.76 : 0.4)
                    : field.theme.tint(field.showAccentMarker ? field.accentColor : field.theme.borderSubtle,
                                       field.theme.darkMode ? 0.56 : 0.3))

            TextInput {
                id: fieldInput

                anchors.fill: parent
                anchors.leftMargin: 9
                anchors.rightMargin: field.unit.length > 0 ? unitChip.width + 12 : 9
                text: field.value
                color: field.theme.textPrimary
                font.pixelSize: 11
                font.family: field.theme.monoFontFamily
                selectByMouse: true
                clip: true
                verticalAlignment: TextInput.AlignVCenter
                selectedTextColor: field.theme.textPrimary
                selectionColor: field.theme.tint(field.showAccentMarker ? field.accentColor : field.theme.accentA, field.theme.darkMode ? 0.34 : 0.22)
                inputMethodHints: field.numeric ? Qt.ImhFormattedNumbersOnly : Qt.ImhNoPredictiveText
                validator: field.numeric ? numericValidator : null

                onTextEdited: field.valueEdited(text)

                DoubleValidator {
                    id: numericValidator

                    notation: DoubleValidator.StandardNotation
                }
            }

            Rectangle {
                id: unitChip

                visible: field.unit.length > 0
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                height: 18
                radius: 9
                color: field.theme.tint(field.showAccentMarker ? field.accentColor : field.theme.accentA, field.theme.darkMode ? 0.18 : 0.1)
                border.width: 1
                border.color: field.theme.tint(field.showAccentMarker ? field.accentColor : field.theme.accentA, field.theme.darkMode ? 0.48 : 0.24)
                width: unitChipLabel.implicitWidth + 10

                Text {
                    id: unitChipLabel

                    anchors.centerIn: parent
                    text: field.unit
                    color: field.theme.textPrimary
                    font.pixelSize: 10
                    font.bold: true
                    font.family: field.theme.bodyFontFamily
                }
            }

            Text {
                visible: field.placeholderText.length > 0 && fieldInput.length === 0 && !fieldInput.activeFocus
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: field.placeholderText
                color: field.theme.textTertiary
                font.pixelSize: 11
                font.family: field.theme.monoFontFamily
            }
        }
    }
}
