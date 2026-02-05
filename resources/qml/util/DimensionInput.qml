pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Item {
    id: dimItem

    property string label: ""
    property string tooltipText: ""
    property real value: 0.0
    property int decimals: 3
    property color accentColor: Theme.accent

    signal valueEdited(real newVal)

    implicitHeight: 28

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: Theme.surface
        border.width: dimField.activeFocus ? 2 : 1
        border.color: dimField.activeFocus ? dimItem.accentColor : Theme.border

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            spacing: 2

            Rectangle {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                radius: 3
                color: Qt.rgba(dimItem.accentColor.r, dimItem.accentColor.g, dimItem.accentColor.b, 0.2)

                Label {
                    anchors.centerIn: parent
                    text: dimItem.label
                    font.pixelSize: 10
                    font.bold: true
                    color: dimItem.accentColor
                }

                ToolTip.visible: dimItem.tooltipText && dimMouseArea.containsMouse
                ToolTip.text: dimItem.tooltipText
                ToolTip.delay: 500

                MouseArea {
                    id: dimMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }

            TextField {
                id: dimField
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: dimItem.value.toFixed(dimItem.decimals)
                selectByMouse: true

                font.pixelSize: 11
                color: Theme.textPrimary

                background: Item {}

                validator: DoubleValidator {
                    bottom: 0.001
                    decimals: dimItem.decimals
                    notation: DoubleValidator.StandardNotation
                }

                onEditingFinished: {
                    const newVal = Math.max(0.001, parseFloat(text) || 0);
                    if (newVal !== dimItem.value) {
                        dimItem.valueEdited(newVal);
                    }
                }
            }
        }
    }
}
