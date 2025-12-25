pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Rectangle {
    id: ribbonMenu
    width: parent.width
    height: 120
    color: "#2c3e50"

    // Generic signal keeps the interface stable while letting callers attach handlers for any action.
    signal actionTriggered(string actionId, var action)

    // Provide data-driven actions instead of per-button signals.
    property var actions: []

    // Allow consumers to override how buttons look without rewriting the layout.
    property Component buttonDelegate: defaultButtonDelegate

    Row {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        Repeater {
            model: ribbonMenu.actions
            delegate: ribbonMenu.buttonDelegate
        }
    }

    Component {
        id: defaultButtonDelegate

        Button {
            required property var modelData

            text: modelData.label ?? ""
            width: 96
            height: 40

            onClicked: {
                if (typeof modelData.onTriggered === "function") {
                    modelData.onTriggered(modelData);
                }
                ribbonMenu.actionTriggered(modelData.id ?? "", modelData);
            }
        }
    }
}
