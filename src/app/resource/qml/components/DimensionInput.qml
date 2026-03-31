pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root

    property string label: ""
    property real value: 0.0
    property int decimals: 3
    property real minValue: 0.001
    property color accentColor: root.theme.accentA
    property string tooltipText: ""
    property AppTheme theme: MainPages.theme

    signal valueEdited(real newVal)

    implicitHeight: 28

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusSmall
        color: root.theme.surface
        border.width: dimField.activeFocus ? 2 : 1
        border.color: dimField.activeFocus ? root.accentColor : root.theme.borderSubtle

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            spacing: 2

            Rectangle {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                radius: 3
                color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.2)

                Text {
                    anchors.centerIn: parent
                    text: root.label
                    font.pixelSize: 10
                    font.bold: true
                    color: root.accentColor
                }

                ToolTip.visible: root.tooltipText.length > 0 && badgeMouseArea.containsMouse
                ToolTip.text: root.tooltipText
                ToolTip.delay: 500

                MouseArea {
                    id: badgeMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }

            TextField {
                id: dimField

                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.value.toFixed(root.decimals)
                selectByMouse: true
                font.pixelSize: 11
                color: root.theme.textPrimary
                background: Item {}

                validator: DoubleValidator {
                    bottom: root.minValue
                    decimals: root.decimals
                    notation: DoubleValidator.StandardNotation
                }

                onEditingFinished: {
                    const newVal = Math.max(root.minValue, parseFloat(text) || 0);
                    if (newVal !== root.value) {
                        root.valueEdited(newVal);
                    }
                }
            }
        }
    }
}
