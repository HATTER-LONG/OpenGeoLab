import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: root

    property string label: ""
    property string placeholder: ""
    property string value: ""

    signal valueEdited(string newValue)

    property bool _updating: false

    implicitWidth: 180
    implicitHeight: contentColumn.implicitHeight

    Column {
        id: contentColumn

        width: root.width > 0 ? root.width : root.implicitWidth
        spacing: 4

        Text {
            text: root.label
            visible: root.label.length > 0
            color: MainPages.theme.textSecondary
            font.pixelSize: 12
        }

        Rectangle {
            width: parent.width
            height: 28
            radius: MainPages.theme.radiusSmall
            color: MainPages.theme.surface
            border.width: inputField.activeFocus ? 2 : 1
            border.color: inputField.activeFocus ? MainPages.theme.accentA : MainPages.theme.borderSubtle

            TextField {
                id: inputField

                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                text: root.value
                color: MainPages.theme.textPrimary
                placeholderText: root.placeholder
                placeholderTextColor: MainPages.theme.textSecondary
                selectByMouse: true
                verticalAlignment: TextInput.AlignVCenter
                background: Item {}

                onTextChanged: {
                    if (!root._updating && root.value !== text) {
                        root._updating = true;
                        root.value = text;
                        root.valueEdited(text);
                        root._updating = false;
                    }
                }
            }

            Connections {
                target: root

                function onValueChanged() {
                    if (!root._updating && inputField.text !== root.value) {
                        root._updating = true;
                        inputField.text = root.value;
                        root._updating = false;
                    }
                }
            }
        }
    }
}
